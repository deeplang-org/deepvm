//
// Created by xj on 2021/3/30.
//

#ifndef _DEEP_INTERP_H
#define _DEEP_INTERP_H

#include "deep_loader.h"

//帧
typedef struct DEEPInterpFrame {  //DEEP帧
    struct DEEPInterpFrame *prev_frame;//指向前一个帧
    struct DEEPFunction *function;//当前函数实例
    uint32_t *sp;  //操作数栈指针
} DEEPInterpFrame;

//操作数栈
typedef struct DEEPStack {
    int32_t capacity;
    uint32_t *sp;
    uint32_t *sp_end;
} DEEPStack;


typedef struct DEEPExecEnv {

    struct DEEPInterpFrame *cur_frame;//当前函数帧
    uint32_t *sp_end;//操作数栈大小
    uint32_t *sp;//sp指针
    uint32_t *local_vars;//函数局部变量
    uint32_t *global_vars;//全局变量
    uint8_t *memory;//内存
} DEEPExecEnv;


//创建操作数栈
DEEPStack *stack_cons(void);

int32_t call_main(DEEPExecEnv *current_env, DEEPModule *module);

void call_function(DEEPExecEnv *current_env, DEEPModule *module, int func_index);

#endif /* _DEEP_INTERP_H */


