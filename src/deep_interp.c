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

#define READ_VALUE(Type, p) \
    (p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

#define STACK_CAPACITY 100
#define CONTROL_STACK_CAPACITY 100

// 安全除法
#define DIVIDE(TYPE, DIVIDEND, DIVISOR) \
(((TYPE)DIVISOR == 0) && \
    (deep_error("Arithmetic Error: Divide by Zero!"), exit(1), 0), \
        (TYPE)DIVIDEND / (TYPE)DIVISOR)

//创建操作数栈
DEEPStack *stack_cons(void) {
    DEEPStack *stack = (DEEPStack *) deep_malloc(sizeof(DEEPStack));
    if (stack == NULL) {
        deep_error("Operand stack creation failed!");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp = (uint32_t *) deep_malloc(sizeof(uint32_t) * STACK_CAPACITY);
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
    stack->frames = (DEEPInterpFrame **)deep_malloc(sizeof(DEEPInterpFrame *));
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
    free(stack->frames);
    free(stack);
}

//执行代码块指令
void exec_instructions(DEEPExecEnv *current_env, DEEPModule *module) {
    uint32_t *sp = current_env->cur_frame->sp;
    uint8_t *ip = current_env->cur_frame->function->code_begin;
    uint8_t *ip_end = ip + current_env->cur_frame->function->code_size - 1;
    uint32_t *memory = current_env->memory;

    while (ip < ip_end) {
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t) *ip;
        // printf("%x\n", opcode);
        switch (opcode) {
            case op_unreachable: {
                deep_error("Runtime Error: Unreachable!");
                exit(1);
                break;
            }
            case op_nop: {
                ip++;
                break;
            }
            case op_block: {
                break;
            }
            case op_br_if: {
                break;
            }
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
            //内存指令
            case i32_load: {
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = read_mem32(memory+base, offset);
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
                write_mem32(memory+base, number,offset);
                break;
            }
            case op_local_get: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_get指令的立即数
                uint32_t a = current_env->local_vars[index];
                pushU32(a);
                break;
            }
            case op_local_set: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_set指令的立即数
                current_env->local_vars[index] = popU32();
                break;
            }
            case op_local_tee: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_tee指令的立即数
                uint32_t num = *(sp - 1);
                current_env->local_vars[index] = num;
                break;
            }
            case op_global_get: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//global_get指令的立即数
                uint32_t a = current_env->global_vars[index];
                pushU32(a);
                break;
            }
            case op_global_set: {
                ip++;
                uint32_t index = read_leb_u32(&ip);//global_set指令的立即数
                current_env->global_vars[index] = popU32();
                break;
            }


                //算术指令
            case i32_eqz: {
                ip++;
                uint32_t a = popU32();
                pushU32(a == 0 ? 1 : 0);
                break;
            }
            case i32_add: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a + b);
                break;
            }
            case i32_sub: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b - a);
                break;
            }
            case i32_mul: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b * a);
                break;
            }
            case i32_divs: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(DIVIDE(int32_t, b, a));
                break;
            }
            case i32_divu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(DIVIDE(uint32_t, b, a));
                break;
            }
            case i32_const: {
                ip++;
                uint32_t temp = read_leb_i32(&ip);
                pushU32(temp);
                break;
            }
            case i32_rems: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(b % a);
                break;
            }
            case i32_and: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b & a);
                break;
            }
            case i32_or: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b | a);
                break;
            }
            case i32_xor: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b ^ a);
                break;
            }
            case i32_shl: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b << (a % 32));
                break;
            }
            case i32_shrs:
            case i32_shru: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b >> (a % 32));
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
                pushF32(copysign(b, a));
                break;
            }
            case f32_const: {
                ip++;
                float temp = read_ieee_32(&ip);
                pushF32(temp);
                break;
            }
            // 类型转换
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
            default:
                deep_error("Unknown opcode %x!", opcode);
                exit(1);
        }
        //检查操作数栈是否溢出
        if (sp > current_env->sp_end) {
            printf("warning! Operand stack overflow!\r\n");
            return;
        }
    }
    //更新env
    current_env->sp = sp;
}

//调用普通函数
void call_function(DEEPExecEnv *current_env, DEEPModule *module, int func_index) {

    //为func函数创建DEEPFunction
    DEEPFunction *func = module->func_section[func_index];

    //函数类型
    DEEPType *deepType = func->func_type;
    uint32_t param_num = deepType->param_count + func->local_vars_count;
    uint32_t ret_num = deepType->ret_count;

    // 分配局部变量的空间
    current_env->local_vars = (uint32_t *) deep_malloc(sizeof(uint32_t) * param_num);
    uint32_t vars_temp = param_num;
    while (vars_temp > 0) {
        uint32_t temp = *(--current_env->sp);
        current_env->local_vars[(vars_temp--) - 1] = temp;
    }

    //局部变量组
    LocalVars **locals = func->local_vars;

    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) deep_malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        deep_error("Malloc area for normal_frame error!");
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = func;
    frame->prev_func_frame = current_env->cur_frame;
    frame->type = function_frame;

    //更新env中内容
    current_env->cur_frame = frame;

    //更新控制栈
    current_env->control_stack->current_frame_index++;
    current_env->control_stack->frames[
        current_env->control_stack->current_frame_index] = frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);

    //执行完毕退栈
    current_env->cur_frame = frame->prev_func_frame;
    current_env->control_stack->current_frame_index--;
    //释放掉局部变量
    deep_free(current_env->local_vars);
    deep_free(frame);
    return;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv *current_env, DEEPModule *module) {

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
    DEEPFunction *main_func = module->func_section[main_index];//module.start_index记录了main函数索引
    
    //分配局部变量的空间
    current_env->local_vars = (uint32_t *)deep_malloc(sizeof(uint32_t) * main_func->local_vars_count);

    //为main函数创建帧
    DEEPInterpFrame *main_frame = (DEEPInterpFrame *) deep_malloc(sizeof(struct DEEPInterpFrame));
    if (main_frame == NULL) {
        deep_error("Malloc area for main_frame error!");
    }
    //初始化
    main_frame->sp = current_env->sp;
    main_frame->function = main_func;
    main_frame->type = function_frame;

    //更新env中内容
    current_env->cur_frame = main_frame;

    //更新控制栈
    current_env->control_stack->frames[
        current_env->control_stack->current_frame_index] = main_frame;
    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);
    deep_free(main_frame);

    //返回栈顶元素
    return *(current_env->sp - 1);
}
