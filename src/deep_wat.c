/**
 * Filename:deep_wat.c
 * Description:WAT（S 表达式）文本 → wasm 二进制的汇编器
 *
 * 支持折叠（嵌套）与平铺两种写法，覆盖 VM 已实现的所有 MVP 指令与 section。
 * 全程使用 libc malloc/free，不触碰 deep_mem 内存池。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <strings.h>
#include "deep_wat.h"
#include "deep_opcode.h"
#include "deep_loader.h"

/* ------------------------------------------------------------------ */
/* 错误输出                                                            */
/* ------------------------------------------------------------------ */

/* 可移植 strdup：避免对 _POSIX_C_SOURCE 的依赖 */
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (p) { memcpy(p, s, n); p[n] = 0; }
    return p;
}

static void wat_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "wat: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

/* ------------------------------------------------------------------ */
/* 增长式字节缓冲 + LEB128 / IEEE writer                               */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} ByteBuf;

static void bb_init(ByteBuf *b) { b->data = NULL; b->len = 0; b->cap = 0; }
static void bb_free(ByteBuf *b) { free(b->data); b->data = NULL; b->len = b->cap = 0; }

static void bb_reserve(ByteBuf *b, size_t extra) {
    if (b->len + extra <= b->cap) return;
    size_t cap = b->cap ? b->cap : 64;
    while (cap < b->len + extra) cap *= 2;
    b->data = (uint8_t *)realloc(b->data, cap);
    b->cap = cap;
}

static void bb_put(ByteBuf *b, uint8_t v) {
    bb_reserve(b, 1);
    b->data[b->len++] = v;
}

static void bb_put_bytes(ByteBuf *b, const uint8_t *p, size_t n) {
    bb_reserve(b, n);
    memcpy(b->data + b->len, p, n);
    b->len += n;
}

static void bb_put_u32le(ByteBuf *b, uint32_t v) {
    bb_put(b, (uint8_t)(v & 0xFF));
    bb_put(b, (uint8_t)((v >> 8) & 0xFF));
    bb_put(b, (uint8_t)((v >> 16) & 0xFF));
    bb_put(b, (uint8_t)((v >> 24) & 0xFF));
}

static void bb_put_u64le(ByteBuf *b, uint64_t v) {
    for (int i = 0; i < 8; i++) bb_put(b, (uint8_t)((v >> (8 * i)) & 0xFF));
}

static void bb_put_leb_u32(ByteBuf *b, uint32_t v) {
    do {
        uint8_t byte = v & 0x7F;
        v >>= 7;
        if (v) byte |= 0x80;
        bb_put(b, byte);
    } while (v);
}

static void bb_put_leb_s32(ByteBuf *b, int32_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = (uint8_t)(value & 0x7F);
        value >>= 7; /* 算术右移（gcc/clang） */
        bool sign = (byte & 0x40) != 0;
        bool done = ((value == 0) && !sign) || ((value == -1) && sign);
        if (done) more = false; else byte |= 0x80;
        bb_put(b, byte);
    }
}

static void bb_put_leb_s64(ByteBuf *b, int64_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = (uint8_t)(value & 0x7F);
        value >>= 7;
        bool sign = (byte & 0x40) != 0;
        bool done = ((value == 0) && !sign) || ((value == -1) && sign);
        if (done) more = false; else byte |= 0x80;
        bb_put(b, byte);
    }
}

static void bb_put_f32(ByteBuf *b, float f) {
    uint32_t u; memcpy(&u, &f, 4); bb_put_u32le(b, u);
}

static void bb_put_f64(ByteBuf *b, double d) {
    uint64_t u; memcpy(&u, &d, 8); bb_put_u64le(b, u);
}

/* ------------------------------------------------------------------ */
/* 词法分析                                                            */
/* ------------------------------------------------------------------ */
enum { TK_LPAREN = 0, TK_RPAREN = 1, TK_ATOM = 2 };

typedef struct {
    int kind;
    char *text;      /* NUL 结尾（用于名字/符号） */
    size_t len;      /* 原始字节长度（用于 data 字符串，可含 NUL） */
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t cap;
} TokenVec;

static void tv_push(TokenVec *v, int kind, const char *text, size_t len) {
    if (v->count == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 64;
        v->items = (Token *)realloc(v->items, v->cap * sizeof(Token));
    }
    Token *t = &v->items[v->count++];
    t->kind = kind;
    t->text = (char *)malloc(len + 1);
    memcpy(t->text, text, len);
    t->text[len] = 0;
    t->len = len;
}

static void tv_free(TokenVec *v) {
    for (size_t i = 0; i < v->count; i++) free(v->items[i].text);
    free(v->items);
}

static bool tokenize(const char *src, TokenVec *out) {
    const char *p = src;
    while (*p) {
        if (isspace((unsigned char)*p)) { p++; continue; }

        /* 行注释 ;; */
        if (p[0] == ';' && p[1] == ';') {
            while (*p && *p != '\n') p++;
            continue;
        }
        /* 块注释 (; ... ;) 可嵌套 */
        if (p[0] == '(' && p[1] == ';') {
            int depth = 1; p += 2;
            while (*p && depth > 0) {
                if (p[0] == '(' && p[1] == ';') { depth++; p += 2; }
                else if (p[0] == ';' && p[1] == ')') { depth--; p += 2; }
                else p++;
            }
            continue;
        }
        if (*p == '(') { tv_push(out, TK_LPAREN, "(", 1); p++; continue; }
        if (*p == ')') { tv_push(out, TK_RPAREN, ")", 1); p++; continue; }

        /* 字符串 "..." */
        if (*p == '"') {
            p++;
            char buf[4096]; size_t n = 0;
            while (*p && *p != '"') {
                unsigned char c = (unsigned char)*p;
                if (c == '\\') {
                    p++;
                    switch (*p) {
                    case 'n': c = '\n'; p++; break;
                    case 't': c = '\t'; p++; break;
                    case 'r': c = '\r'; p++; break;
                    case '\\': c = '\\'; p++; break;
                    case '"': c = '"'; p++; break;
                    case '\'': c = '\''; p++; break;
                    case '0': c = 0; p++; break;
                    case 'x': {
                        p++;
                        unsigned v = 0; int h = 0;
                        while (isxdigit((unsigned char)*p) && h < 2) {
                            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                                          (tolower((unsigned char)*p) - 'a' + 10));
                            p++; h++;
                        }
                        c = (unsigned char)v; break;
                    }
                    case 'u': {
                        if (p[1] == '{') {
                            p += 2; unsigned cp = 0;
                            while (*p && *p != '}') {
                                cp = cp * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                                                (tolower((unsigned char)*p) - 'a' + 10));
                                p++;
                            }
                            if (*p == '}') p++;
                            /* 编码为 UTF-8 */
                            if (cp < 0x80) { buf[n++] = (char)cp; }
                            else if (cp < 0x800) {
                                buf[n++] = (char)(0xC0 | (cp >> 6));
                                buf[n++] = (char)(0x80 | (cp & 0x3F));
                            } else {
                                buf[n++] = (char)(0xE0 | (cp >> 12));
                                buf[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                                buf[n++] = (char)(0x80 | (cp & 0x3F));
                            }
                            continue;
                        }
                        c = 'u'; break;
                    }
                    default: c = (unsigned char)*p; p++; break;
                    }
                } else {
                    p++;
                }
                if (n < sizeof(buf) - 1) buf[n++] = (char)c;
            }
            if (*p == '"') p++;
            tv_push(out, TK_ATOM, buf, n);
            continue;
        }

        /* 原子：符号 / 数字 */
        const char *start = p;
        while (*p && !isspace((unsigned char)*p) && *p != '(' && *p != ')' &&
               *p != ';' && *p != '"') {
            p++;
        }
        if (p > start) tv_push(out, TK_ATOM, start, (size_t)(p - start));
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* S 表达式树                                                          */
/* ------------------------------------------------------------------ */
enum { SX_ATOM = 0, SX_LIST = 1 };

typedef struct SExpr {
    int kind;
    char *text; size_t len;
    struct SExpr **kids; size_t nkids;
} SExpr;

static SExpr *sx_atom(const char *text, size_t len) {
    SExpr *s = (SExpr *)calloc(1, sizeof(SExpr));
    s->kind = SX_ATOM;
    s->text = (char *)malloc(len + 1);
    memcpy(s->text, text, len); s->text[len] = 0; s->len = len;
    return s;
}

static SExpr *sx_list(void) {
    SExpr *s = (SExpr *)calloc(1, sizeof(SExpr));
    s->kind = SX_LIST;
    return s;
}

static void sx_add(SExpr *list, SExpr *child) {
    list->kids = (SExpr **)realloc(list->kids, (list->nkids + 1) * sizeof(SExpr *));
    list->kids[list->nkids++] = child;
}

static void sx_free(SExpr *s) {
    if (!s) return;
    free(s->text);
    for (size_t i = 0; i < s->nkids; i++) sx_free(s->kids[i]);
    free(s->kids);
    free(s);
}

static SExpr *parse_one(TokenVec *toks, size_t *i) {
    if (*i >= toks->count) return NULL;
    Token *t = &toks->items[(*i)++];
    if (t->kind == TK_ATOM) return sx_atom(t->text, t->len);
    if (t->kind == TK_LPAREN) {
        SExpr *list = sx_list();
        while (*i < toks->count && toks->items[*i].kind != TK_RPAREN) {
            SExpr *child = parse_one(toks, i);
            if (child) sx_add(list, child);
        }
        if (*i < toks->count) (*i)++; /* consume ')' */
        return list;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* 助记符表：标准 WAT 名 → opcode                                      */
/* ------------------------------------------------------------------ */
typedef struct { const char *name; uint8_t op; } Mnemonic;

static const Mnemonic MNEMONICS[] = {
    {"unreachable",0x00},{"nop",0x01},{"block",0x02},{"loop",0x03},{"if",0x04},
    {"else",0x05},{"end",0x0B},{"br",0x0C},{"br_if",0x0D},{"br_table",0x0E},
    {"return",0x0F},{"call",0x10},{"call_indirect",0x11},{"drop",0x1A},{"select",0x1B},
    {"local.get",0x20},{"local.set",0x21},{"local.tee",0x22},
    {"global.get",0x23},{"global.set",0x24},
    {"i32.load",0x28},{"i64.load",0x29},{"f32.load",0x2A},{"f64.load",0x2B},
    {"i32.load8_s",0x2C},{"i32.load8_u",0x2D},{"i32.load16_s",0x2E},{"i32.load16_u",0x2F},
    {"i64.load8_s",0x30},{"i64.load8_u",0x31},{"i64.load16_s",0x32},{"i64.load16_u",0x33},
    {"i64.load32_s",0x34},{"i64.load32_u",0x35},
    {"i32.store",0x36},{"i64.store",0x37},{"f32.store",0x38},{"f64.store",0x39},
    {"i32.store8",0x3A},{"i32.store16",0x3B},{"i64.store8",0x3C},{"i64.store16",0x3D},
    {"i64.store32",0x3E},{"memory.size",0x3F},{"memory.grow",0x40},
    {"i32.const",0x41},{"i64.const",0x42},{"f32.const",0x43},{"f64.const",0x44},
    {"i32.eqz",0x45},{"i32.eq",0x46},{"i32.ne",0x47},{"i32.lt_s",0x48},{"i32.lt_u",0x49},
    {"i32.gt_s",0x4A},{"i32.gt_u",0x4B},{"i32.le_s",0x4C},{"i32.le_u",0x4D},
    {"i32.ge_s",0x4E},{"i32.ge_u",0x4F},
    {"i64.eqz",0x50},{"i64.eq",0x51},{"i64.ne",0x52},{"i64.lt_s",0x53},{"i64.lt_u",0x54},
    {"i64.gt_s",0x55},{"i64.gt_u",0x56},{"i64.le_s",0x57},{"i64.le_u",0x58},
    {"i64.ge_s",0x59},{"i64.ge_u",0x5A},
    {"f32.eq",0x5B},{"f32.ne",0x5C},{"f32.lt",0x5D},{"f32.gt",0x5E},
    {"f32.le",0x5F},{"f32.ge",0x60},
    {"f64.eq",0x61},{"f64.ne",0x62},{"f64.lt",0x63},{"f64.gt",0x64},
    {"f64.le",0x65},{"f64.ge",0x66},
    {"i32.clz",0x67},{"i32.ctz",0x68},{"i32.popcnt",0x69},
    {"i32.add",0x6A},{"i32.sub",0x6B},{"i32.mul",0x6C},
    {"i32.div_s",0x6D},{"i32.div_u",0x6E},{"i32.rem_s",0x6F},{"i32.rem_u",0x70},
    {"i32.and",0x71},{"i32.or",0x72},{"i32.xor",0x73},
    {"i32.shl",0x74},{"i32.shr_s",0x75},{"i32.shr_u",0x76},{"i32.rotl",0x77},{"i32.rotr",0x78},
    {"i64.clz",0x79},{"i64.ctz",0x7A},{"i64.popcnt",0x7B},
    {"i64.add",0x7C},{"i64.sub",0x7D},{"i64.mul",0x7E},
    {"i64.div_s",0x7F},{"i64.div_u",0x80},{"i64.rem_s",0x81},{"i64.rem_u",0x82},
    {"i64.and",0x83},{"i64.or",0x84},{"i64.xor",0x85},
    {"i64.shl",0x86},{"i64.shr_s",0x87},{"i64.shr_u",0x88},{"i64.rotl",0x89},{"i64.rotr",0x8A},
    {"f32.abs",0x8B},{"f32.neg",0x8C},{"f32.ceil",0x8D},{"f32.floor",0x8E},
    {"f32.trunc",0x8F},{"f32.nearest",0x90},{"f32.sqrt",0x91},
    {"f32.add",0x92},{"f32.sub",0x93},{"f32.mul",0x94},{"f32.div",0x95},
    {"f32.min",0x96},{"f32.max",0x97},{"f32.copysign",0x98},
    {"f64.abs",0x99},{"f64.neg",0x9A},{"f64.ceil",0x9B},{"f64.floor",0x9C},
    {"f64.trunc",0x9D},{"f64.nearest",0x9E},{"f64.sqrt",0x9F},
    {"f64.add",0xA0},{"f64.sub",0xA1},{"f64.mul",0xA2},{"f64.div",0xA3},
    {"f64.min",0xA4},{"f64.max",0xA5},{"f64.copysign",0xA6},
    {"i32.wrap_i64",0xA7},{"i32.trunc_f32_s",0xA8},{"i32.trunc_f32_u",0xA9},
    {"i32.trunc_f64_s",0xAA},{"i32.trunc_f64_u",0xAB},
    {"i64.extend_i32_s",0xAC},{"i64.extend_i32_u",0xAD},
    {"i64.trunc_f32_s",0xAE},{"i64.trunc_f32_u",0xAF},
    {"i64.trunc_f64_s",0xB0},{"i64.trunc_f64_u",0xB1},
    {"f32.convert_i32_s",0xB2},{"f32.convert_i32_u",0xB3},
    {"f32.convert_i64_s",0xB4},{"f32.convert_i64_u",0xB5},{"f32.demote_f64",0xB6},
    {"f64.convert_i32_s",0xB7},{"f64.convert_i32_u",0xB8},
    {"f64.convert_i64_s",0xB9},{"f64.convert_i64_u",0xBA},{"f64.promote_f32",0xBB},
    {"i32.reinterpret_f32",0xBC},{"i64.reinterpret_f64",0xBD},
    {"f32.reinterpret_i32",0xBE},{"f64.reinterpret_i64",0xBF},
};

static int lookup_opcode(const char *name) {
    for (size_t i = 0; i < sizeof(MNEMONICS) / sizeof(MNEMONICS[0]); i++) {
        if (!strcmp(MNEMONICS[i].name, name)) return MNEMONICS[i].op;
    }
    return -1;
}

static int valtype_from_str(const char *s) {
    if (!strcmp(s, "i32")) return type_i32;
    if (!strcmp(s, "i64")) return type_i64;
    if (!strcmp(s, "f32")) return type_f32;
    if (!strcmp(s, "f64")) return type_f64;
    return -1;
}

/* ------------------------------------------------------------------ */
/* 函数类型签名                                                        */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t *params; size_t nparams;
    uint8_t *results; size_t nresults;
} TypeSig;

static bool sig_eq(const TypeSig *a, const TypeSig *b) {
    if (a->nparams != b->nparams || a->nresults != b->nresults) return false;
    for (size_t i = 0; i < a->nparams; i++) if (a->params[i] != b->params[i]) return false;
    for (size_t i = 0; i < a->nresults; i++) if (a->results[i] != b->results[i]) return false;
    return true;
}

/* ------------------------------------------------------------------ */
/* 汇编上下文                                                          */
/* ------------------------------------------------------------------ */
typedef struct {
    /* 收集到的顶层 form（SExpr 引用，不拥有） */
    SExpr **types; size_t ntypes;
    SExpr **imports; size_t nimports;
    SExpr **funcs; size_t nfuncs;      /* 定义的函数 */
    SExpr **globals; size_t nglobals;  /* 定义的全局 */
    SExpr **exports; size_t nexports;
    SExpr **elems; size_t nelems;
    SExpr **datas; size_t ndatas;
    SExpr *memory;      /* 定义的 memory，NULL 表示无 */
    SExpr *table;       /* 定义的 table，NULL 表示无 */
    SExpr *start;

    /* import 计数 */
    size_t imp_func, imp_global, imp_table, imp_mem;

    /* 类型表（索引即 type index） */
    TypeSig *types_sig; size_t ntypes_sig;
    char **type_names;

    /* 函数名（总函数数，imports 在前） */
    char **func_names; size_t nfunc_total;
    /* 全局名（总全局数，imports 在前） */
    char **global_names; size_t nglobal_total;

    /* 每个函数的 type index（imports 在前，定义函数随后） */
    int32_t *func_typeidx; size_t nfunc_typeidx;
} Asm;

/* 每个函数体发射时的上下文 */
typedef struct {
    char **locals; size_t nlocals;   /* local 名（含参数，按索引） */
    char **labels; size_t nlabels;   /* 标签栈，底部为函数隐式标签 */
} FnCtx;

static void fc_push_name(char ***arr, size_t *n, const char *name) {
    *arr = (char **)realloc(*arr, (*n + 1) * sizeof(char *));
    (*arr)[*n] = name ? xstrdup(name) : NULL;
    (*n)++;
}

static void fc_pop(char **arr, size_t *n) {
    if (*n == 0) return;
    (*n)--;
    free(arr[*n]);
}

static int32_t resolve_name(char **names, size_t count, const char *ref) {
    if (!ref || !*ref) return -1;
    if (ref[0] == '$') {
        for (size_t i = 0; i < count; i++)
            if (names[i] && !strcmp(names[i], ref + 1)) return (int32_t)i;
        return -1;
    }
    char *end;
    long v = strtol(ref, &end, 10);
    if (*ref && *end == 0 && v >= 0) return (int32_t)v;
    return -1;
}

/* 标签深度解析 */
static bool resolve_label_depth(FnCtx *fc, const char *ref, uint32_t *depth) {
    if (ref[0] == '$') {
        for (size_t i = fc->nlabels; i > 0; i--) {
            if (fc->labels[i - 1] && !strcmp(fc->labels[i - 1], ref + 1)) {
                *depth = (uint32_t)(fc->nlabels - i);
                return true;
            }
        }
        return false;
    }
    char *end;
    long v = strtol(ref, &end, 10);
    if (*ref && *end == 0 && v >= 0) { *depth = (uint32_t)v; return true; }
    return false;
}

/* ------------------------------------------------------------------ */
/* 数值解析                                                            */
/* ------------------------------------------------------------------ */
static bool parse_int_lit(const char *s, int64_t *out) {
    bool neg = false;
    if (*s == '+' || *s == '-') { neg = (*s == '-'); s++; }
    int base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
    if (!*s) return false;
    uint64_t v = 0;
    for (; *s; s++) {
        if (*s == '_') continue;
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else return false;
        if (d >= base) return false;
        v = v * (uint64_t)base + (uint64_t)d;
    }
    *out = neg ? -(int64_t)v : (int64_t)v;
    return true;
}

static double parse_float_lit(const char *s) {
    if (!strncmp(s, "nan", 3) && (s[3] == ':' || s[3] == 0)) return NAN;
    if (!strcmp(s, "inf") || !strcmp(s, "+inf") || !strcmp(s, "infinity")) return INFINITY;
    if (!strcmp(s, "-inf") || !strcmp(s, "-infinity")) return -INFINITY;
    char *end;
    double d = strtod(s, &end);
    return d;
}

/* ------------------------------------------------------------------ */
/* 指令发射                                                            */
/* ------------------------------------------------------------------ */

/* 常量指令：emit opcode + immediate */
static bool emit_const(ByteBuf *code, const char *m, const char *v) {
    if (!strcmp(m, "i32.const")) {
        int64_t x; if (!parse_int_lit(v, &x)) { wat_error("bad i32.const literal '%s'", v); return false; }
        uint32_t w = (uint32_t)(uint64_t)x;
        bb_put(code, 0x41); bb_put_leb_s32(code, (int32_t)w); return true;
    }
    if (!strcmp(m, "i64.const")) {
        int64_t x; if (!parse_int_lit(v, &x)) { wat_error("bad i64.const literal '%s'", v); return false; }
        bb_put(code, 0x42); bb_put_leb_s64(code, x); return true;
    }
    if (!strcmp(m, "f32.const")) {
        bb_put(code, 0x43); bb_put_f32(code, (float)parse_float_lit(v)); return true;
    }
    if (!strcmp(m, "f64.const")) {
        bb_put(code, 0x44); bb_put_f64(code, parse_float_lit(v)); return true;
    }
    return false;
}

static bool is_const_mnemonic(const char *m) {
    return !strcmp(m, "i32.const") || !strcmp(m, "i64.const") ||
           !strcmp(m, "f32.const") || !strcmp(m, "f64.const");
}

/* 解析 memarg（align/offset 关键字，返回 offset） */
static uint32_t parse_memarg(SExpr **kids, size_t nkids, size_t *consumed) {
    uint32_t offset = 0;
    *consumed = 0;
    for (size_t i = 0; i < nkids; i++) {
        SExpr *k = kids[i];
        if (k->kind == SX_ATOM && !strncmp(k->text, "offset=", 7)) {
            int64_t v; if (parse_int_lit(k->text + 7, &v)) offset = (uint32_t)v;
            (*consumed)++;
        } else if (k->kind == SX_ATOM && !strncmp(k->text, "align=", 6)) {
            (*consumed)++; /* VM 忽略 align */
        } else {
            break;
        }
    }
    return offset;
}

enum { R_OK = 0, R_END = 1, R_ELSE = 2, R_ERR = 3 };
static int emit_seq(Asm *a, ByteBuf *code, SExpr **elems, size_t n, size_t *i, FnCtx *fc);
static bool emit_folded(Asm *a, ByteBuf *code, SExpr *sx, FnCtx *fc);
static bool emit_expr(Asm *a, ByteBuf *code, SExpr *sx, FnCtx *fc);

/* 发射一个指令（折叠形式：list 的头是助记符，其余是操作数） */
static bool emit_folded(Asm *a, ByteBuf *code, SExpr *sx, FnCtx *fc) {
    const char *m = sx->kids[0]->text;
    size_t n = sx->nkids - 1;
    SExpr **op = sx->kids + 1;

    if (!strcmp(m, "block") || !strcmp(m, "loop")) {
        /* (block $l? (result T)? body...) */
        size_t i = 0;
        const char *label = NULL;
        uint8_t bt = 0x40;
        if (i < n && op[i]->kind == SX_ATOM && op[i]->text[0] == '$') { label = op[i]->text + 1; i++; }
        if (i < n && op[i]->kind == SX_LIST && !strcmp(op[i]->kids[0]->text, "result")) {
            SExpr *r = op[i];
            if (r->nkids >= 2 && r->kids[1]->kind == SX_ATOM) {
                int t = valtype_from_str(r->kids[1]->text);
                if (t < 0) { wat_error("bad block result type"); return false; }
                bt = (uint8_t)t;
            }
            i++;
        }
        fc_push_name(&fc->labels, &fc->nlabels, label);
        bb_put(code, !strcmp(m, "block") ? 0x02 : 0x03);
        bb_put(code, bt);
        if (emit_seq(a, code, op, n, &i, fc) != R_OK) return false;
        bb_put(code, 0x0B); /* end */
        fc_pop(fc->labels, &fc->nlabels);
        return true;
    }

    if (!strcmp(m, "if")) {
        size_t i = 0;
        const char *label = NULL;
        uint8_t bt = 0x40;
        if (i < n && op[i]->kind == SX_ATOM && op[i]->text[0] == '$') { label = op[i]->text + 1; i++; }
        if (i < n && op[i]->kind == SX_LIST && !strcmp(op[i]->kids[0]->text, "result")) {
            SExpr *r = op[i];
            if (r->nkids >= 2 && r->kids[1]->kind == SX_ATOM) {
                int t = valtype_from_str(r->kids[1]->text);
                if (t < 0) { wat_error("bad if result type"); return false; }
                bt = (uint8_t)t;
            }
            i++;
        }
        if (i >= n) { wat_error("if missing condition"); return false; }
        fc_push_name(&fc->labels, &fc->nlabels, label);
        /* condition（单表达式：折叠/原子） */
        if (!emit_expr(a, code, op[i], fc)) return false;
        i++;
        bb_put(code, 0x04); bb_put(code, bt);
        /* then 体 */
        if (i < n && op[i]->kind == SX_LIST && !strcmp(op[i]->kids[0]->text, "then")) {
            SExpr *tl = op[i]; size_t ti = 1;
            if (emit_seq(a, code, tl->kids, tl->nkids, &ti, fc) != R_OK) return false;
            i++;
        } else {
            /* 无 (then)：then 体为 [i .. else/end) */
            size_t then_end = i;
            while (then_end < n && !(op[then_end]->kind == SX_LIST && !strcmp(op[then_end]->kids[0]->text, "else"))) then_end++;
            size_t ti = i;
            if (emit_seq(a, code, op, then_end, &ti, fc) != R_OK) return false;
            i = then_end;
        }
        /* else 体 */
        if (i < n && op[i]->kind == SX_LIST && !strcmp(op[i]->kids[0]->text, "else")) {
            bb_put(code, 0x05);
            SExpr *el = op[i]; size_t ei = 1;
            if (emit_seq(a, code, el->kids, el->nkids, &ei, fc) != R_OK) return false;
            i++;
        }
        bb_put(code, 0x0B); /* end */
        fc_pop(fc->labels, &fc->nlabels);
        return true;
    }

    if (is_const_mnemonic(m)) {
        if (n < 1 || op[0]->kind != SX_ATOM) { wat_error("%s needs a literal", m); return false; }
        return emit_const(code, m, op[0]->text);
    }

    if (!strcmp(m, "local.get") || !strcmp(m, "local.set") || !strcmp(m, "local.tee")) {
        if (n < 1 || op[0]->kind != SX_ATOM) { wat_error("%s needs index", m); return false; }
        if (!strcmp(m, "local.set") || !strcmp(m, "local.tee")) {
            if (n < 2) { wat_error("%s needs value", m); return false; }
            if (!emit_expr(a, code, op[1], fc)) return false;
        }
        int32_t idx = resolve_name(fc->locals, fc->nlocals, op[0]->text);
        if (idx < 0) { wat_error("unknown local '%s'", op[0]->text); return false; }
        bb_put(code, (uint8_t)lookup_opcode(m));
        bb_put_leb_u32(code, (uint32_t)idx);
        return true;
    }

    if (!strcmp(m, "global.get") || !strcmp(m, "global.set")) {
        if (n < 1 || op[0]->kind != SX_ATOM) { wat_error("%s needs index", m); return false; }
        if (!strcmp(m, "global.set")) {
            if (n < 2) { wat_error("global.set needs value"); return false; }
            if (!emit_expr(a, code, op[1], fc)) return false;
        }
        int32_t idx = resolve_name(a->global_names, a->nglobal_total, op[0]->text);
        if (idx < 0) { wat_error("unknown global '%s'", op[0]->text); return false; }
        bb_put(code, (uint8_t)lookup_opcode(m));
        bb_put_leb_u32(code, (uint32_t)idx);
        return true;
    }

    if (!strcmp(m, "call")) {
        if (n < 1 || op[0]->kind != SX_ATOM) { wat_error("call needs function"); return false; }
        for (size_t i = 1; i < n; i++) if (!emit_expr(a, code, op[i], fc)) return false;
        int32_t idx = resolve_name(a->func_names, a->nfunc_total, op[0]->text);
        if (idx < 0) { wat_error("unknown function '%s'", op[0]->text); return false; }
        bb_put(code, 0x10);
        bb_put_leb_u32(code, (uint32_t)idx);
        return true;
    }

    if (!strcmp(m, "call_indirect")) {
        size_t i = 0;
        int32_t typeidx = 0;
        if (i < n && op[i]->kind == SX_LIST && !strcmp(op[i]->kids[0]->text, "type")) {
            if (op[i]->nkids < 2 || op[i]->kids[1]->kind != SX_ATOM) { wat_error("call_indirect bad type"); return false; }
            typeidx = resolve_name(a->type_names, a->ntypes_sig, op[i]->kids[1]->text);
            if (typeidx < 0) { wat_error("unknown type '%s'", op[i]->kids[1]->text); return false; }
            i++;
        }
        /* 其余为 [args..., index]，index 在最后 */
        if (i >= n) { wat_error("call_indirect needs index"); return false; }
        for (size_t j = i; j + 1 < n; j++) if (!emit_expr(a, code, op[j], fc)) return false;
        if (!emit_expr(a, code, op[n - 1], fc)) return false; /* index */
        bb_put(code, 0x11);
        bb_put_leb_u32(code, (uint32_t)typeidx);
        bb_put(code, 0x00); /* reserved */
        return true;
    }

    if (!strcmp(m, "memory.size") || !strcmp(m, "memory.grow")) {
        if (!strcmp(m, "memory.grow")) {
            if (n < 1) { wat_error("memory.grow needs value"); return false; }
            if (!emit_expr(a, code, op[0], fc)) return false;
        }
        bb_put(code, (uint8_t)lookup_opcode(m));
        bb_put(code, 0x00); /* 内存索引（MVP 恒 0） */
        return true;
    }

    if (!strcmp(m, "br") || !strcmp(m, "br_if")) {
        if (n < 1 || op[0]->kind != SX_ATOM) { wat_error("%s needs label", m); return false; }
        if (!strcmp(m, "br_if")) {
            if (n < 2) { wat_error("br_if needs condition"); return false; }
            if (!emit_expr(a, code, op[1], fc)) return false;
        }
        uint32_t depth;
        if (!resolve_label_depth(fc, op[0]->text, &depth)) { wat_error("unknown label '%s'", op[0]->text); return false; }
        bb_put(code, (uint8_t)lookup_opcode(m));
        bb_put_leb_u32(code, depth);
        return true;
    }

    if (!strcmp(m, "br_table")) {
        /* (br_table $l0 $l1 ... $default (idx)) —— 最后一个操作数是索引表达式 */
        if (n < 2) { wat_error("br_table needs labels + index"); return false; }
        if (!emit_expr(a, code, op[n - 1], fc)) return false; /* index */
        size_t nlabels = n - 1;
        bb_put(code, 0x0E);
        bb_put_leb_u32(code, (uint32_t)(nlabels - 1)); /* count */
        for (size_t j = 0; j < nlabels; j++) {
            if (op[j]->kind != SX_ATOM) { wat_error("br_table target must be label"); return false; }
            uint32_t depth;
            if (!resolve_label_depth(fc, op[j]->text, &depth)) { wat_error("unknown label '%s'", op[j]->text); return false; }
            if (depth > 0xFF) { wat_error("br_table depth > 255"); return false; }
            bb_put(code, (uint8_t)depth); /* VM 怪癖：目标是单字节 */
        }
        return true;
    }

    if (!strcmp(m, "select")) {
        for (size_t i = 1; i <= n && i <= 3; i++) if (!emit_expr(a, code, op[i - 1], fc)) return false;
        bb_put(code, 0x1B);
        return true;
    }

    if (!strcmp(m, "drop")) {
        if (n < 1) { wat_error("drop needs operand"); return false; }
        if (!emit_expr(a, code, op[0], fc)) return false;
        bb_put(code, 0x1A);
        return true;
    }

    /* 内存 load/store */
    {
        int opc = lookup_opcode(m);
        if (opc >= 0x28 && opc <= 0x3E) {
            size_t consumed;
            uint32_t offset = parse_memarg(op, n, &consumed);
            if (opc >= 0x36) { /* store：先地址后值 */
                if (consumed >= n) { wat_error("%s missing operands", m); return false; }
                if (!emit_expr(a, code, op[consumed], fc)) return false;
                if (consumed + 1 >= n) { wat_error("%s missing value", m); return false; }
                if (!emit_expr(a, code, op[consumed + 1], fc)) return false;
            } else {
                if (consumed >= n) { wat_error("%s missing address", m); return false; }
                if (!emit_expr(a, code, op[consumed], fc)) return false;
            }
            bb_put(code, (uint8_t)opc);
            bb_put_leb_u32(code, 0); /* align（VM 忽略） */
            bb_put_leb_u32(code, offset);
            return true;
        }
    }

    /* 其余指令：先发射折叠的操作数，再发射 opcode */
    int opc = lookup_opcode(m);
    if (opc < 0) { wat_error("unknown instruction '%s'", m); return false; }
    for (size_t i = 0; i < n; i++) {
        if (!emit_expr(a, code, op[i], fc)) return false;
    }
    bb_put(code, (uint8_t)opc);
    return true;
}

/* 发射任意一个 SExpr 指令（ATOM 走平铺零立即数/常量，LIST 走折叠） */
static bool emit_expr(Asm *a, ByteBuf *code, SExpr *sx, FnCtx *fc) {
    if (sx->kind == SX_ATOM) {
        const char *m = sx->text;
        int opc = lookup_opcode(m);
        if (opc < 0) { wat_error("unknown instruction '%s'", m); return false; }
        bb_put(code, (uint8_t)opc);
        return true;
    }
    return emit_folded(a, code, sx, fc);
}

/* 发射函数体的指令序列（支持平铺 + 折叠） */
static int emit_seq(Asm *a, ByteBuf *code, SExpr **elems, size_t n, size_t *i, FnCtx *fc) {
    while (*i < n) {
        SExpr *e = elems[*i];
        if (e->kind == SX_ATOM) {
            const char *m = e->text;
            if (!strcmp(m, "end")) { (*i)++; return R_END; }
            if (!strcmp(m, "else")) { (*i)++; return R_ELSE; }
            if (!strcmp(m, "block") || !strcmp(m, "loop")) {
                (*i)++;
                const char *label = NULL;
                if (*i < n && elems[*i]->kind == SX_ATOM && elems[*i]->text[0] == '$') { label = elems[*i]->text + 1; (*i)++; }
                fc_push_name(&fc->labels, &fc->nlabels, label);
                bb_put(code, !strcmp(m, "block") ? 0x02 : 0x03);
                bb_put(code, 0x40);
                int r = emit_seq(a, code, elems, n, i, fc);
                bb_put(code, 0x0B);
                fc_pop(fc->labels, &fc->nlabels);
                if (r == R_ELSE || r == R_ERR) return R_ERR;
                continue;
            }
            if (!strcmp(m, "if")) {
                (*i)++;
                const char *label = NULL;
                if (*i < n && elems[*i]->kind == SX_ATOM && elems[*i]->text[0] == '$') { label = elems[*i]->text + 1; (*i)++; }
                fc_push_name(&fc->labels, &fc->nlabels, label);
                bb_put(code, 0x04); bb_put(code, 0x40);
                int r = emit_seq(a, code, elems, n, i, fc);
                if (r == R_ELSE) {
                    bb_put(code, 0x05);
                    r = emit_seq(a, code, elems, n, i, fc);
                }
                bb_put(code, 0x0B);
                fc_pop(fc->labels, &fc->nlabels);
                if (r == R_ELSE || r == R_ERR) return R_ERR;
                continue;
            }
            /* 平铺的常量：mnemonic 后跟一个立即数原子 */
            if (is_const_mnemonic(m)) {
                if (*i + 1 >= n || elems[*i + 1]->kind != SX_ATOM) { wat_error("%s needs literal", m); return R_ERR; }
                (*i)++;
                if (!emit_const(code, m, elems[*i]->text)) return R_ERR;
                (*i)++;
                continue;
            }
            /* 平铺的带索引指令：后跟一个名字/编号原子 */
            if (!strcmp(m, "local.get") || !strcmp(m, "local.set") || !strcmp(m, "local.tee") ||
                !strcmp(m, "global.get") || !strcmp(m, "global.set") ||
                !strcmp(m, "call") || !strcmp(m, "br") || !strcmp(m, "br_if")) {
                (*i)++;
                if (*i >= n || elems[*i]->kind != SX_ATOM) { wat_error("%s needs operand", m); return R_ERR; }
                const char *ref = elems[*i]->text;
                int32_t idx;
                uint32_t depth;
                if (!strcmp(m, "br") || !strcmp(m, "br_if")) {
                    if (!resolve_label_depth(fc, ref, &depth)) { wat_error("unknown label '%s'", ref); return R_ERR; }
                    bb_put(code, (uint8_t)lookup_opcode(m));
                    bb_put_leb_u32(code, depth);
                } else if (!strcmp(m, "call")) {
                    idx = resolve_name(a->func_names, a->nfunc_total, ref);
                    if (idx < 0) { wat_error("unknown function '%s'", ref); return R_ERR; }
                    bb_put(code, 0x10); bb_put_leb_u32(code, (uint32_t)idx);
                } else if (!strcmp(m, "local.get") || !strcmp(m, "local.set") || !strcmp(m, "local.tee")) {
                    idx = resolve_name(fc->locals, fc->nlocals, ref);
                    if (idx < 0) { wat_error("unknown local '%s'", ref); return R_ERR; }
                    bb_put(code, (uint8_t)lookup_opcode(m)); bb_put_leb_u32(code, (uint32_t)idx);
                } else {
                    idx = resolve_name(a->global_names, a->nglobal_total, ref);
                    if (idx < 0) { wat_error("unknown global '%s'", ref); return R_ERR; }
                    bb_put(code, (uint8_t)lookup_opcode(m)); bb_put_leb_u32(code, (uint32_t)idx);
                }
                (*i)++;
                continue;
            }
            /* 平铺 memory.size / memory.grow：需要后跟 0x00 立即数 */
            if (!strcmp(m, "memory.size") || !strcmp(m, "memory.grow")) {
                bb_put(code, (uint8_t)lookup_opcode(m));
                bb_put(code, 0x00);
                (*i)++;
                continue;
            }
            /* 平铺 call_indirect：可选 (type $t)，后跟 typeidx + 0x00 */
            if (!strcmp(m, "call_indirect")) {
                (*i)++;
                int32_t typeidx = 0;
                if (*i < n && elems[*i]->kind == SX_LIST && !strcmp(elems[*i]->kids[0]->text, "type")) {
                    SExpr *t = elems[*i];
                    if (t->nkids >= 2 && t->kids[1]->kind == SX_ATOM) {
                        typeidx = resolve_name(a->type_names, a->ntypes_sig, t->kids[1]->text);
                        if (typeidx < 0) { wat_error("unknown type '%s'", t->kids[1]->text); return R_ERR; }
                    }
                    (*i)++;
                }
                bb_put(code, 0x11);
                bb_put_leb_u32(code, (uint32_t)typeidx);
                bb_put(code, 0x00);
                continue;
            }
            /* 平铺 br_table：后跟若干个 $label，最后一个为 default */
            if (!strcmp(m, "br_table")) {
                (*i)++;
                size_t start = *i;
                while (*i < n && elems[*i]->kind == SX_ATOM && elems[*i]->text[0] == '$') (*i)++;
                size_t nlabels = *i - start;
                if (nlabels < 1) { wat_error("br_table needs labels"); return R_ERR; }
                bb_put(code, 0x0E);
                bb_put_leb_u32(code, (uint32_t)(nlabels - 1)); /* count */
                for (size_t k = start; k < *i; k++) {
                    uint32_t depth;
                    if (!resolve_label_depth(fc, elems[k]->text, &depth)) { wat_error("unknown label '%s'", elems[k]->text); return R_ERR; }
                    if (depth > 0xFF) { wat_error("br_table depth > 255"); return R_ERR; }
                    bb_put(code, (uint8_t)depth);
                }
                continue;
            }
            /* 其余零立即数指令（含 load/store 的平铺需要 align/offset） */
            int opc = lookup_opcode(m);
            if (opc < 0) { wat_error("unknown instruction '%s'", m); return R_ERR; }
            if (opc >= 0x28 && opc <= 0x3E) {
                /* 内存 load/store 平铺：可选 offset=/align= 原子 */
                (*i)++;
                uint32_t offset = 0;
                while (*i < n && elems[*i]->kind == SX_ATOM &&
                       (!strncmp(elems[*i]->text, "offset=", 7) || !strncmp(elems[*i]->text, "align=", 6))) {
                    if (!strncmp(elems[*i]->text, "offset=", 7)) {
                        int64_t v; if (parse_int_lit(elems[*i]->text + 7, &v)) offset = (uint32_t)v;
                    }
                    (*i)++;
                }
                bb_put(code, (uint8_t)opc);
                bb_put_leb_u32(code, 0); /* align（VM 忽略） */
                bb_put_leb_u32(code, offset);
                continue;
            }
            bb_put(code, (uint8_t)opc);
            (*i)++;
            continue;
        }
        /* LIST：折叠指令 */
        if (!emit_folded(a, code, e, fc)) return R_ERR;
        (*i)++;
    }
    return R_OK;
}

/* ------------------------------------------------------------------ */
/* section 组装                                                        */
/* ------------------------------------------------------------------ */
static void put_section(ByteBuf *out, uint8_t id, const ByteBuf *payload) {
    bb_put(out, id);
    bb_put_leb_u32(out, (uint32_t)payload->len);
    bb_put_bytes(out, payload->data, payload->len);
}

/* 解析 (type $t (func ...)) 为 TypeSig */
static bool parse_type_form(SExpr *form, TypeSig *sig) {
    memset(sig, 0, sizeof(*sig));
    for (size_t i = 1; i < form->nkids; i++) {
        SExpr *c = form->kids[i];
        if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "func")) {
            for (size_t j = 1; j < c->nkids; j++) {
                SExpr *p = c->kids[j];
                if (p->kind == SX_LIST && !strcmp(p->kids[0]->text, "param")) {
                    for (size_t k = 1; k < p->nkids; k++) {
                        if (p->kids[k]->kind == SX_ATOM) {
                            int t = valtype_from_str(p->kids[k]->text);
                            if (t >= 0) { sig->params = realloc(sig->params, sig->nparams + 1); sig->params[sig->nparams++] = (uint8_t)t; }
                        }
                    }
                } else if (p->kind == SX_LIST && !strcmp(p->kids[0]->text, "result")) {
                    for (size_t k = 1; k < p->nkids; k++) {
                        if (p->kids[k]->kind == SX_ATOM) {
                            int t = valtype_from_str(p->kids[k]->text);
                            if (t >= 0) { sig->results = realloc(sig->results, sig->nresults + 1); sig->results[sig->nresults++] = (uint8_t)t; }
                        }
                    }
                }
            }
            return true;
        }
    }
    return false;
}

/* 查找或新增一个类型签名，返回 type index */
static int32_t find_or_add_type(Asm *a, const TypeSig *sig) {
    for (size_t i = 0; i < a->ntypes_sig; i++) {
        if (sig_eq(&a->types_sig[i], sig)) return (int32_t)i;
    }
    TypeSig *ts = realloc(a->types_sig, (a->ntypes_sig + 1) * sizeof(TypeSig));
    a->types_sig = ts;
    a->types_sig[a->ntypes_sig] = *sig; /* 浅拷贝，所有权交给表 */
    a->type_names = realloc(a->type_names, (a->ntypes_sig + 1) * sizeof(char *));
    a->type_names[a->ntypes_sig] = NULL;
    return (int32_t)(a->ntypes_sig++);
}

/* 解析函数描述（param/result/(type $t)），注册并返回其 type index */
static int32_t register_func_type(Asm *a, SExpr *form, size_t start) {
    int32_t type_use = -1;
    TypeSig sig; memset(&sig, 0, sizeof(sig));
    bool has_inline = false;
    for (size_t j = start; j < form->nkids; j++) {
        SExpr *c = form->kids[j];
        if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "type")) {
            if (c->nkids >= 2 && c->kids[1]->kind == SX_ATOM)
                type_use = resolve_name(a->type_names, a->ntypes_sig, c->kids[1]->text);
        } else if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "param")) {
            for (size_t k = 1; k < c->nkids; k++) {
                if (c->kids[k]->kind == SX_ATOM) {
                    int t = valtype_from_str(c->kids[k]->text);
                    if (t >= 0) { sig.params = realloc(sig.params, sig.nparams + 1); sig.params[sig.nparams++] = (uint8_t)t; has_inline = true; }
                }
            }
        } else if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "result")) {
            for (size_t k = 1; k < c->nkids; k++) {
                if (c->kids[k]->kind == SX_ATOM) {
                    int t = valtype_from_str(c->kids[k]->text);
                    if (t >= 0) { sig.results = realloc(sig.results, sig.nresults + 1); sig.results[sig.nresults++] = (uint8_t)t; has_inline = true; }
                }
            }
        }
    }
    if (type_use >= 0) {
        free(sig.params); free(sig.results);
        return type_use;
    }
    return find_or_add_type(a, has_inline ? &sig : &(TypeSig){0, 0, 0, 0});
}

/* 统计 form 内的内联 (export "name") 个数 */
static size_t count_inline_exports(SExpr *form) {
    size_t c = 0;
    for (size_t j = 1; j < form->nkids; j++) {
        SExpr *k = form->kids[j];
        if (k->kind == SX_LIST && k->nkids >= 2 && !strcmp(k->kids[0]->text, "export")) c++;
    }
    return c;
}

/* 发射一条 export entry */
static void emit_export_entry(ByteBuf *b, const char *name, size_t namelen, uint8_t kind, uint32_t idx) {
    bb_put_leb_u32(b, (uint32_t)namelen);
    bb_put_bytes(b, (const uint8_t *)name, namelen);
    bb_put(b, kind);
    bb_put_leb_u32(b, idx);
}

/* 主汇编入口 */
static int assemble_module(SExpr *root, ByteBuf *out) {
    Asm a; memset(&a, 0, sizeof(a));
    if (root->kind != SX_LIST || root->nkids < 1 || root->kids[0]->kind != SX_ATOM ||
        strcmp(root->kids[0]->text, "module")) {
        wat_error("expected (module ...)");
        return -1;
    }

    /* ---- 收集顶层 form ---- */
    for (size_t i = 1; i < root->nkids; i++) {
        SExpr *c = root->kids[i];
        if (c->kind != SX_LIST || c->nkids < 1 || c->kids[0]->kind != SX_ATOM) continue;
        const char *kw = c->kids[0]->text;
        if (!strcmp(kw, "type")) { a.types = realloc(a.types, (a.ntypes + 1) * sizeof(SExpr *)); a.types[a.ntypes++] = c; }
        else if (!strcmp(kw, "import")) { a.imports = realloc(a.imports, (a.nimports + 1) * sizeof(SExpr *)); a.imports[a.nimports++] = c; }
        else if (!strcmp(kw, "func")) { a.funcs = realloc(a.funcs, (a.nfuncs + 1) * sizeof(SExpr *)); a.funcs[a.nfuncs++] = c; }
        else if (!strcmp(kw, "global")) { a.globals = realloc(a.globals, (a.nglobals + 1) * sizeof(SExpr *)); a.globals[a.nglobals++] = c; }
        else if (!strcmp(kw, "export")) { a.exports = realloc(a.exports, (a.nexports + 1) * sizeof(SExpr *)); a.exports[a.nexports++] = c; }
        else if (!strcmp(kw, "elem")) { a.elems = realloc(a.elems, (a.nelems + 1) * sizeof(SExpr *)); a.elems[a.nelems++] = c; }
        else if (!strcmp(kw, "data")) { a.datas = realloc(a.datas, (a.ndatas + 1) * sizeof(SExpr *)); a.datas[a.ndatas++] = c; }
        else if (!strcmp(kw, "memory")) { a.memory = c; }
        else if (!strcmp(kw, "table")) { a.table = c; }
        else if (!strcmp(kw, "start")) { a.start = c; }
        else { wat_error("unsupported top-level form '%s'", kw); return -1; }
    }

    /* ---- 解析 type 并建名 ---- */
    for (size_t i = 0; i < a.ntypes; i++) {
        TypeSig sig; memset(&sig, 0, sizeof(sig));
        if (!parse_type_form(a.types[i], &sig)) { wat_error("bad type form"); return -1; }
        int32_t idx = find_or_add_type(&a, &sig);
        /* 命名（$name 紧跟 (type 之后，位于 kids[1]） */
        if (a.types[i]->nkids > 1 && a.types[i]->kids[1]->kind == SX_ATOM &&
            a.types[i]->kids[1]->text[0] == '$') {
            a.type_names[idx] = xstrdup(a.types[i]->kids[1]->text + 1);
        }
    }

    /* ---- import 计数 + 命名 ---- */
    for (size_t i = 0; i < a.nimports; i++) {
        SExpr *imp = a.imports[i];
        if (imp->nkids < 4 || imp->kids[3]->kind != SX_LIST || imp->kids[3]->nkids < 1) {
            wat_error("bad import form"); return -1;
        }
        const char *kind = imp->kids[3]->kids[0]->text;
        if (!strcmp(kind, "func")) a.imp_func++;
        else if (!strcmp(kind, "global")) a.imp_global++;
        else if (!strcmp(kind, "table")) a.imp_table++;
        else if (!strcmp(kind, "memory")) a.imp_mem++;
        else { wat_error("unsupported import kind '%s'", kind); return -1; }
    }

    a.nfunc_total = a.imp_func + a.nfuncs;
    a.nglobal_total = a.imp_global + a.nglobals;
    a.func_names = calloc(a.nfunc_total ? a.nfunc_total : 1, sizeof(char *));
    a.global_names = calloc(a.nglobal_total ? a.nglobal_total : 1, sizeof(char *));

    /* 每个函数（import 在前，定义随后）的 type index */
    a.func_typeidx = calloc(a.nfunc_total ? a.nfunc_total : 1, sizeof(int32_t));
    a.nfunc_typeidx = a.nfunc_total;

    /* 注册导入函数的类型 + 名字，导入全局的名字 */
    size_t fi = 0, gi = 0;
    for (size_t i = 0; i < a.nimports; i++) {
        SExpr *imp = a.imports[i];
        const char *kind = imp->kids[3]->kids[0]->text;
        if (!strcmp(kind, "func")) {
            SExpr *desc = imp->kids[3];
            a.func_typeidx[fi] = register_func_type(&a, desc, 1);
            if (desc->nkids > 1 && desc->kids[1]->kind == SX_ATOM && desc->kids[1]->text[0] == '$')
                a.func_names[fi] = xstrdup(desc->kids[1]->text + 1);
            fi++;
        } else if (!strcmp(kind, "global")) {
            SExpr *desc = imp->kids[3];
            if (desc->nkids > 1 && desc->kids[1]->kind == SX_ATOM && desc->kids[1]->text[0] == '$')
                a.global_names[gi] = xstrdup(desc->kids[1]->text + 1);
            gi++;
        }
    }

    /* 注册定义函数的类型 + 名字（$name 紧跟 (func 之后，位于 kids[1]） */
    for (size_t i = 0; i < a.nfuncs; i++) {
        SExpr *f = a.funcs[i];
        const char *fname = NULL;
        if (f->nkids > 1 && f->kids[1]->kind == SX_ATOM && f->kids[1]->text[0] == '$')
            fname = f->kids[1]->text + 1;
        a.func_typeidx[a.imp_func + i] = register_func_type(&a, f, 1);
        a.func_names[a.imp_func + i] = fname ? xstrdup(fname) : NULL;
    }

    /* 定义全局的名字（$name 紧跟 (global 之后，位于 kids[1]） */
    for (size_t i = 0; i < a.nglobals; i++) {
        SExpr *g = a.globals[i];
        if (g->nkids > 1 && g->kids[1]->kind == SX_ATOM && g->kids[1]->text[0] == '$')
            a.global_names[a.imp_global + i] = xstrdup(g->kids[1]->text + 1);
    }

    /* ---- 发射 header ---- */
    bb_put(out, 0x00); bb_put(out, 0x61); bb_put(out, 0x73); bb_put(out, 0x6d);
    bb_put_u32le(out, 1);

    /* ---- type section ---- */
    {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)a.ntypes_sig);
        for (size_t i = 0; i < a.ntypes_sig; i++) {
            bb_put(&b, 0x60);
            bb_put_leb_u32(&b, (uint32_t)a.types_sig[i].nparams);
            for (size_t j = 0; j < a.types_sig[i].nparams; j++) bb_put(&b, a.types_sig[i].params[j]);
            bb_put_leb_u32(&b, (uint32_t)a.types_sig[i].nresults);
            for (size_t j = 0; j < a.types_sig[i].nresults; j++) bb_put(&b, a.types_sig[i].results[j]);
        }
        put_section(out, 1, &b);
        bb_free(&b);
    }

    /* ---- import section ---- */
    if (a.nimports > 0) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)a.nimports);
        size_t fi = 0;
        for (size_t i = 0; i < a.nimports; i++) {
            SExpr *imp = a.imports[i];
            /* module 名 */
            if (imp->kids[1]->kind != SX_ATOM || imp->kids[2]->kind != SX_ATOM) { wat_error("bad import names"); return -1; }
            bb_put_leb_u32(&b, (uint32_t)imp->kids[1]->len);
            bb_put_bytes(&b, (uint8_t *)imp->kids[1]->text, imp->kids[1]->len);
            bb_put_leb_u32(&b, (uint32_t)imp->kids[2]->len);
            bb_put_bytes(&b, (uint8_t *)imp->kids[2]->text, imp->kids[2]->len);
            SExpr *desc = imp->kids[3];
            const char *kind = desc->kids[0]->text;
            if (!strcmp(kind, "func")) {
                bb_put(&b, 0x00);
                bb_put_leb_u32(&b, (uint32_t)a.func_typeidx[fi]);
                fi++;
            } else if (!strcmp(kind, "table")) {
                bb_put(&b, 0x01);
                /* (table $t? min max? funcref) 取 min/max */
                uint32_t mn = 0, mx = 0; bool has_max = false;
                for (size_t j = 1; j < desc->nkids; j++) {
                    SExpr *c = desc->kids[j];
                    if (c->kind == SX_ATOM && c->text[0] != '$') {
                        if (!strcmp(c->text, "funcref")) continue;
                        int64_t v; if (parse_int_lit(c->text, &v)) { if (!has_max && mn == 0 && j == 1) { mn = (uint32_t)v; } else { mx = (uint32_t)v; has_max = true; } }
                    }
                }
                bb_put(&b, 0x70);
                bb_put(&b, has_max ? 0x01 : 0x00);
                bb_put_leb_u32(&b, mn);
                if (has_max) bb_put_leb_u32(&b, mx);
            } else if (!strcmp(kind, "memory")) {
                bb_put(&b, 0x02);
                uint32_t mn = 0, mx = 0; bool has_max = false; size_t nums = 0;
                for (size_t j = 1; j < desc->nkids; j++) {
                    SExpr *c = desc->kids[j];
                    if (c->kind == SX_ATOM && c->text[0] != '$') {
                        int64_t v; if (parse_int_lit(c->text, &v)) { if (nums == 0) mn = (uint32_t)v; else { mx = (uint32_t)v; has_max = true; } nums++; }
                    }
                }
                bb_put(&b, has_max ? 0x01 : 0x00);
                bb_put_leb_u32(&b, mn);
                if (has_max) bb_put_leb_u32(&b, mx);
            } else if (!strcmp(kind, "global")) {
                bb_put(&b, 0x03);
                /* (global $g? (mut T)? ) 类型 + mutability */
                uint8_t ty = 0x7F; bool mut = false;
                for (size_t j = 1; j < desc->nkids; j++) {
                    SExpr *c = desc->kids[j];
                    if (c->kind == SX_ATOM) { int t = valtype_from_str(c->text); if (t >= 0) ty = (uint8_t)t; }
                    else if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "mut")) {
                        mut = true;
                        if (c->nkids >= 2 && c->kids[1]->kind == SX_ATOM) { int t = valtype_from_str(c->kids[1]->text); if (t >= 0) ty = (uint8_t)t; }
                    }
                }
                bb_put(&b, ty); bb_put(&b, mut ? 0x01 : 0x00);
            } else { wat_error("unsupported import kind"); return -1; }
        }
        put_section(out, 2, &b);
        bb_free(&b);
    }

    /* ---- function section（定义函数各自的 type index） ---- */
    if (a.nfuncs > 0) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)a.nfuncs);
        for (size_t i = 0; i < a.nfuncs; i++) {
            bb_put_leb_u32(&b, (uint32_t)a.func_typeidx[a.imp_func + i]);
        }
        put_section(out, 3, &b);
        bb_free(&b);
    }

    /* ---- table section ---- */
    if (a.table) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, 1);
        uint32_t mn = 0, mx = 0; bool has_max = false; size_t nums = 0;
        for (size_t j = 1; j < a.table->nkids; j++) {
            SExpr *c = a.table->kids[j];
            if (c->kind == SX_ATOM && c->text[0] != '$') {
                if (!strcmp(c->text, "funcref")) continue;
                int64_t v; if (parse_int_lit(c->text, &v)) { if (nums == 0) mn = (uint32_t)v; else { mx = (uint32_t)v; has_max = true; } nums++; }
            }
        }
        bb_put(&b, 0x70); bb_put(&b, has_max ? 0x01 : 0x00); bb_put_leb_u32(&b, mn);
        if (has_max) bb_put_leb_u32(&b, mx);
        put_section(out, 4, &b);
        bb_free(&b);
    }

    /* ---- memory section ---- */
    if (a.memory) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, 1);
        uint32_t mn = 1, mx = 0; bool has_max = false; size_t nums = 0;
        for (size_t j = 1; j < a.memory->nkids; j++) {
            SExpr *c = a.memory->kids[j];
            if (c->kind == SX_ATOM && c->text[0] != '$') {
                int64_t v; if (parse_int_lit(c->text, &v)) { if (nums == 0) mn = (uint32_t)v; else { mx = (uint32_t)v; has_max = true; } nums++; }
            }
        }
        bb_put(&b, has_max ? 0x01 : 0x00); bb_put_leb_u32(&b, mn);
        if (has_max) bb_put_leb_u32(&b, mx);
        put_section(out, 5, &b);
        bb_free(&b);
    }

    /* ---- global section（定义全局） ---- */
    if (a.nglobals > 0) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)a.nglobals);
        for (size_t i = 0; i < a.nglobals; i++) {
            SExpr *g = a.globals[i];
            uint8_t ty = 0x7F; bool mut = false; SExpr *init = NULL;
            for (size_t j = 1; j < g->nkids; j++) {
                SExpr *c = g->kids[j];
                if (c->kind == SX_ATOM && c->text[0] == '$') continue;
                if (c->kind == SX_ATOM) { int t = valtype_from_str(c->text); if (t >= 0) ty = (uint8_t)t; }
                else if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "mut")) {
                    mut = true;
                    if (c->nkids >= 2 && c->kids[1]->kind == SX_ATOM) { int t = valtype_from_str(c->kids[1]->text); if (t >= 0) ty = (uint8_t)t; }
                } else if (c->kind == SX_LIST && c->kids[0]->kind == SX_ATOM &&
                           (is_const_mnemonic(c->kids[0]->text) || !strcmp(c->kids[0]->text, "global.get"))) {
                    init = c;
                }
            }
            bb_put(&b, ty); bb_put(&b, mut ? 0x01 : 0x00);
            if (!init) { wat_error("global missing init"); return -1; }
            if (is_const_mnemonic(init->kids[0]->text)) {
                if (!emit_const(&b, init->kids[0]->text, init->kids[1]->text)) return -1;
            } else {
                bb_put(&b, 0x23);
                int32_t idx = resolve_name(a.global_names, a.nglobal_total, init->kids[1]->text);
                bb_put_leb_u32(&b, idx < 0 ? 0 : (uint32_t)idx);
            }
            bb_put(&b, 0x0B); /* end */
        }
        put_section(out, 6, &b);
        bb_free(&b);
    }

    /* ---- export section ---- */
    size_t total_exports = a.nexports;
    for (size_t i = 0; i < a.nfuncs; i++) total_exports += count_inline_exports(a.funcs[i]);
    for (size_t i = 0; i < a.nglobals; i++) total_exports += count_inline_exports(a.globals[i]);
    if (a.memory) total_exports += count_inline_exports(a.memory);
    if (a.table) total_exports += count_inline_exports(a.table);

    if (total_exports > 0) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)total_exports);
        /* 顶层 (export "name" (kind idx)) */
        for (size_t i = 0; i < a.nexports; i++) {
            SExpr *e = a.exports[i];
            if (e->nkids < 3 || e->kids[1]->kind != SX_ATOM || e->kids[2]->kind != SX_LIST) { wat_error("bad export"); return -1; }
            SExpr *desc = e->kids[2];
            const char *kind = desc->kids[0]->text;
            if (desc->nkids < 2 || desc->kids[1]->kind != SX_ATOM) { wat_error("bad export desc"); return -1; }
            if (!strcmp(kind, "func")) {
                int32_t idx = resolve_name(a.func_names, a.nfunc_total, desc->kids[1]->text);
                if (idx < 0) { wat_error("unknown export func"); return -1; }
                emit_export_entry(&b, e->kids[1]->text, e->kids[1]->len, 0x00, (uint32_t)idx);
            } else if (!strcmp(kind, "global")) {
                int32_t idx = resolve_name(a.global_names, a.nglobal_total, desc->kids[1]->text);
                if (idx < 0) { wat_error("unknown export global"); return -1; }
                emit_export_entry(&b, e->kids[1]->text, e->kids[1]->len, 0x03, (uint32_t)idx);
            } else if (!strcmp(kind, "memory")) {
                emit_export_entry(&b, e->kids[1]->text, e->kids[1]->len, 0x02, 0);
            } else if (!strcmp(kind, "table")) {
                emit_export_entry(&b, e->kids[1]->text, e->kids[1]->len, 0x01, 0);
            } else { wat_error("bad export kind"); return -1; }
        }
        /* 内联 (export "name")：func / global / memory / table */
        for (size_t i = 0; i < a.nfuncs; i++) {
            SExpr *f = a.funcs[i];
            for (size_t j = 1; j < f->nkids; j++) {
                SExpr *k = f->kids[j];
                if (k->kind == SX_LIST && k->nkids >= 2 && !strcmp(k->kids[0]->text, "export"))
                    emit_export_entry(&b, k->kids[1]->text, k->kids[1]->len, 0x00, (uint32_t)(a.imp_func + i));
            }
        }
        for (size_t i = 0; i < a.nglobals; i++) {
            SExpr *g = a.globals[i];
            for (size_t j = 1; j < g->nkids; j++) {
                SExpr *k = g->kids[j];
                if (k->kind == SX_LIST && k->nkids >= 2 && !strcmp(k->kids[0]->text, "export"))
                    emit_export_entry(&b, k->kids[1]->text, k->kids[1]->len, 0x03, (uint32_t)(a.imp_global + i));
            }
        }
        if (a.memory) {
            for (size_t j = 1; j < a.memory->nkids; j++) {
                SExpr *k = a.memory->kids[j];
                if (k->kind == SX_LIST && k->nkids >= 2 && !strcmp(k->kids[0]->text, "export"))
                    emit_export_entry(&b, k->kids[1]->text, k->kids[1]->len, 0x02, 0);
            }
        }
        if (a.table) {
            for (size_t j = 1; j < a.table->nkids; j++) {
                SExpr *k = a.table->kids[j];
                if (k->kind == SX_LIST && k->nkids >= 2 && !strcmp(k->kids[0]->text, "export"))
                    emit_export_entry(&b, k->kids[1]->text, k->kids[1]->len, 0x01, 0);
            }
        }
        put_section(out, 7, &b);
        bb_free(&b);
    }

    /* ---- start section ---- */
    if (a.start) {
        ByteBuf b; bb_init(&b);
        if (a.start->nkids < 2 || a.start->kids[1]->kind != SX_ATOM) { wat_error("bad start"); return -1; }
        int32_t idx = resolve_name(a.func_names, a.nfunc_total, a.start->kids[1]->text);
        if (idx < 0) { wat_error("unknown start func"); return -1; }
        bb_put_leb_u32(&b, (uint32_t)idx);
        put_section(out, 8, &b);
        bb_free(&b);
    }

    /* ---- elem section ---- */
    if (a.nelems > 0) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)a.nelems);
        for (size_t i = 0; i < a.nelems; i++) {
            SExpr *e = a.elems[i];
            bb_put_leb_u32(&b, 0); /* table index */
            /* offset */
            size_t j = 1; uint32_t off = 0;
            if (j < e->nkids && e->kids[j]->kind == SX_LIST && !strcmp(e->kids[j]->kids[0]->text, "i32.const")) {
                if (e->kids[j]->nkids >= 2) { int64_t v; if (parse_int_lit(e->kids[j]->kids[1]->text, &v)) off = (uint32_t)v; }
                j++;
            } else if (j < e->nkids && e->kids[j]->kind == SX_ATOM) {
                int64_t v; if (parse_int_lit(e->kids[j]->text, &v)) off = (uint32_t)v;
                j++;
            }
            bb_put(&b, 0x41); bb_put_leb_s32(&b, (int32_t)off); bb_put(&b, 0x0B);
            /* func indices */
            size_t cnt = 0; for (size_t k = j; k < e->nkids; k++) cnt++;
            bb_put_leb_u32(&b, (uint32_t)cnt);
            for (size_t k = j; k < e->nkids; k++) {
                if (e->kids[k]->kind != SX_ATOM) { wat_error("bad elem"); return -1; }
                int32_t idx = resolve_name(a.func_names, a.nfunc_total, e->kids[k]->text);
                if (idx < 0) { wat_error("unknown elem func '%s'", e->kids[k]->text); return -1; }
                bb_put_leb_u32(&b, (uint32_t)idx);
            }
        }
        put_section(out, 9, &b);
        bb_free(&b);
    }

    /* ---- code section ---- */
    if (a.nfuncs > 0) {
        ByteBuf bodies; bb_init(&bodies);
        bb_put_leb_u32(&bodies, (uint32_t)a.nfuncs);
        for (size_t i = 0; i < a.nfuncs; i++) {
            SExpr *f = a.funcs[i];
            /* 收集 locals（非参数）与参数名 */
            FnCtx fc; memset(&fc, 0, sizeof(fc));
            /* 参数名 */
            for (size_t j = 1; j < f->nkids; j++) {
                SExpr *c = f->kids[j];
                if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "param")) {
                    for (size_t k = 1; k < c->nkids; k++) {
                        if (c->kids[k]->kind == SX_ATOM && c->kids[k]->text[0] == '$' && k + 1 < c->nkids) {
                            fc_push_name(&fc.locals, &fc.nlocals, c->kids[k]->text + 1); k++;
                        } else if (c->kids[k]->kind == SX_ATOM) {
                            int t = valtype_from_str(c->kids[k]->text);
                            if (t >= 0) fc_push_name(&fc.locals, &fc.nlocals, NULL);
                        }
                    }
                }
            }
            /* local 类型簇 + 名字 */
            uint8_t *local_types = NULL; size_t nlocal_types = 0;
            for (size_t j = 1; j < f->nkids; j++) {
                SExpr *c = f->kids[j];
                if (c->kind == SX_LIST && !strcmp(c->kids[0]->text, "local")) {
                    for (size_t k = 1; k < c->nkids; k++) {
                        if (c->kids[k]->kind == SX_ATOM && c->kids[k]->text[0] == '$' && k + 1 < c->nkids) {
                            int t = valtype_from_str(c->kids[k + 1]->text);
                            if (t >= 0) { local_types = realloc(local_types, nlocal_types + 1); local_types[nlocal_types++] = (uint8_t)t; fc_push_name(&fc.locals, &fc.nlocals, c->kids[k]->text + 1); }
                            k++;
                        } else if (c->kids[k]->kind == SX_ATOM) {
                            int t = valtype_from_str(c->kids[k]->text);
                            if (t >= 0) { local_types = realloc(local_types, nlocal_types + 1); local_types[nlocal_types++] = (uint8_t)t; fc_push_name(&fc.locals, &fc.nlocals, NULL); }
                        }
                    }
                }
            }
            /* 函数隐式标签 */
            fc_push_name(&fc.labels, &fc.nlabels, NULL);

            /* 发射指令体 */
            ByteBuf code; bb_init(&code);
            /* 找 body 起点：跳过 param/result/local/type/export/$name */
            size_t body_start = 1;
            for (; body_start < f->nkids; body_start++) {
                SExpr *c = f->kids[body_start];
                if (c->kind == SX_ATOM && c->text[0] == '$') continue;
                if (c->kind == SX_LIST) {
                    const char *k = c->kids[0]->text;
                    if (!strcmp(k, "param") || !strcmp(k, "result") || !strcmp(k, "local") ||
                        !strcmp(k, "type") || !strcmp(k, "export")) continue;
                }
                break;
            }
            if (emit_seq(&a, &code, f->kids, f->nkids, &body_start, &fc) != R_OK) { wat_error("bad function body"); return -1; }

            /* 组装 body：locals 簇 + 指令 */
            ByteBuf body; bb_init(&body);
            /* 把 local_types 压缩成簇 */
            size_t li = 0; size_t clusters = 0; ByteBuf decl; bb_init(&decl);
            while (li < nlocal_types) {
                uint8_t t = local_types[li]; size_t cnt = 0;
                while (li < nlocal_types && local_types[li] == t) { cnt++; li++; }
                bb_put_leb_u32(&decl, (uint32_t)cnt);
                bb_put(&decl, t);
                clusters++;
            }
            bb_put_leb_u32(&body, (uint32_t)clusters);
            bb_put_bytes(&body, decl.data, decl.len);
            bb_put_bytes(&body, code.data, code.len);
            bb_put(&body, 0x0B); /* func body end */

            bb_put_leb_u32(&bodies, (uint32_t)body.len);
            bb_put_bytes(&bodies, body.data, body.len);

            bb_free(&body); bb_free(&decl); bb_free(&code);
            free(local_types);
            for (size_t x = 0; x < fc.nlocals; x++) free(fc.locals[x]);
            free(fc.locals);
            for (size_t x = 0; x < fc.nlabels; x++) free(fc.labels[x]);
            free(fc.labels);
        }
        put_section(out, 10, &bodies);
        bb_free(&bodies);
    }

    /* ---- data section ---- */
    if (a.ndatas > 0) {
        ByteBuf b; bb_init(&b);
        bb_put_leb_u32(&b, (uint32_t)a.ndatas);
        for (size_t i = 0; i < a.ndatas; i++) {
            SExpr *d = a.datas[i];
            size_t j = 1; uint32_t off = 0;
            if (j < d->nkids && d->kids[j]->kind == SX_LIST && !strcmp(d->kids[j]->kids[0]->text, "i32.const")) {
                if (d->kids[j]->nkids >= 2) { int64_t v; if (parse_int_lit(d->kids[j]->kids[1]->text, &v)) off = (uint32_t)v; }
                j++;
            } else if (j < d->nkids && d->kids[j]->kind == SX_ATOM) {
                int64_t v; if (parse_int_lit(d->kids[j]->text, &v)) off = (uint32_t)v;
                j++;
            }
            /* 计算数据总长 */
            size_t total = 0;
            for (size_t k = j; k < d->nkids; k++) total += d->kids[k]->len;
            bb_put(&b, 0x00); /* memidx */
            bb_put(&b, 0x41); bb_put_leb_s32(&b, (int32_t)off); bb_put(&b, 0x0B);
            bb_put_leb_u32(&b, (uint32_t)total);
            for (size_t k = j; k < d->nkids; k++) bb_put_bytes(&b, (uint8_t *)d->kids[k]->text, d->kids[k]->len);
        }
        put_section(out, 11, &b);
        bb_free(&b);
    }

    /* ---- 释放 ---- */
    free(a.func_names); free(a.global_names); free(a.func_typeidx);
    for (size_t i = 0; i < a.ntypes_sig; i++) { free(a.types_sig[i].params); free(a.types_sig[i].results); free(a.type_names[i]); }
    free(a.types_sig); free(a.type_names);
    free(a.types); free(a.imports); free(a.funcs); free(a.globals); free(a.exports);
    free(a.elems); free(a.datas);

    return 0;
}

/* ------------------------------------------------------------------ */
/* 公开入口                                                            */
/* ------------------------------------------------------------------ */
bool deep_wat_is_path(const char *path) {
    if (!path) return false;
    size_t n = strlen(path);
    if (n < 4) return false;
    const char *ext = path + n - 4;
    return strcasecmp(ext, ".wat") == 0;
}

int32_t deep_wat_compile_file(const char *input_path, const char *output_path) {
    FILE *f = fopen(input_path, "rb");
    if (!f) { wat_error("cannot open '%s'", input_path); return -1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *src = (char *)malloc((size_t)sz + 1);
    if (fread(src, 1, (size_t)sz, f) != (size_t)sz) { wat_error("read failed '%s'", input_path); fclose(f); free(src); return -1; }
    src[sz] = 0;
    fclose(f);

    TokenVec toks; memset(&toks, 0, sizeof(toks));
    if (!tokenize(src, &toks)) { free(src); tv_free(&toks); return -1; }

    size_t ti = 0;
    SExpr *root = parse_one(&toks, &ti);

    ByteBuf out; bb_init(&out);
    int rc = -1;
    if (!root) wat_error("empty input");
    else rc = assemble_module(root, &out);

    if (rc == 0) {
        FILE *o = fopen(output_path, "wb");
        if (!o) { wat_error("cannot write '%s'", output_path); rc = -1; }
        else {
            fwrite(out.data, 1, out.len, o);
            fclose(o);
        }
    }

    sx_free(root);
    tv_free(&toks);
    bb_free(&out);
    free(src);
    return rc;
}
