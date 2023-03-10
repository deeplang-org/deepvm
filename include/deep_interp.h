//
//Created by xj on 2021/3/30.
//

#ifndef _DEEP_INTERP_H
#define _DEEP_INTERP_H

#include <stdbool.h>
#include <stdint.h>
#include "deep_loader.h"

//DEEP帧类型
typedef enum DEEPFrameType {
    FUNCTION_FRAME,
    BLOCK_FRAME,
    IF_FRAME,
    LOOP_FRAME,
} DEEPFrameType;

//DEEP帧
typedef struct DEEPInterpFrame {
    struct DEEPInterpFrame *prev_func_frame; //指向前一个函数帧
    struct DEEPFunction *function; //当前函数实例
    uint8_t *sp; //操作数栈指针
    DEEPFrameType type; //类型
    uint8_t *local_vars; //局部变量
} DEEPInterpFrame;

//操作数栈
typedef struct DEEPStack {
    int32_t capacity;
    uint8_t *sp;
    uint8_t *sp_end;
} DEEPStack;

//控制栈
typedef struct DEEPControlStack {
    int32_t capacity;
    DEEPInterpFrame **frames;
    uint32_t current_frame_index;
} DEEPControlStack;

typedef struct DEEPExecEnv {

    struct DEEPInterpFrame *cur_frame; //当前函数帧
    uint8_t *sp_end; //操作数栈大小
    uint8_t *sp; //sp指针
    uint8_t *local_vars; //函数局部变量
    uint64_t *global_vars; //全局变量
    uint8_t *memory; //内存
    DEEPControlStack *control_stack; //控制栈
    int jump_depth; //为0时表示不需要跳出函数或结构体；为x表示要跳出x层；
    // 为负数表示正在执行return指令
} DEEPExecEnv;


//创建操作数栈
DEEPStack *stack_cons(void);

//创建控制栈
DEEPControlStack *control_stack_cons(void);

//销毁操作数栈
void stack_free(DEEPStack *stack);

//销毁控制栈
void control_stack_free(DEEPControlStack *stack);

//读结构体
void read_block(uint8_t *ip, uint8_t **start, uint32_t *offset);

int64_t call_main(DEEPExecEnv *current_env, DEEPModule *module);

void call_function(DEEPExecEnv *current_env, DEEPModule *module, int func_index);

//进入一个block, 需要提供这个block对应的DEEPFunction（我们把block也当作函数包装进去）
//返回执行结束时的指令地址
uint8_t *enter_frame(DEEPExecEnv *current_env, DEEPModule *module, DEEPFunction *block, DEEPFrameType frame_type);

#endif /* _DEEP_INTERP_H */
