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
//暂时只支持block
void read_block(uint8_t *ip, uint8_t **start, uint32_t *offset, bool search_for_else) {
    *start = ip;
    bool finish = false;
    uint8_t net_bracket = 1;
    while (!finish) {
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t)*ip;
        switch (opcode)
        {
        case op_end:
            ip++;
            net_bracket--;
            if (net_bracket == 0) {
                *offset = (uint32_t)ip - *(uint32_t *)start;
                finish = true;
            }
            break;
        //面对其他指令，只要正常阅读，不需要解释；这个函数只找end或else
        //不过现在甚至不考虑else；只在做block的情况
        //无立即数指令：
        case op_return:
        case op_nop:
        case op_unreachable:
        case i32_add:
        case i32_and:
        case i32_clz:
        case i32_ctz:
        case i32_divs:
        case i32_divu:
        case i32_eq:
        case i32_eqz:
        case i32_ges:
        case i32_geu:
        case i32_gts:
        case i32_gtu:
        case i32_its:
        case i32_itu:
        case i32_les:
        case i32_leu:
        case i32_mul:
        case i32_ne:
        case i32_or:
        case i32_popcnt: // 这个不确定是不是需要立即数，不知道是什么
        case i32_rems:
        case i32_rotl:
        case i32_rotr:
        case i32_shl:
        case i32_shrs:
        case i32_shru:
        case i32_sub:
        case i32_trunc_f32_s:
        case i32_trunc_f32_u:
        case i32_xor:
        case f32_abs:
        case f32_add:
        case f32_ceil:
        case f32_convert_i32_s:
        case f32_convert_i32_u:
        case f32_copysign:
        case f32_div:
        case f32_floor:
        case f32_max:
        case f32_min:
        case f32_mul:
        case f32_nearest:
        case f32_neg:
        case f32_sqrt:
        case f32_sub:
        case f32_trunc:
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
        case op_br:
        case op_br_if:
            ip++;
            read_leb_u32(&ip);
            break;
        //需要特殊考虑的指令：
        case op_block:
            ip++;
            READ_BYTE(ip);
            net_bracket++;
            break;
        default:
            deep_error("Unknown opcode %x!", opcode);
            exit(1);
        }
    }
}

//执行代码块指令
void exec_instructions(DEEPExecEnv *current_env, DEEPModule *module) {
    uint32_t *sp = current_env->cur_frame->sp;
    uint8_t *ip = current_env->cur_frame->function->code_begin;
    uint8_t *ip_end = ip + current_env->cur_frame->function->code_size;
    uint32_t *memory = current_env->memory;
    while (ip < ip_end) {
        //判断是否需要跳出
        if (current_env->jump_depth) {
            current_env->jump_depth--;
            break;
        }
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t) *ip;
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
            case op_block: {
                ip++;
                //暂时不支持WASM限制放开之后提供的多返s回值和传参数机制
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
                read_block(ip, &start, &offset, false);
                //为block创建DEEPFunction结构体
                DEEPFunction *block = deep_malloc(sizeof(DEEPFunction));
                //block的局部变量和当前function的一致
                block->local_var_types = current_env->cur_frame->function->local_var_types;
                block->local_vars_count = current_env->cur_frame->function->local_vars_count;
                block->code_begin = start;
                block->code_size = offset;
                block->func_type = type;
                //执行block
                current_env->sp = sp;
                ip = enter_frame(current_env, module, block, BLOCK_FRAME);
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
            case op_return: {
                //将jump_depth设为负数，代表正在执行return指令
                //等跳出最近的函数时，会将其重新设为0
                current_env->jump_depth = -1;
                ip++;
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
                current_env->local_vars[index] = *(sp - 1);
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
            /* 比较指令 */
            case i32_its: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushU32(b < a ? 1 : 0);
                break;
            }
            case i32_eqz: {
                ip++;
                uint32_t a = popU32();
                pushU32(a == 0 ? 1 : 0);
                break;
            }
            case i32_gts: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushU32(b > a ? 1 : 0);
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
            deep_error("Error: Operand stack overflow!\r\n");
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
    uint32_t ret_num = deepType->ret_count;

    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) deep_malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        deep_error("Malloc area for normal_frame error!");
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = func;
    frame->prev_func_frame = current_env->cur_frame;
    frame->type = FUNCTION_FRAME;
    frame->local_vars = (uint32_t *)deep_malloc(sizeof(uint32_t) * func->local_vars_count);
    //局部变量的空间已经在函数帧中分配好
    current_env->local_vars = frame->local_vars;
    uint32_t vars_temp = func->local_vars_count;
    while (vars_temp > 0) {
        uint32_t temp = *(--current_env->sp);
        current_env->local_vars[(vars_temp--) - 1] = temp;
    }

    //更新env中内容
    current_env->cur_frame = frame;
    //更新控制栈
    current_env->control_stack->current_frame_index++;
    current_env->control_stack->frames[
        current_env->control_stack->current_frame_index] = frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    
    exec_instructions(current_env, module);

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
    main_frame->local_vars = (uint32_t *)deep_malloc(sizeof(uint32_t) * main_func->local_vars_count);
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
    return *(current_env->sp - 1);
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
    exec_instructions(current_env, module);

    //执行完毕退栈
    current_env->control_stack->current_frame_index--;
    current_env->cur_frame = current_env->control_stack->frames[
        current_env->control_stack->current_frame_index];
    //不释放局部变量，因为这些变量原来的函数还要用

    //这里释放了frame，但是frame里面的DEEPFunction（作为参数传进来的）还没有释放，
    //需要caller处理
    deep_free(frame);

    return frame_type == LOOP_FRAME ? 
        block->code_begin : 
        block->code_begin + block->code_size;
}
