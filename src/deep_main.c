
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_log.h"
#include "deep_mem.h"
#include "deep_version.h"
#include "deep_wat.h"

#define WASM_FILE_SIZE 1024

// 能分配的总空间大小（整个解释器用到的stack之外的空间全部算在里面）
#define MEM_SIZE (2 * PAGESIZE)

// 全局变量数量上限
#define GLOBAL_VAR_COUNT 100

// 声明deepmem的空间
uint8_t deepmem[MEM_SIZE] = {0};

int32_t main(int argv, char **args) {
    // 解析参数：--version / -v、-o <输出>、输入文件
    char *input_path = NULL;
    char *output_path = NULL;
    for (int32_t i = 1; i < argv; i++) {
        if (strcmp(args[i], "--version") == 0 || strcmp(args[i], "-v") == 0) {
            deep_print_version();
            return 0;
        } else if (strcmp(args[i], "-o") == 0) {
            if (i + 1 < argv) {
                output_path = args[++i];
            } else {
                deep_error("-o needs an output path");
                return -1;
            }
        } else if (input_path == NULL) {
            input_path = args[i];
        }
    }

    // WAT 文本 → 汇编为 .wasm 二进制（扩展名自动识别）
    if (input_path != NULL && deep_wat_is_path(input_path)) {
        char *out = output_path;
        bool owned = false;
        if (out == NULL) {
            size_t len = strlen(input_path);
            out = (char *)malloc(len + 2);
            if (out == NULL) {
                deep_error("malloc fail.");
                return -1;
            }
            memcpy(out, input_path, len - 3);
            strcpy(out + len - 3, "wasm");
            owned = true;
        }
        int32_t rc = deep_wat_compile_file(input_path, out);
        if (owned) free(out);
        return rc;
    }

    // 初始化deepmem
    deep_mem_init(deepmem, MEM_SIZE);

    // 测试用：获取当前空闲内存数量
    uint64_t free_mem = deep_get_free_memory();

    char *path = input_path;
    if (path == NULL) {
        deep_error("no file input!");
        return -1;
    }
    uint8_t *q = (uint8_t *) deep_malloc(WASM_FILE_SIZE);
    uint8_t *p = q;
    if (p == NULL) {
        deep_error("malloc fail.");
        return -1;
    }

    FILE *fp = fopen(path, "rb"); /* read wasm file with binary mode */
    if (fp == NULL) {
        deep_error("file open fail.");
        return -1;
    }
    int32_t size = fread(p, 1, WASM_FILE_SIZE, fp);
    if (size == 0) {
        deep_error ("fread faill.");
        return -1;
    }
    DEEPModule *module = deep_load(&p, size);
    if (module == NULL) {
        deep_error("load fail.");
        return -1;
    }
    //创建操作数栈
    DEEPStack *stack = stack_cons();
    //创建控制栈
    DEEPControlStack *control_stack = control_stack_cons();
    //先声明环境并初始化
    DEEPExecEnv deep_env;
    DEEPExecEnv *current_env = &deep_env;
    current_env->sp_end = stack->sp_end;
    current_env->sp = stack->sp;
    // 线性内存：按 Memory section 的最小页数初始化（无该 section 时默认 1 页）
    uint32_t min_pages = 1;
    if (module->memory != NULL && module->memory->min_pages > 0) {
        min_pages = module->memory->min_pages;
    }
    current_env->memory = init_memory(min_pages);
    current_env->memory_pages = min_pages;

    // 全局变量：按 Global section 初始化（每个全局变量占 8 字节，值存于低字节）
    uint32_t global_bytes = module->global_count * sizeof(uint64_t);
    current_env->global_vars = (uint64_t *) deep_malloc(global_bytes > 0 ? global_bytes : sizeof(uint64_t));
    for (uint32_t i = 0; i < module->global_count; i++) {
        current_env->global_vars[i] = module->global_section[i]->init_value;
    }

    // 函数表：按 Table/Elem section 初始化（call_indirect 使用）
    current_env->table = NULL;
    current_env->table_size = 0;
    if (module->table != NULL && module->table_count > 0) {
        current_env->table_size = module->table->min;
        current_env->table = (uint32_t *) deep_malloc(current_env->table_size * sizeof(uint32_t));
        for (uint32_t i = 0; i < current_env->table_size; i++) {
            current_env->table[i] = UINT32_MAX; // 未初始化哨兵
        }
        // 应用 elem 段
        for (uint32_t i = 0; i < module->elem_count; i++) {
            DEEPElem *e = module->elem_section[i];
            for (uint32_t j = 0; j < e->func_count; j++) {
                uint32_t idx = e->offset + j;
                if (idx < current_env->table_size) {
                    current_env->table[idx] = e->func_indices[j];
                }
            }
        }
    }

    current_env->control_stack = control_stack;
    current_env->jump_depth = 0;
    int64_t ans = call_main(current_env, module);

    printf("%ld\n", ans);
    fflush(stdout);

    /* release memory */
    fclose(fp);
    stack_free(stack);
    control_stack_free(control_stack);
    module_free(module);
    deep_free(current_env->global_vars);
    deep_free(current_env->memory);
    deep_free(current_env->table);
    deep_free(q);
    p = NULL;

    // 测试用：如果结束时空闲内存数量不变，说明没有内存泄漏
    if (free_mem != deep_get_free_memory()) {
        deep_error("memory leak!");
        return -1;
    }
    return 0;
}
