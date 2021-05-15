
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_log.h"
#define WASM_FILE_SIZE 1024

int32_t main(int argv, char ** args) {
    char* path;
    if(argv==1){
        error("no file input!");
    }else{
        path = args[1];
    }
	uint8_t* q = (uint8_t *) malloc(WASM_FILE_SIZE);
	uint8_t* p = q;
    if (p == NULL) {
        error("malloc fail.");
        return -1;
    }

    FILE *fp = fopen(path, "rb"); /* read wasm file with binary mode */
    if(fp == NULL) {
        error("file open fail.");
        return -1;
    }
	int32_t size = fread(p, 1, WASM_FILE_SIZE, fp);
    if (size == 0) {
        error ("fread faill.");
        return -1;
    }
	DEEPModule* module = deep_load(&p, size);
    if (module == NULL) {
        error("load fail.");
        return -1;
    }
    //创建操作数栈
    DEEPStack *stack = stack_cons();
    //先声明环境并初始化
    DEEPExecEnv deep_env;
    DEEPExecEnv* current_env = &deep_env;
    current_env->sp_end = stack->sp_end;
    current_env->sp = stack->sp;
    int32_t ans = call_main(current_env,module);

    printf("%d\n",ans);
    fflush(stdout);

    /* release memory */
    fclose(fp);
    free(module);
    free(q);
    p = NULL;
    return 0;
}
