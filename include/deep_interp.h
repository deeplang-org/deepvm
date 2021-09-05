//
//Created by xj on 2021/3/30.
//

#ifndef _DEEP_INTERP_H
#define _DEEP_INTERP_H

#include <stdint.h>
#include "deep_loader.h"

//DEEP帧类型
typedef enum DEEPFrameType {
    FUNCTION_FRAME,
    BLOCK_FRAME,
    IF_THEN_FRAME,
    IF_ELSE_FRAME,
    LOOP_FRAME,
} DEEPFrameType;

//DEEP帧
typedef struct DEEPInterpFrame {
    struct DEEPInterpFrame *prev_func_frame; //指向前一个函数帧
    struct DEEPFunction *function; //当前函数实例
    uint32_t *sp; //操作数栈指针
    DEEPFrameType type; //类型
} DEEPInterpFrame;

//操作数栈
typedef struct DEEPStack {
    int32_t capacity;
    uint32_t *sp;
    uint32_t *sp_end;
} DEEPStack;

//控制栈
typedef struct DEEPControlStack {
    int32_t capacity;
    DEEPInterpFrame **frames;
    uint32_t current_frame_index;
} DEEPControlStack;

typedef struct DEEPExecEnv {

    struct DEEPInterpFrame *cur_frame; //当前函数帧
    uint32_t *sp_end; //操作数栈大小
    uint32_t *sp; //sp指针
    uint32_t *local_vars; //函数局部变量
    uint32_t *global_vars; //全局变量
    uint8_t *memory; //内存
    DEEPControlStack *control_stack; //控制栈
} DEEPExecEnv;


//创建操作数栈
DEEPStack *stack_cons(void);

//创建控制栈
DEEPControlStack *control_stack_cons(void);

//销毁操作数栈
void stack_free(DEEPStack *stack);

//销毁控制栈
void control_stack_free(DEEPControlStack *stack);

int32_t call_main(DEEPExecEnv *current_env, DEEPModule *module);

void call_function(DEEPExecEnv *current_env, DEEPModule *module, int func_index);

#endif /* _DEEP_INTERP_H */
