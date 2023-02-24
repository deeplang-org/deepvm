//
// Created by xj on 2021/3/30.
//
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_log.h"
#include "deep_mem.h"
#include "deep_opcode.h"

#define popS32() (*(int32_t *)(--sp))
#define popF32() (*(float *)(--sp))
#define popU32() (*(uint32_t *)(--sp))
#define pushS32(x) *(sp) = (int32_t)(x);sp++
#define pushF32(x) do {float __x = (float)x;*(sp) = *(int32_t *)(&__x);sp++;} while (0)
#define pushU32(x) *(sp) = (uint32_t)(x);sp++

#define popS64() (*(int64_t *)(--sp))
#define popF64() (*(double *)(--sp))
#define popU64() (*(uint64_t *)(--sp))
#define pushS64(x) *(sp) = (int64_t)(x);sp++
#define pushF64(x) do {double __x = (double)x;*(sp) = *(int64_t *)(&__x);sp++;} while (0)
#define pushU64(x) *(sp) = (uint64_t)(x);sp++

#define READ_VALUE(Type, p) \
    (p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

#define STACK_CAPACITY 100
#define CONTROL_STACK_CAPACITY 100

// 安全除法
#define DIVIDE(TYPE, DIVIDEND, DIVISOR) \
(((TYPE)DIVISOR == 0) && \
    (deep_error("Arithmetic Error: Division by Zero!"), exit(1), 0), \
        (TYPE)DIVIDEND / (TYPE)DIVISOR)
#define MODULUS(TYPE, DIVIDEND, DIVISOR) \
(((TYPE)DIVISOR == 0) && \
    (deep_error("Arithmetic Error: Remainder by Zero!"), exit(1), 0), \
        (TYPE)DIVIDEND % (TYPE)DIVISOR)

typedef void *(*fun_ptr_t)(void *);

typedef void (*built_in_function)(DEEPExecEnv *env, DEEPModule *module);

typedef struct {
    char *func_name;
    fun_ptr_t func;
} DEEPNative;

static void native_puts(DEEPExecEnv *env, DEEPModule *module) {
    uint64_t *sp = env->cur_frame->sp;
    uint32_t offset = popU32();
    DEEPData *data;
    bool data_found = false;
    //找到包含该offset的数据段信息。以后可以考虑用DEEPHash优化
    for (uint32_t i = 0; i < module->data_count; i++) {
        data = module->data_section[i];
        if (data->offset <= offset && offset < data->offset + data->datasize) {
            data_found = true;
            break;
        }
    }

    if (!data_found) {
        pushS32(-1);
        return;
    }

    printf("%s",(char *)data->data);
    pushS32(0);
}

static void native_puti(DEEPExecEnv *env, DEEPModule *module) {
    uint64_t *sp = env->cur_frame->sp;
    printf("%d", popS32());
    pushS32(0);
}

static void native_putf(DEEPExecEnv *env, DEEPModule *module) {
    uint64_t *sp = env->cur_frame->sp;
    printf("%f", popF32());
    pushS32(0);
}

#define DEEPNATIVE_COUNT 3

//表：所有的built-in函数
static DEEPNative g_DeepNativeMap[] = {
    {"puts", (fun_ptr_t)native_puts},
    {"puti", (fun_ptr_t)native_puti},
    {"putf", (fun_ptr_t)native_putf},
};

static void deep_native_call(const char *name, DEEPExecEnv *env, DEEPModule *module) {
    //TODO: 用DEEPHash避免多次比较
        for (unsigned i = 0; i < DEEPNATIVE_COUNT; i++) {
            if (!strcmp(name, g_DeepNativeMap[i].func_name)) {
                ((built_in_function)(g_DeepNativeMap[i].func))(env, module);
                break;
            }
        }
}

//创建操作数栈
DEEPStack *stack_cons(void) {
    DEEPStack *stack = (DEEPStack *) deep_malloc(sizeof(DEEPStack));
    if (stack == NULL) {
        deep_error("Operand stack creation failed!");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp = (uint64_t *) deep_malloc(sizeof(uint64_t) * STACK_CAPACITY);
    if (stack->sp == NULL) {
        deep_error("Malloc area for stack error!");
    }
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//销毁操作数栈
void stack_free(DEEPStack *stack) {
    deep_free(stack->sp);
    deep_free(stack);
}

//创建控制栈
DEEPControlStack *control_stack_cons(void) {
    DEEPControlStack *stack = (DEEPControlStack *)deep_malloc(sizeof(DEEPControlStack));
    if (stack == NULL) {
        deep_error("Control stack creation failed!");
        return NULL;
    }
    stack->capacity = CONTROL_STACK_CAPACITY;
    stack->frames = (DEEPInterpFrame **)deep_malloc(stack->capacity * sizeof(DEEPInterpFrame *));
    if (stack->frames == NULL) {
        deep_error("Control stack creation failed!");
        deep_free(stack);
        return NULL;
    }
    stack->current_frame_index = 0;
    return stack;
}

//销毁控制栈
void control_stack_free(DEEPControlStack *stack) {
    assert(stack->current_frame_index == 0);
    deep_free(stack->frames);
    deep_free(stack);
}

//读结构体的范围
void read_block(uint8_t *ip, uint8_t **start, uint32_t *offset) {
    *start = ip;
    bool finish = false;
    uint8_t net_bracket = 1;
    while (!finish) {
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t)*ip;
        // printf("%x\n", opcode);
        switch (opcode)
        {
        case op_end:
            ip++;
            net_bracket--;
            if (net_bracket == 0) {
                *offset = (uint32_t)(ip - *start);
                finish = true;
            }
            break;
        case op_else:
            ip++;
            net_bracket--;
            if (net_bracket == 0) {
                *offset = (uint32_t)(ip - *start);
                finish = true;
            }
            net_bracket++;
            break;
        //面对其他指令，只要正常阅读，不需要解释；这个函数只找end或else
        //无立即数指令：
        case op_return:
        case op_nop:
        case op_unreachable:
        case i32_add:
        case i64_add:
        case i32_and:
        case i64_and:
        case i32_clz:
        case i32_ctz:
        case i32_popcnt:
        case i64_clz:
        case i64_ctz:
        case i64_popcnt:
        case i32_divs:
        case i64_divs:
        case i32_divu:
        case i64_divu:
        case i32_eq:
        case i32_eqz:
        case i32_ne:
        case i32_ges:
        case i32_geu:
        case i32_gts:
        case i32_gtu:
        case i32_lts:
        case i32_ltu:
        case i32_les:
        case i32_leu:
        case i64_eq:
        case i64_eqz:
        case i64_ne:
        case i64_ges:
        case i64_geu:
        case i64_gts:
        case i64_gtu:
        case i64_lts:
        case i64_ltu:
        case i64_les:
        case i64_leu:
        case i32_mul:
        case i64_mul:
        case i32_or:
        case i64_or:
        case i32_rems:
        case i64_rems:
        case i32_remu:
        case i64_remu:
        case i32_rotl:
        case i64_rotl:
        case i32_rotr:
        case i64_rotr:
        case i32_shl:
        case i64_shl:
        case i32_shrs:
        case i64_shrs:
        case i32_shru:
        case i64_shru:
        case i32_sub:
        case i64_sub:
        case i32_trunc_f32_s:
        case i32_trunc_f32_u:
        case i32_trunc_f64_s:
        case i32_trunc_f64_u:
        case i64_trunc_f32_s:
        case i64_trunc_f32_u:
        case i64_trunc_f64_s:
        case i64_trunc_f64_u:
        case i32_xor:
        case i64_xor:
        case f32_eq:
        case f32_ne:
        case f32_lt:
        case f32_gt:
        case f32_le:
        case f32_ge:
        case f32_abs:
        case f64_abs:
        case f32_add:
        case f32_ceil:
        case f64_ceil:
        case f32_convert_i32_s:
        case f32_convert_i32_u:
        case f32_convert_i64_s:
        case f32_convert_i64_u:
        case f64_convert_i32_s:
        case f64_convert_i32_u:
        case f64_convert_i64_s:
        case f64_convert_i64_u:
        case i32_reinterpret_f32:
        case i64_reinterpret_f64:
        case f32_reinterpret_i32:
        case f64_reinterpret_i64:
        case f32_copysign:
        case f32_div:
        case f32_floor:
        case f64_floor:
        case f32_max:
        case f32_min:
        case f32_mul:
        case f32_nearest:
        case f64_nearest:
        case f32_neg:
        case f64_neg:
        case f32_sqrt:
        case f64_sqrt:
        case f32_sub:
        case f32_trunc:
        case f64_trunc:
        case f64_eq:
        case f64_ne:
        case f64_lt:
        case f64_gt:
        case f64_le:
        case f64_ge:
        case i32_wrap_i64:
        case i64_extend_i32_s:
        case i64_extend_i32_u:
        case f32_demote_f64:
        case f64_promote_f32:
        case op_drop:
        case op_select:
            ip++;
            break;
        //需要一个leb立即数的指令：
        case op_call:
        case op_local_get:
        case op_local_set:
        case op_local_tee:
        case op_global_get:
        case op_global_set:
        case f32_const:
        case i32_const:
        case f64_const:
        case i64_const:
        case op_br:
        case op_br_if:
            ip++;
            read_leb_u32(&ip);
            break;
        //需要特殊考虑的指令：
        case op_block:
        case op_loop:
        case op_if:
            ip++;
            READ_BYTE(ip);
            net_bracket++;
            break;
        case op_br_table:
            ip++;
            ip += read_leb_u32(&ip) + 1;
            break;
        default:
            deep_error("Unknown opcode %x!", opcode);
            exit(1);
        }
    }
}

//执行代码块指令，true表示正常结束，false表示碰到br语句而跳出。
bool exec_instructions(DEEPExecEnv *current_env, DEEPModule *module) {
    uint64_t *sp = current_env->cur_frame->sp;
    uint8_t *ip = current_env->cur_frame->function->code_begin;
    uint8_t *ip_end = ip + current_env->cur_frame->function->code_size;
    uint8_t *memory = current_env->memory;
    while (current_env->jump_depth || ip < ip_end) {
        //判断是否需要跳出
        if (current_env->jump_depth) {
            current_env->jump_depth--;
            //更新env
            current_env->sp = sp;
            return false;
        }
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t) *ip;
        // printf("op: %x\n", opcode);
        switch (opcode) {
            /* 控制指令 */
            case op_unreachable: {
                deep_error("Runtime Error: Unreachable!");
                exit(1);
                break;
            }
            case op_nop: {
                ip++;
                break;
            }
            case op_block:
            case op_loop: {
                ip++;
                //暂时不支持WASM限制放开之后提供的多返回值和传参数机制
                uint8_t return_type = READ_BYTE(ip);
                DEEPType *type = deep_malloc(sizeof(DEEPType));
                //block无参数
                type->param_count = 0;
                type->ret_count = return_type == op_type_void ? 0 : 1;
                //设置返回值的类型（如果有）
                if (type->ret_count == 1) {
                    type->type = deep_malloc(1);
                    type->type[0] = return_type;
                }
                //读取block主体
                uint8_t *start;
                uint32_t offset;
                read_block(ip, &start, &offset);
                //为block创建DEEPFunction结构体
                DEEPFunction *block = deep_malloc(sizeof(DEEPFunction));
                if (block == NULL) {
                    deep_error("Heap overflow at opcode %x!", opcode);
                    exit(1);
                }
                //block的局部变量和当前function的一致
                block->local_var_types = current_env->cur_frame->function->local_var_types;
                block->local_var_count = current_env->cur_frame->function->local_var_count;
                block->code_begin = start;
                block->code_size = offset;
                block->func_type = type;
                //执行block
                current_env->sp = sp;
                ip = enter_frame(current_env, module, block,
                        opcode == op_block ? BLOCK_FRAME : LOOP_FRAME);
                sp = current_env->sp;
                //释放空间
                deep_free(type);
                deep_free(block);
                break;
            }
            case op_if: {
                ip++;
                //暂时不支持WASM限制放开之后提供的多返回值和传参数机制
                uint8_t return_type = READ_BYTE(ip);
                DEEPType *type = deep_malloc(sizeof(DEEPType));
                //if无参数
                type->param_count = 0;
                type->ret_count = return_type == op_type_void ? 0 : 1;
                //设置返回值的类型（如果有）
                if (type->ret_count == 1) {
                    type->type = deep_malloc(1);
                    type->type[0] = return_type;
                }
                //读取if两个分支的主体
                uint8_t *then_start, *else_start;
                uint32_t then_offset, else_offset;
                read_block(ip, &then_start, &then_offset);
                else_start = then_start + then_offset;
                read_block(else_start, &else_start, &else_offset);
                //为block创建DEEPFunction结构体
                DEEPFunction *block = deep_malloc(sizeof(DEEPFunction));
                if (block == NULL) {
                    deep_error("Heap overflow at opcode %x!", opcode);
                    exit(1);
                }
                //block的局部变量和当前function的一致
                block->local_var_types = current_env->cur_frame->function->local_var_types;
                block->local_var_count = current_env->cur_frame->function->local_var_count;
                //执行block
                block->func_type = type;
                current_env->sp = sp;
                if (popU32()) {
                    //执行then分支，完成之后需要把ip设置到end的位置
                    block->code_begin = then_start;
                    block->code_size = then_offset;
                    enter_frame(current_env, module, block,
                        opcode == IF_FRAME);
                    ip = else_start + else_offset;
                } else {
                    //执行else分支，完成之后ip自动在end的位置
                    block->code_begin = else_start;
                    block->code_size = else_offset;
                    ip = enter_frame(current_env, module, block,
                        opcode == IF_FRAME);
                }
                sp = current_env->sp;
                //释放空间
                deep_free(type);
                deep_free(block);
                break;
            }
            case op_br: {
                ip++;
                current_env->jump_depth = read_leb_u32(&ip) + 1;
                break;
            }
            case op_br_if: {
                ip++;
                current_env->jump_depth = popU32() ? (read_leb_u32(&ip) + 1) : (read_leb_u32(&ip), 0);
                break;
            }
            case op_br_table: {
                ip++;
                uint8_t jump_table_size = 1 + (uint8_t)read_leb_u32(&ip);
                ip += jump_table_size;
                uint8_t jump_label = (uint8_t)popS32();
                if (jump_label < jump_table_size - 1) {
                    current_env->jump_depth
                        = *(ip - jump_table_size + jump_label) + 1;
                } else {
                    current_env->jump_depth = *ip + 1;
                }
                break;
            }
            case op_return: {
                //将jump_depth设为负数，代表正在执行return指令
                //等跳出最近的函数时，会将其重新设为0
                current_env->jump_depth = -1;
                ip++;
                break;
            }
            //如果碰到else，则表示当前在then分支，并且执行完毕
            case op_else:
            case op_end: {
                ip++;
                break;
            }
            case op_call: {
                ip++;
                uint32_t func_index = read_leb_u32(&ip);//被调用函数index
                //需要一个同步操作
                current_env->sp = sp;
                call_function(current_env, module, func_index);
                sp = current_env->sp;
                break;
            }
            /* 参数指令 */
            case op_drop: {
                ip++;
                popU32();
                break;
            }
            /* 内存指令 */
            case i32_load: {
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = read_mem32(memory + base, offset);
                pushU32(number);
                break;
            }
            case i32_store: {
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = popU32();
                write_mem32(memory + base, number, offset);
                break;
            }
            case op_local_get: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_get指令的立即数
                uint64_t a = current_env->local_vars[index];
                pushU64(a);
                break;
            }
            case op_local_set: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_set指令的立即数
                current_env->local_vars[index] = popU64();
                break;
            }
            case op_local_tee: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_tee指令的立即数
                current_env->local_vars[index] = *(sp - 1);
                break;
            }
            case op_global_get: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//global_get指令的立即数
                uint64_t a = current_env->global_vars[index];
                pushU64(a);
                break;
            }
            case op_global_set: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//global_set指令的立即数
                current_env->global_vars[index] = popU64();
                break;
            }
            /* 比较指令 */
            case i32_eqz: {
                ip++;
                int32_t a = popU32();
                pushU32(a == 0 ? 1 : 0);
                break;
            }
            case i32_eq: {
                ip++;
                int32_t a = popU32();
                int32_t b = popU32();
                pushU32(b == a ? 1 : 0);
                break;
            }
            case i32_ne: {
                ip++;
                int32_t a = popU32();
                int32_t b = popU32();
                pushU32(b != a ? 1 : 0);
                break;
            }
            case i32_lts: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case i32_ltu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case i32_gts: {
                ip++;
                uint32_t a = popS32();
                uint32_t b = popS32();
                pushU32(b > a ? 1 : 0);
                break;
            }
            case i32_gtu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b > a ? 1 : 0);
                break;
            }
            case i32_les: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushU32(b <= a ? 1 : 0);
                break;
            }
            case i32_leu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b <= a ? 1 : 0);
                break;
            }
            case i32_ges: {
                ip++;
                uint32_t a = popS32();
                uint32_t b = popS32();
                pushU32(b >= a ? 1 : 0);
                break;
            }
            case i32_geu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b >= a ? 1 : 0);
                break;
            }
            case i64_eqz: {
                ip++;
                int64_t a = popU64();
                pushU32(a == 0 ? 1 : 0);
                break;
            }
            case i64_eq: {
                ip++;
                int64_t a = popU64();
                int64_t b = popU64();
                pushU32(b == a ? 1 : 0);
                break;
            }
            case i64_ne: {
                ip++;
                int64_t a = popU64();
                int64_t b = popU64();
                pushU32(b != a ? 1 : 0);
                break;
            }
            case i64_lts: {
                ip++;
                int64_t a = popS64();
                int64_t b = popS64();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case i64_ltu: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case i64_gts: {
                ip++;
                uint64_t a = popS64();
                uint64_t b = popS64();
                pushU32(b > a ? 1 : 0);
                break;
            }
            case i64_gtu: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU32(b > a ? 1 : 0);
                break;
            }
            case i64_les: {
                ip++;
                int64_t a = popS64();
                int64_t b = popS64();
                pushU32(b <= a ? 1 : 0);
                break;
            }
            case i64_leu: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU32(b <= a ? 1 : 0);
                break;
            }
            case i64_ges: {
                ip++;
                uint64_t a = popS64();
                uint64_t b = popS64();
                pushU32(b >= a ? 1 : 0);
                break;
            }
            case i64_geu: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU32(b >= a ? 1 : 0);
                break;
            }
            case f32_eq: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushU32(b == a ? 1 : 0);
                break;
            }
            case f32_ne: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushU32(b != a ? 1 : 0);
                break;
            }
            case f32_lt: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case f32_gt: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushU32(b > a ? 1 : 0);
                break;
            }
            case f32_le: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushU32(b <= a ? 1 : 0);
                break;
            }
            case f32_ge: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushU32(b >= a ? 1 : 0);
                break;
            }
            case f64_eq: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushU32(b == a ? 1 : 0);
                break;
            }
            case f64_ne: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushU32(b != a ? 1 : 0);
                break;
            }
            case f64_lt: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case f64_gt: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushU32(b > a ? 1 : 0);
                break;
            }
            case f64_le: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushU32(b <= a ? 1 : 0);
                break;
            }
            case f64_ge: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushU32(b >= a ? 1 : 0);
                break;
            }
            /* 参数指令 */
            case op_select: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                uint32_t c = popU32();
                pushU32(a ? c : b);
                break;
            }
            /* 算术指令 */
            case i32_add: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a + b);
                break;
            }
            case i64_add: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(a + b);
                break;
            }
            case i32_sub: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b - a);
                break;
            }
            case i64_sub: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b - a);
                break;
            }
            case i32_mul: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b * a);
                break;
            }
            case i64_mul: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b * a);
                break;
            }
            case i32_divs: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(DIVIDE(int32_t, b, a));
                break;
            }
            case i64_divs: {
                ip++;
                int64_t a = popS64();
                int64_t b = popS64();
                pushS64(DIVIDE(int64_t, b, a));
                break;
            }
            case i32_divu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(DIVIDE(uint32_t, b, a));
                break;
            }
            case i64_divu: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(DIVIDE(uint64_t, b, a));
                break;
            }
            case i32_rems: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(MODULUS(int32_t, b, a));
                break;
            }
            case i64_rems: {
                ip++;
                int64_t a = popS64();
                int64_t b = popS64();
                pushS64(MODULUS(int64_t, b, a));
                break;
            }
            case i32_remu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(MODULUS(uint32_t, b, a));
                break;
            }
            case i64_remu: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(MODULUS(uint64_t, b, a));
                break;
            }
            case i32_const: {
                ip++;
                uint32_t temp = read_leb_i32(&ip);
                pushU32(temp);
                break;
            }
            case i64_const: {
                ip++;
                uint64_t temp = read_leb_i64(&ip);
                pushU64(temp);
                break;
            }
            case i32_and: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b & a);
                break;
            }
            case i64_and: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b & a);
                break;
            }
            case i32_or: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b | a);
                break;
            }
            case i64_or: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b | a);
                break;
            }
            case i32_xor: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b ^ a);
                break;
            }
            case i64_xor: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b ^ a);
                break;
            }
            case i32_shl: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b << (a % 32));
                break;
            }
            case i64_shl: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b << (a % 64));
                break;
            }
            case i32_shrs: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(b < 0 ? b >> a | ~(~0U >> a) : b >> a);
                break;
            }
            case i64_shrs: {
                ip++;
                int64_t a = popS64();
                int64_t b = popS64();
                pushS64(b < 0 ? b >> a | ~(~0U >> a) : b >> a);
                break;
            }
            case i32_shru: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b >> a);
                break;
            }
            case i64_shru: {
                ip++;
                uint64_t a = popU64();
                uint64_t b = popU64();
                pushU64(b >> a);
                break;
            }
            case f32_add: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(b + a);
                break;
            }
            case f32_sub: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(b - a);
                break;
            }
            case f32_mul: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(b * a);
                break;
            }
            case f32_div: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(DIVIDE(float, b, a));
                break;
            }
            case f32_min: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(b < a ? b : a);
                break;
            }
            case f32_max: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(b > a ? b : a);
                break;
            }
            case f32_copysign: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(copysignf(b, a));
                break;
            }
            case f64_add: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(b + a);
                break;
            }
            case f64_sub: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(b - a);
                break;
            }
            case f64_mul: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(b * a);
                break;
            }
            case f64_div: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(DIVIDE(double, b, a));
                break;
            }
            case f64_min: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(b < a ? b : a);
                break;
            }
            case f64_max: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(b > a ? b : a);
                break;
            }
            case f64_copysign: {
                ip++;
                double a = popF64();
                double b = popF64();
                pushF64(copysign(b, a));
                break;
            }
            case f32_const: {
                ip++;
                float temp = read_ieee_32(&ip);
                pushF32(temp);
                break;
            }
            case f64_const: {
                ip++;
                double temp = read_ieee_64(&ip);
                pushF64(temp);
                break;
            }
            // 一元算数
            case i32_clz: {
                ip++;
                int32_t a = popS32();
                // 需要确保调用的builtin处理的是32位
                #if __SIZEOF_INT__ == 4
                pushS32(__builtin_clz(a));
                #else
                pushS32(__builtin_clzl(a));
                #endif
            }
            case i32_ctz: {
                ip++;
                int32_t a = popS32();
                // 需要确保调用的builtin处理的是32位
                #if __SIZEOF_INT__ == 4
                pushS32(__builtin_ctz(a));
                #else
                pushS32(__builtin_ctzl(a));
                #endif
            }
            case i32_popcnt: {
                ip++;
                int64_t a = popS64();
                // 需要确保调用的builtin处理的是32位
                #if __SIZEOF_INT__ == 4
                pushS32(__builtin_popcount(a));
                #else
                pushS32(__builtin_popcountl(a));
                #endif
            }
            case i64_clz: {
                ip++;
                int32_t a = popS32();
                // 需要确保调用的builtin处理的是64位
                #if __SIZEOF_LONG__ == 8
                pushS64(__builtin_clzl(a));
                #else
                pushS64(__builtin_clzll(a));
                #endif
            }
            case i64_ctz: {
                ip++;
                int64_t a = popS64();
                // 需要确保调用的builtin处理的是64位
                #if __SIZEOF_LONG__ == 8
                pushS64(__builtin_ctzl(a));
                #else
                pushS64(__builtin_ctzll(a));
                #endif
            }
            case i64_popcnt: {
                ip++;
                int64_t a = popS64();
                // 需要确保调用的builtin处理的是64位
                #if __SIZEOF_LONG__ == 8
                pushS64(__builtin_popcountl(a));
                #else
                pushS64(__builtin_popcountll(a));
                #endif
            }
            case f32_abs: {
                ip++;
                float a = popF32();
                pushF32(fabsf(a));
            }
            case f32_neg: {
                ip++;
                float a = popF32();
                pushF32(-a);
            }
            case f32_ceil: {
                ip++;
                float a = popF32();
                pushF32(ceilf(a));
            }
            case f32_floor: {
                ip++;
                float a = popF32();
                pushF32(floorf(a));
            }
            case f32_trunc: {
                ip++;
                float a = popF32();
                pushF32(truncf(a));
            }
            case f32_nearest: {
                ip++;
                float a = popF32();
                pushF32(roundf(a));
            }
            case f32_sqrt: {
                ip++;
                float a = popF32();
                pushF32(sqrtf(a));
            }
            case f64_abs: {
                ip++;
                double a = popF64();
                pushF64(fabs(a));
            }
            case f64_neg: {
                ip++;
                double a = popF64();
                pushF64(-a);
            }
            case f64_ceil: {
                ip++;
                double a = popF64();
                pushF64(ceil(a));
            }
            case f64_floor: {
                ip++;
                double a = popF64();
                pushF64(floor(a));
            }
            case f64_trunc: {
                ip++;
                double a = popF64();
                pushF64(trunc(a));
            }
            case f64_nearest: {
                ip++;
                double a = popF64();
                pushF64(round(a));
            }
            case f64_sqrt: {
                ip++;
                double a = popF64();
                pushF64(sqrt(a));
            }
            // 类型转换
            case i32_wrap_i64: {
                int32_t temp = (int64_t)popS64();
                pushS32(temp);
            }
            case i32_trunc_f32_s: {
                ip++;
                int32_t temp = (int32_t)popF32();
                pushS32(temp);
                break;
            }
            case i32_trunc_f32_u: {
                ip++;
                uint32_t temp = (uint32_t)popF32();
                pushU32(temp);
                break;
            }
            case i32_trunc_f64_s: {
                ip++;
                int32_t temp = (int32_t)popF64();
                pushS32(temp);
                break;
            }
            case i32_trunc_f64_u: {
                ip++;
                uint32_t temp = (uint32_t)popF64();
                pushU32(temp);
                break;
            }
            case i64_extend_i32_s: {
                int64_t temp = (int64_t)popS32();
                pushS64(temp);
            }
            case i64_extend_i32_u: {
                uint64_t temp = (uint64_t)popU32();
                pushU64(temp);
            }
            case i64_trunc_f32_s: {
                ip++;
                int64_t temp = (int64_t)popF32();
                pushS64(temp);
                break;
            }
            case i64_trunc_f32_u: {
                ip++;
                uint64_t temp = (uint64_t)popF32();
                pushU64(temp);
                break;
            }
            case i64_trunc_f64_s: {
                ip++;
                int64_t temp = (int64_t)popF64();
                pushS64(temp);
                break;
            }
            case i64_trunc_f64_u: {
                ip++;
                uint64_t temp = (uint64_t)popF64();
                pushU64(temp);
                break;
            }
            case f32_convert_i32_s: {
                ip++;
                float temp = (float)popS32();
                pushF32(temp);
                break;
            }
            case f32_convert_i32_u: {
                ip++;
                float temp = (float)popU32();
                pushF32(temp);
                break;
            }
            case f32_convert_i64_s: {
                ip++;
                float temp = (float)popS64();
                pushF32(temp);
                break;
            }
            case f32_convert_i64_u: {
                ip++;
                float temp = (float)popU64();
                pushF32(temp);
                break;
            }
            case f32_demote_f64: {
                ip++;
                float temp = (float)popF64();
                pushF32(temp);
                break;
            }
            case f64_convert_i32_s: {
                ip++;
                double temp = (double)popS32();
                pushF64(temp);
                break;
            }
            case f64_convert_i32_u: {
                ip++;
                double temp = (double)popU32();
                pushF64(temp);
                break;
            }
            case f64_convert_i64_s: {
                ip++;
                double temp = (double)popS64();
                pushF64(temp);
                break;
            }
            case f64_convert_i64_u: {
                ip++;
                double temp = (double)popU64();
                pushF64(temp);
                break;
            }
            case f64_promote_f32: {
                ip++;
                double temp = (double)popF32();
                pushF64(temp);
                break;
            }
            // Reinterpret不会改变实际存储方式，因此实际上不需要操作
            case i32_reinterpret_f32:
            case i64_reinterpret_f64:
            case f32_reinterpret_i32:
            case f64_reinterpret_i64: {
                ip++;
                break;
            }
            default:
                deep_error("Unknown opcode %x!", opcode);
                exit(1);
        }
        //检查操作数栈是否溢出
        if (sp > current_env->sp_end) {
            deep_error("Error: Operand stack overflow!\r\n");
            return false;
        }
    }
    //更新env
    current_env->sp = sp;
    return true;
}

//调用普通函数
void call_function(DEEPExecEnv *current_env, DEEPModule *module, int func_index) {

    //为func函数创建DEEPFunction
    DEEPFunction *func = module->func_section[func_index];

    //函数类型
    DEEPType *deepType = func->func_type;
    uint32_t ret_num = deepType->ret_count;

    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) deep_malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        deep_error("Malloc area for normal_frame error!");
        return;
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = func;
    frame->prev_func_frame = current_env->cur_frame;
    frame->type = FUNCTION_FRAME;
    frame->local_vars = (uint64_t *)deep_malloc(sizeof(uint64_t) * func->local_var_count);
    //局部变量的空间已经在函数帧中分配好
    current_env->local_vars = frame->local_vars;
    uint32_t param_count = func->func_type->param_count;
    while (param_count > 0) {
        uint64_t temp = *(--current_env->sp);
        current_env->local_vars[(param_count--) - 1] = temp;
    }

    //更新env中内容
    current_env->cur_frame = frame;
    //更新控制栈
    current_env->control_stack->current_frame_index++;
    current_env->control_stack->frames[
    current_env->control_stack->current_frame_index] = frame;

    //处理外部函数
    if (func->is_import) {
        //TODO: 用DEEPHash快速找。
        uint32_t count = 0;
        char *name = NULL;
        for (uint32_t i = 0; i < module->import_count; i++) {
            if (count == func_index) {
                 name = module->import_section[i]->member_name;
                 break;
            }

            if (module->import_section[i]->tag == FUNC_TAG_TYPE) {
                count++;
            }
        }

        if (name == NULL) {
            printf("NUM: %d\n", func_index);
            deep_error("Cannot find built-in function!\n");
            exit(-1);
        }

        deep_native_call(name, current_env, module);
    } else {
        exec_instructions(current_env, module);
    }

    //如果遇到了return指令，则跳出到这一步就可以了
    //当jump_depth为负数时，表示正在执行return指令
    if (current_env->jump_depth < 0) {
        current_env->jump_depth = 0;
    }
    //执行完毕退栈
    current_env->cur_frame = frame->prev_func_frame;
    current_env->control_stack->current_frame_index--;
    //释放掉局部变量
    deep_free(current_env->local_vars);
    deep_free(frame);
    current_env->local_vars = current_env->control_stack->frames[
    current_env->control_stack->current_frame_index]->local_vars;
}

//为main函数创建帧，执行main函数
int64_t call_main(DEEPExecEnv *current_env, DEEPModule *module) {

    //create DEEPFunction for main
    //find the index of main
    int main_index = -1;
    uint32_t export_count = module->export_count;

    for (uint32_t i = 0; i < export_count; i++) {
        if (strcmp((module->export_section[i])->name, "main") == 0) {
            main_index = module->export_section[i]->index;
        }
    }
    if (main_index < 0) {
        deep_error("the main function index failed!");
        return -1;
    }

    //为main函数创建DEEPFunction
    DEEPFunction *main_func = module->func_section[main_index]; //module.start_index记录了main函数索引

    //为main函数创建帧
    DEEPInterpFrame *main_frame = (DEEPInterpFrame *) deep_malloc(sizeof(struct DEEPInterpFrame));
    if (main_frame == NULL) {
        deep_error("Malloc area for main_frame error!");
    }
    //初始化
    main_frame->sp = current_env->sp;
    main_frame->function = main_func;
    main_frame->type = FUNCTION_FRAME;
    main_frame->local_vars = (uint64_t *)deep_malloc(sizeof(uint64_t) * main_func->local_var_count);
    //局部变量的空间已经在函数帧中分配好
    current_env->local_vars = main_frame->local_vars;

    //更新env中内容
    current_env->cur_frame = main_frame;

    //更新控制栈
    current_env->control_stack->frames[
        current_env->control_stack->current_frame_index] = main_frame;
    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);
    deep_free(main_frame->local_vars);
    deep_free(main_frame);

    //返回栈顶元素
    return *(int64_t *)(current_env->sp - 1);
}

//进入一个block/loop, 需要提供这个block对应的DEEPFunction（我们把block也当作函数包装进去）
//返回执行结束时的指令地址
uint8_t *enter_frame(DEEPExecEnv *current_env, DEEPModule *module, DEEPFunction *block, DEEPFrameType frame_type) {
    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) deep_malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        deep_error("Malloc area for normal_frame error!");
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = block;
    frame->prev_func_frame = current_env->cur_frame->prev_func_frame;
    //不需要更新局部变量，因为目前进入block之后，局部变量和function是一样的
    frame->local_vars = current_env->local_vars;
    frame->type = frame_type;

    //更新env中内容
    current_env->cur_frame = frame;

    //更新控制栈
    current_env->control_stack->current_frame_index++;
    current_env->control_stack->frames[
        current_env->control_stack->current_frame_index] = frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    //如果发现在loop中，则循环执行，直至离开loop。
    while (!exec_instructions(current_env, module) && current_env->jump_depth == 0) {
        if (frame_type != LOOP_FRAME) break;
    }

    //执行完毕退栈
    current_env->control_stack->current_frame_index--;
    current_env->cur_frame = current_env->control_stack->frames[
        current_env->control_stack->current_frame_index];
    //不释放局部变量，因为这些变量原来的函数还要用

    //这里释放了frame，但是frame里面的DEEPFunction（作为参数传进来的）还没有释放，
    //需要caller处理
    deep_free(frame);

    return block->code_begin + block->code_size;
}
