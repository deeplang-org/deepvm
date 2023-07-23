//
// Created by xj on 2021/3/30.
// Later modified by MMZK1526.
//

#ifndef _DEEP_INTERP_H
#define _DEEP_INTERP_H

#include <stdbool.h>
#include <stdint.h>
#include "deep_loader.h"

// DEEP帧类型
typedef enum DEEPFrameType {
    FUNCTION_FRAME,
    BLOCK_FRAME,
    IF_FRAME,
    LOOP_FRAME,
} DEEPFrameType;

// DEEP帧
// 每当进入一个新的scope后便会创建一个新的帧，用于存储局部变量和scope本身的信息。
// 例如，当进入一个函数时，会创建一个新的函数帧，用于存储函数的局部变量和函数的信息。
// 同理，当进入loop或block时，也会创建一个新的帧。
//
// prev_func_frame: 
// 指向前一个函数帧。若当前已经在最外层函数（main函数）中，则为NULL。
// 需要该指针的原因是，当执行return指令时，需要将当前函数帧弹出，然后将控制权交给前一个函数帧。
//
// function:
// 当前函数。如果进入的是非函数（如block），则指向当前所处的函数。
//
// sp:
// 操作数栈指针。指向当前操作数栈的栈顶，和DEEPExecEnv中的sp相同。
// TODO: 是否可以直接使用DEEPExecEnv中的sp，然后移除DEEPInterpFrame中的sp？
//
// type:
// 帧类型。用于区分当前帧是函数帧还是block帧等。
//
// local_vars:
// 局部变量数组。用于存储当前函数的局部变量，和DEEPExecEnv中的local_vars相同。
// TODO: 是否可以直接使用DEEPExecEnv中的local_vars，然后移除DEEPInterpFrame中的local_vars？
// 答：似乎不行，因为不知道每个变量多长，所以离开了当前函数实例后，就无法知道局部变量的位置。
// 或许可以把当前函数实例也放在DEEPExecEnv中？
typedef struct DEEPInterpFrame {
    struct DEEPInterpFrame *prev_func_frame;
    struct DEEPFunction *function;
    uint8_t *sp;
    DEEPFrameType type;
    uint8_t *local_vars;
} DEEPInterpFrame;

// 操作数栈
// TODO：似乎不需要该类型，因为DEEPExecEnv中的sp和sp_end已经足够了。
typedef struct DEEPStack {
    int32_t capacity; // 一般为编译时固定值
    uint8_t *sp;
    uint8_t *sp_end;
} DEEPStack;

// 控制栈。所有DEEP帧都存储在控制栈中。
typedef struct DEEPControlStack {
    int32_t capacity; // 一般为编译时固定值
    DEEPInterpFrame **frames;
    uint32_t current_frame_index; // TODO：检查是否Overflow
} DEEPControlStack;

// DEEP执行环境。用于存储DEEP虚拟机的运行时信息。
//
// cur_frame:
// 当前函数帧。指向当前正在执行的函数帧。
//
// sp_end:
// 指向操作数栈的栈底。对于每个帧都是固定值。
//
// sp:
// 操作数栈指针。指向当前操作数栈的栈顶。
//
// local_vars:
// 局部变量数组。用于存储当前函数的局部变量。
// TODO：光有这个没有用，详见DEEPInterpFrame中的local_vars注释。
//
// global_vars:
// 全局变量数组。用于存储全局变量。和local_var不同，目前所有global_vars都是定长的。
//
// memory:
// 在运行解释器时，解释器当前所指向的内存。
//
// control_stack:
// 控制栈。用于存储所有DEEP帧。详见DEEPControlStack注释。
//
// jump_depth:
// 跳转深度。用于控制跳转。
// 当遇到break指令时，会将jump_depth设置成对应跳出的深度。每次即将执行任何指令前，都会先
// 检查jump_depth是否为0，若不为0，则会跳出当前scope并将jump_depth减1。
// 类似的，当解释器执行完一个loop时，若jump_depth为0，则会继续循环，否则会跳出loop。
// 当遇到return指令时，会将jump_depth设置成-1，这样遇到任何loop都会直接跳出。
// 当解释器执行完一个函数时，会将jump_depth设置成0。
typedef struct DEEPExecEnv {
    struct DEEPInterpFrame *cur_frame;
    uint8_t *sp_end;
    uint8_t *sp;
    uint8_t *local_vars;
    uint64_t *global_vars;
    uint8_t *memory;
    DEEPControlStack *control_stack;
    int jump_depth;
} DEEPExecEnv;

// 创建操作数栈
DEEPStack *stack_cons(void);

// 创建控制栈
DEEPControlStack *control_stack_cons(void);

// 销毁操作数栈
void stack_free(DEEPStack *stack);

// 销毁控制栈
void control_stack_free(DEEPControlStack *stack);

// 读一个帧（函数或block）的开始位置和长度（offset）
void read_block(uint8_t *ip, uint8_t **start, uint32_t *offset);

// 开始解释main函数
int64_t call_main(DEEPExecEnv *current_env, DEEPModule *module);

// 调用一个函数
void call_function(DEEPExecEnv *current_env, DEEPModule *module, int func_index);

// 进入一个block/loop, 需要提供这个block对应的DEEPFunction（我们把block也当作函数包装进去）
// 返回执行结束时的指令地址
uint8_t *enter_frame(DEEPExecEnv *current_env, DEEPModule *module, DEEPFunction *block, DEEPFrameType frame_type);

#endif /* _DEEP_INTERP_H */
