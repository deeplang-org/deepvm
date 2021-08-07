
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_log.h"
#include "deep_mem.h"

#define WASM_FILE_SIZE 1024

// 能分配的总空间大小（整个解释器用到的stack之外的空间全部算在里面）
#define MEM_SIZE (2 * PAGESIZE)

// 声明deepmem的空间
uint8_t deepmem[MEM_SIZE] = {0};

int32_t main(int argv, char **args) {
    // 初始化deepmem
    deep_mem_init(deepmem, MEM_SIZE);

    char *path;
    if(argv==1){
        deep_error("no file input!");
    }else{
        path = args[1];
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
    //先声明环境并初始化
    DEEPExecEnv deep_env;
    DEEPExecEnv *current_env = &deep_env;
    current_env->sp_end = stack->sp_end;
    current_env->sp = stack->sp;
    current_env->global_vars = (uint32_t *) deep_malloc(sizeof(uint32_t) * 100);
    current_env->memory = init_memory(1);
    int32_t ans = call_main(current_env, module);

    printf("%d\n", ans);
    fflush(stdout);

    /* release memory */
    fclose(fp);
    stack_free(stack);
    module_free(module);
    deep_free(current_env->global_vars);
    deep_free(current_env->memory);
    deep_free(q);
    p = NULL;
    return 0;
}
