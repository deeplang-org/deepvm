<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:src/deep_interp.cpp
//
// Created by xj on 2021/3/30.
//

#include "interp.h"
#include <math.c>
#include "loader.cpp"
#include "opcode.h"
#include <stdio.h>
#include <math.h>
using namespace std;


#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++ 
#define pushU32(x) *(sp) = (uint32_t)(x);sp++ 

//执行代码块指令
void exec_instructions(DEEPExecEnv* env){
    int* sp = env->cur_frame->sp;
    char* ip = env->cur_frame->function->code_begin;
    char* ip_end = ip + env->cur_frame->function->code_size - 1;
    while(ip < ip_end){
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        int opcode = (int)*ip;
        switch(opcode){
            case op_end:{
                ip++;
                break;
            }
            case i32_eqz:{
                ip++;
                uint32_t a = popU32();
                popU32(a==0?1:0);
                break;
            }
            case i32_add:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a+b);
                break;
            }
            case i32_sub:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a-b);
                break;
            }
            case i32_mul:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a*b);
                break; 
            }
            case i32_divs:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a/b);
                break; 
            }
            case i32_divu:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a/b);
                break;
            }
            case i32_const:{
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                ip++;
                break;
            }
            case i32_rems:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a%b);
                break;
            }
            case i32_and:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a&b);
                break;
            }
            case i32_or:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a|b);
                break;
            }
            case i32_xor:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a^b);
                break;
            }
            case i32_shl:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a<<(b%32));
                break;
            }
            case i32_shrs:
            case i32_shru:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a>>(b%32));
                break;
            }
            case f32_add:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a+b);
                break;
            }
            case f32_sub:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a-b);
                break;
            }
            case f32_mul:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a*b);
                break;
            }
            case f32_div:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a/b);
                break;
            }
            case f32_min:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a<b?a:b);
                break;
            }
            case f32_max:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a>b?a:b);
                break; 
            }
            case f32_copysign:{
                ip++;
                float a =popF32();
                float b =popF32();
                pushF32(copysign(a,b));
                break;
            }
            case i32_eqz:{
                ip++;
                uint32_t a = popU32();
                pushU32(a==0?1:0);
            }
        }
        //检查操作数栈是否溢出
        if(sp > env->sp_end){
            printf("%s","warning! Operand stack overflow!");
            break;
        }
    }
    //更新env
    env->sp=sp;
}

//为main函数创建帧，执行main函数
int call_main(DEEPExecEnv* current_env, DEEPModule* module){

    //为main函数创建DEEPFunction
    DEEPFunction* main_func = module->func_section[4];//module.start_index记录了main函数索引

    //为main函数创建帧
    struct DEEPInterpFrame frame;
    //初始化
    frame.sp = current_env->sp;
    frame.function = main_func;

    //更新env中内容
    current_env->cur_frame = &frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env);

    //返回栈顶元素
    return  *(current_env->sp-1);
}

int main() {
    //创建操作数栈
    DEEPStack stack = stack_cons();

    //先声明环境并初始化
    DEEPExecEnv deep_env;
    DEEPExecEnv* current_env=&deep_env;
    current_env->sp_end=stack.sp_end;
    current_env->sp=stack.sp;

    //创建module，简单加减乘除只用到一个module
    DEEPModule* module = loader();//从loader中获取

    int ans = call_main(current_env,module);

    printf("%d",ans);

}
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"


#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++ 
#define pushU32(x) *(sp) = (uint32_t)(x);sp++ 

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack* stack_cons(void)
{
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    stack->capacity = STACK_CAPACITY;
    stack->sp= (uint32_t*) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv* env){
    uint32_t* sp = env->cur_frame->sp;
    uint8_t* ip = env->cur_frame->function->code_begin;
    uint8_t* ip_end = ip + env->cur_frame->function->code_size - 1;
    while(ip < ip_end){
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t)*ip;
        switch(opcode){
            case op_end:{
                ip++;
                break;
            }
            case i32_eqz:{
                ip++;
                uint32_t a = popU32();
                pushU32(a==0?1:0);
                break;
            }
            case i32_add:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a+b);
                break;
            }
            case i32_sub:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a-b);
                break;
            }
            case i32_mul:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a*b);
                break; 
            }
            case i32_divs:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a/b);
                break; 
            }
            case i32_divu:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a/b);
                break;
            }
            case i32_const:{
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                ip++;
                break;
            }
            case i32_rems:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a%b);
                break;
            }
            case i32_and:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a&b);
                break;
            }
            case i32_or:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a|b);
                break;
            }
            case i32_xor:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a^b);
                break;
            }
            case i32_shl:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a<<(b%32));
                break;
            }
            case i32_shrs:
            case i32_shru:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a>>(b%32));
                break;
            }
            case f32_add:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a+b);
                break;
            }
            case f32_sub:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a-b);
                break;
            }
            case f32_mul:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a*b);
                break;
            }
            case f32_div:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a/b);
                break;
            }
            case f32_min:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a<b?a:b);
                break;
            }
            case f32_max:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a>b?a:b);
                break; 
            }
            case f32_copysign:{
                ip++;
                float a =popF32();
                float b =popF32();
                pushF32(copysign(a,b));
                break;
            }
            default:break;
        }
        //检查操作数栈是否溢出
        if(sp > env->sp_end){
            printf("%s","warning! Operand stack overflow!");
            break;
        }
    }
    //更新env
    env->sp=sp;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv* current_env, DEEPModule* module)
{

    //为main函数创建DEEPFunction
    DEEPFunction* main_func = module->func_section[4];//module.start_index记录了main函数索引

    //为main函数创建帧
    struct DEEPInterpFrame frame;
    //初始化
    frame.sp = current_env->sp;
    frame.function = main_func;

    //更新env中内容
    current_env->cur_frame = &frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env);

    //返回栈顶元素
    return  *(current_env->sp-1);
}
>>>>>>> 779e4b0... [code]fix warnings and replace int with int32_t:src/deep_interp.c
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"


#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++ 
#define pushU32(x) *(sp) = (uint32_t)(x);sp++ 

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack* stack_cons(void)
{
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    stack->capacity = STACK_CAPACITY;
    stack->sp= (uint32_t*) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv* env){
    uint32_t* sp = env->cur_frame->sp;
    uint8_t* ip = env->cur_frame->function->code_begin;
    uint8_t* ip_end = ip + env->cur_frame->function->code_size - 1;
    while(ip < ip_end){
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t)*ip;
        switch(opcode){
            case op_end:{
                ip++;
                break;
            }
            case i32_eqz:{
                ip++;
                uint32_t a = popU32();
                pushU32(a==0?1:0);
                break;
            }
            case i32_add:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a+b);
                break;
            }
            case i32_sub:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a-b);
                break;
            }
            case i32_mul:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a*b);
                break; 
            }
            case i32_divs:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a/b);
                break; 
            }
            case i32_divu:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a/b);
                break;
            }
            case i32_const:{
                ip++;
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                ip++;
                break;
            }
            case i32_rems:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a%b);
                break;
            }
            case i32_and:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a&b);
                break;
            }
            case i32_or:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a|b);
                break;
            }
            case i32_xor:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a^b);
                break;
            }
            case i32_shl:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a<<(b%32));
                break;
            }
            case i32_shrs:
            case i32_shru:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a>>(b%32));
                break;
            }
            case f32_add:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a+b);
                break;
            }
            case f32_sub:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a-b);
                break;
            }
            case f32_mul:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a*b);
                break;
            }
            case f32_div:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a/b);
                break;
            }
            case f32_min:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a<b?a:b);
                break;
            }
            case f32_max:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a>b?a:b);
                break; 
            }
            case f32_copysign:{
                ip++;
                float a =popF32();
                float b =popF32();
                pushF32(copysign(a,b));
                break;
            }
            default:break;
        }
        //检查操作数栈是否溢出
        if(sp > env->sp_end){
            printf("%s","warning! Operand stack overflow!");
            break;
        }
    }
    //更新env
    env->sp=sp;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv* current_env, DEEPModule* module)
{

    //为main函数创建DEEPFunction
    DEEPFunction* main_func = module->func_section[4];//module.start_index记录了main函数索引

    //为main函数创建帧
    struct DEEPInterpFrame frame;
    //初始化
    frame.sp = current_env->sp;
    frame.function = main_func;

    //更新env中内容
    current_env->cur_frame = &frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env);

    //返回栈顶元素
    return  *(current_env->sp-1);
}
>>>>>>> 61d37e3... Add input file by command-line
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"


#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++
#define pushU32(x) *(sp) = (uint32_t)(x);sp++

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack* stack_cons(void)
{
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    if(stack == NULL){
        printf("Operand stack creation failed\r\n");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp= (uint32_t*) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv* env){
    uint32_t* sp = env->cur_frame->sp;
    uint8_t* ip = env->cur_frame->function->code_begin;
    uint8_t* ip_end = ip + env->cur_frame->function->code_size - 1;
    while(ip < ip_end){
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t)*ip;
        switch(opcode){
            case op_end:{
                ip++;
                break;
            }
            case i32_eqz:{
                ip++;
                uint32_t a = popU32();
                pushU32(a==0?1:0);
                break;
            }
            case i32_add:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a+b);
                break;
            }
            case i32_sub:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a-b);
                break;
            }
            case i32_mul:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a*b);
                break;
            }
            case i32_divs:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a/b);
                break;
            }
            case i32_divu:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a/b);
                break;
            }
            case i32_const:{
                ip++;
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                ip++;
                break;
            }
            case i32_rems:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a%b);
                break;
            }
            case i32_and:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a&b);
                break;
            }
            case i32_or:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a|b);
                break;
            }
            case i32_xor:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a^b);
                break;
            }
            case i32_shl:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a<<(b%32));
                break;
            }
            case i32_shrs:
            case i32_shru:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a>>(b%32));
                break;
            }
            case f32_add:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a+b);
                break;
            }
            case f32_sub:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a-b);
                break;
            }
            case f32_mul:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a*b);
                break;
            }
            case f32_div:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a/b);
                break;
            }
            case f32_min:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a<b?a:b);
                break;
            }
            case f32_max:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a>b?a:b);
                break;
            }
            case f32_copysign:{
                ip++;
                float a =popF32();
                float b =popF32();
                pushF32(copysign(a,b));
                break;
            }
            default:break;
        }
        //检查操作数栈是否溢出
        if(sp > env->sp_end){
            printf("warning! Operand stack overflow!\r\n");
            return;
        }
    }
    //更新env
    env->sp=sp;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv* current_env, DEEPModule* module)
{

    //create DEEPFunction for main
    //find the index of main
    int main_index = -1;
    int export_count = module->export_count;
    for (int i=0;i<export_count;i++){
        if(strcmp((module->export_section[i])->name,"main")==0){
            main_index = module->export_section[i]->index;
        }
    }
    if(main_index < 0){
        printf("the main function index failed!\r\n");
        return -1;
    }

    //为main函数创建DEEPFunction
    DEEPFunction* main_func = module->func_section[main_index];//module.start_index记录了main函数索引

    //为main函数创建帧
    struct DEEPInterpFrame frame;
    //初始化
    frame.sp = current_env->sp;
    frame.function = main_func;

    //更新env中内容
    current_env->cur_frame = &frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env);

    //返回栈顶元素
    return  *(current_env->sp-1);
}
>>>>>>> 9c27668... add find main index function
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"

#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++
#define pushU32(x) *(sp) = (uint32_t)(x);sp++

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack* stack_cons(void)
{
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    if(stack == NULL){
        printf("Operand stack creation failed!\r\n");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp= (uint32_t*) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    if(stack->sp == NULL){
        printf("Malloc area for stack error!\r\n");
    }
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv* current_env, DEEPModule* module){
    uint32_t* sp = current_env->cur_frame->sp;
    uint8_t* ip = current_env->cur_frame->function->code_begin;
    uint8_t* ip_end = ip + current_env->cur_frame->function->code_size - 1;
    while(ip < ip_end){
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t)*ip;
        switch(opcode){
            case op_end:{
                ip++;
                break;
            }
            case op_call:{
                ip++;
                uint32_t func_index = read_leb_u32(&ip);//被调用函数index
                //需要一个同步操作
                current_env->sp = sp;
                call_function(current_env, module, func_index);
                sp = current_env->sp;
                break;
            }
            case op_local_get:{
                ip++;
                uint32_t index = read_leb_u32(&ip);//local_get指令的立即数
                uint32_t a = current_env->vars[index];
                pushU32(current_env->vars[index]);
                break;
            }
            case i32_eqz:{
                ip++;
                uint32_t a = popU32();
                pushU32(a==0?1:0);
                break;
            }
            case i32_add:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a+b);
                break;
            }
            case i32_sub:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a-b);
                break;
            }
            case i32_mul:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a*b);
                break;
            }
            case i32_divs:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a/b);
                break;
            }
            case i32_divu:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a/b);
                break;
            }
            case i32_const:{
                ip++;
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                break;
            }
            case i32_rems:{
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a%b);
                break;
            }
            case i32_and:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a&b);
                break;
            }
            case i32_or:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a|b);
                break;
            }
            case i32_xor:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a^b);
                break;
            }
            case i32_shl:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a<<(b%32));
                break;
            }
            case i32_shrs:
            case i32_shru:{
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a>>(b%32));
                break;
            }
            case f32_add:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a+b);
                break;
            }
            case f32_sub:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a-b);
                break;
            }
            case f32_mul:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a*b);
                break;
            }
            case f32_div:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a/b);
                break;
            }
            case f32_min:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a<b?a:b);
                break;
            }
            case f32_max:{
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a>b?a:b);
                break;
            }
            case f32_copysign:{
                ip++;
                float a =popF32();
                float b =popF32();
                pushF32(copysign(a,b));
                break;
            }
            default:break;
        }
        //检查操作数栈是否溢出
        if(sp > current_env->sp_end){
            printf("warning! Operand stack overflow!\r\n");
            return;
        }
    }
    //更新env
    current_env->sp=sp;
}

//调用普通函数
void call_function(DEEPExecEnv* current_env, DEEPModule* module, int func_index){

    //为func函数创建DEEPFunction
    DEEPFunction* func = module->func_section[func_index];

    //函数类型
    DEEPType* deepType = func->func_type;
    int param_num = deepType->param_count;
    int ret_num = deepType->ret_count;

//    current_env->sp-=param_num;//操作数栈指针下移
    current_env->vars = (uint32_t)malloc(sizeof(uint32_t) * param_num);
    int vars_temp = param_num;
    while(vars_temp > 0){
        int temp = *(--current_env->sp);
        current_env->vars[(vars_temp--) - 1] = temp;
    }

    //局部变量组
    LocalVars** locals = func->localvars;

    //为func函数创建帧
    struct DEEPInterpFrame frame;
    //初始化
    frame.sp = current_env->sp;
    frame.function = func;
    frame.prev_frame = current_env->cur_frame;

    //更新env中内容
    current_env->cur_frame = &frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env,module);

    //执行完毕退栈
    current_env->cur_frame = frame.prev_frame;
//    free(&frame);
    return;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv* current_env, DEEPModule* module)
{

    //create DEEPFunction for main
    //find the index of main
    int main_index = -1;
    int export_count = module->export_count;
    for (int i=0;i<export_count;i++){
        if(strcmp((module->export_section[i])->name,"main")==0){
            main_index = module->export_section[i]->index;
        }
    }
    if(main_index < 0){
        printf("the main function index failed!\r\n");
        return -1;
    }

    //为main函数创建DEEPFunction
    DEEPFunction* main_func = module->func_section[main_index];//module.start_index记录了main函数索引

    //为main函数创建帧
    struct DEEPInterpFrame frame;
    //初始化
    frame.sp = current_env->sp;
    frame.function = main_func;

    //更新env中内容
    current_env->cur_frame = &frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env,module);

    //返回栈顶元素
    return  *(current_env->sp-1);
}
>>>>>>> aed2955... add "call" instruction
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"

#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++
#define pushU32(x) *(sp) = (uint32_t)(x);sp++

#define READ_VALUE(Type, p) \
    (p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack *stack_cons(void) {
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    if (stack == NULL) {
        printf("Operand stack creation failed!\r\n");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp = (uint32_t *) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    if (stack->sp == NULL) {
        printf("Malloc area for stack error!\r\n");
    }
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv *current_env, DEEPModule *module) {
    uint32_t *sp = current_env->cur_frame->sp;
    uint8_t *ip = current_env->cur_frame->function->code_begin;
    uint8_t *ip_end = ip + current_env->cur_frame->function->code_size - 1;
    while (ip < ip_end) {
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t) *ip;
        switch (opcode) {
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

                //内存指令:load,store
            case i32_load: {
                ip += 2;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t temp1 = popU32();
//                pushU32(data_list[temp1+offset]);

                break;
            }
            case i32_store: {
                ip += 2;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t temp1 = popU32();
                uint32_t temp2 = popU32();//temp2+offset为实际地址
//                data_list[temp2+offset] = temp1;

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
                pushU32(a - b);
                break;
            }
            case i32_mul: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a * b);
                break;
            }
            case i32_divs: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a / b);
                break;
            }
            case i32_divu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a / b);
                break;
            }
            case i32_const: {
                ip++;
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                break;
            }
            case i32_rems: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a % b);
                break;
            }
            case i32_and: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a & b);
                break;
            }
            case i32_or: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a | b);
                break;
            }
            case i32_xor: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a ^ b);
                break;
            }
            case i32_shl: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a << (b % 32));
                break;
            }
            case i32_shrs:
            case i32_shru: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a >> (b % 32));
                break;
            }
            case f32_add: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a + b);
                break;
            }
            case f32_sub: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a - b);
                break;
            }
            case f32_mul: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a * b);
                break;
            }
            case f32_div: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a / b);
                break;
            }
            case f32_min: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a < b ? a : b);
                break;
            }
            case f32_max: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a > b ? a : b);
                break;
            }
            case f32_copysign: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(copysign(a, b));
                break;
            }
            default:
                break;
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
    int param_num = deepType->param_count;
    int ret_num = deepType->ret_count;

//    current_env->sp-=param_num;//操作数栈指针下移
    current_env->local_vars = (uint32_t *) malloc(sizeof(uint32_t) * param_num);
    int vars_temp = param_num;
    while (vars_temp > 0) {
        int temp = *(--current_env->sp);
        current_env->local_vars[(vars_temp--) - 1] = temp;
    }

    //局部变量组
    LocalVars **locals = func->localvars;

    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        printf("Malloc area for normal_frame error!\r\n");
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = func;
    frame->prev_frame = current_env->cur_frame;

    //更新env中内容
    current_env->cur_frame = frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);

    //执行完毕退栈
    current_env->cur_frame = frame->prev_frame;
    //释放掉局部变量
    free(current_env->local_vars);
    free(frame);
    return;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv *current_env, DEEPModule *module) {

    //create DEEPFunction for main
    //find the index of main
    int main_index = -1;
    int export_count = module->export_count;
    for (int i = 0; i < export_count; i++) {
        if (strcmp((module->export_section[i])->name, "main") == 0) {
            main_index = module->export_section[i]->index;
        }
    }
    if (main_index < 0) {
        printf("the main function index failed!\r\n");
        return -1;
    }

    //为main函数创建DEEPFunction
    DEEPFunction *main_func = module->func_section[main_index];//module.start_index记录了main函数索引

    //为main函数创建帧
    DEEPInterpFrame *main_frame = (DEEPInterpFrame *) malloc(sizeof(struct DEEPInterpFrame));
    if (main_frame == NULL) {
        printf("Malloc area for main_frame error!\r\n");
    }
    //初始化
    main_frame->sp = current_env->sp;
    main_frame->function = main_func;

    //更新env中内容
    current_env->cur_frame = main_frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);
    free(main_frame);

    //返回栈顶元素
    return *(current_env->sp - 1);
}
>>>>>>> ec7849f... support call and get_local, fix some bugs
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"

#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++
#define pushU32(x) *(sp) = (uint32_t)(x);sp++

#define READ_VALUE(Type, p) \
    (p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack *stack_cons(void) {
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    if (stack == NULL) {
        printf("Operand stack creation failed!\r\n");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp = (uint32_t *) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    if (stack->sp == NULL) {
        printf("Malloc area for stack error!\r\n");
    }
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv *current_env, DEEPModule *module) {
    uint32_t *sp = current_env->cur_frame->sp;
    uint8_t *ip = current_env->cur_frame->function->code_begin;
    uint8_t *ip_end = ip + current_env->cur_frame->function->code_size - 1;
    uint32_t *memory = current_env ->memory;
    while (ip < ip_end) {
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t) *ip;
        switch (opcode) {
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
            case i32_load:{
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = read_mem32(memory+base,offset);
                pushU32(number);
            }
            case i32_store:{
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = popU32();
                write_mem32(memory+base,number,offset);
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
                pushU32(a - b);
                break;
            }
            case i32_mul: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a * b);
                break;
            }
            case i32_divs: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a / b);
                break;
            }
            case i32_divu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a / b);
                break;
            }
            case i32_const: {
                ip++;
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                break;
            }
            case i32_rems: {
                ip++;
                int32_t a = popS32();
                int32_t b = popS32();
                pushS32(a % b);
                break;
            }
            case i32_and: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a & b);
                break;
            }
            case i32_or: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a | b);
                break;
            }
            case i32_xor: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a ^ b);
                break;
            }
            case i32_shl: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a << (b % 32));
                break;
            }
            case i32_shrs:
            case i32_shru: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(a >> (b % 32));
                break;
            }
            case f32_add: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a + b);
                break;
            }
            case f32_sub: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a - b);
                break;
            }
            case f32_mul: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a * b);
                break;
            }
            case f32_div: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a / b);
                break;
            }
            case f32_min: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a < b ? a : b);
                break;
            }
            case f32_max: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(a > b ? a : b);
                break;
            }
            case f32_copysign: {
                ip++;
                float a = popF32();
                float b = popF32();
                pushF32(copysign(a, b));
                break;
            }
            default:
                break;
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
    int param_num = deepType->param_count;
    int ret_num = deepType->ret_count;

//    current_env->sp-=param_num;//操作数栈指针下移
    current_env->local_vars = (uint32_t *) malloc(sizeof(uint32_t) * param_num);
    int vars_temp = param_num;
    while (vars_temp > 0) {
        int temp = *(--current_env->sp);
        current_env->local_vars[(vars_temp--) - 1] = temp;
    }

    //局部变量组
    LocalVars **locals = func->localvars;

    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        printf("Malloc area for normal_frame error!\r\n");
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = func;
    frame->prev_frame = current_env->cur_frame;

    //更新env中内容
    current_env->cur_frame = frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);

    //执行完毕退栈
    current_env->cur_frame = frame->prev_frame;
    //释放掉局部变量
    free(current_env->local_vars);
    free(frame);
    return;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv *current_env, DEEPModule *module) {

    //create DEEPFunction for main
    //find the index of main
    int main_index = -1;
    int export_count = module->export_count;
    for (int i = 0; i < export_count; i++) {
        if (strcmp((module->export_section[i])->name, "main") == 0) {
            main_index = module->export_section[i]->index;
        }
    }
    if (main_index < 0) {
        printf("the main function index failed!\r\n");
        return -1;
    }

    //为main函数创建DEEPFunction
    DEEPFunction *main_func = module->func_section[main_index];//module.start_index记录了main函数索引

    //为main函数创建帧
    DEEPInterpFrame *main_frame = (DEEPInterpFrame *) malloc(sizeof(struct DEEPInterpFrame));
    if (main_frame == NULL) {
        printf("Malloc area for main_frame error!\r\n");
    }
    //初始化
    main_frame->sp = current_env->sp;
    main_frame->function = main_func;

    //更新env中内容
    current_env->cur_frame = main_frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);
    free(main_frame);

    //返回栈顶元素
    return *(current_env->sp - 1);
}
>>>>>>> b1f2dda... Add memory support
=======
//
// Created by xj on 2021/3/30.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "deep_interp.h"
#include "deep_loader.h"
#include "deep_opcode.h"

#define popS32() (int32_t)*(--sp)
#define popF32() (float)*(--sp)
#define popU32() (uint32_t)*(--sp)
#define pushS32(x)  *(sp) = (int32_t)(x);sp++
#define pushF32(x) *(sp) = (float)(x);sp++
#define pushU32(x) *(sp) = (uint32_t)(x);sp++

#define READ_VALUE(Type, p) \
    (p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

#define STACK_CAPACITY 100

//创建操作数栈
DEEPStack *stack_cons(void) {
    DEEPStack *stack = (DEEPStack *) malloc(sizeof(DEEPStack));
    if (stack == NULL) {
        printf("Operand stack creation failed!\r\n");
        return NULL;
    }
    stack->capacity = STACK_CAPACITY;
    stack->sp = (uint32_t *) malloc(sizeof(uint32_t) * STACK_CAPACITY);
    if (stack->sp == NULL) {
        printf("Malloc area for stack error!\r\n");
    }
    stack->sp_end = stack->sp + stack->capacity;
    return stack;
}

//执行代码块指令
void exec_instructions(DEEPExecEnv *current_env, DEEPModule *module) {
    uint32_t *sp = current_env->cur_frame->sp;
    uint8_t *ip = current_env->cur_frame->function->code_begin;
    uint8_t *ip_end = ip + current_env->cur_frame->function->code_size - 1;
    uint32_t *memory = current_env ->memory;
    while (ip < ip_end) {
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        uint32_t opcode = (uint32_t) *ip;
        switch (opcode) {
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
            case i32_load:{
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = read_mem32(memory+base,offset);
                pushU32(number);
            }
            case i32_store:{
                ip++;
                uint32_t base = popU32();
                uint32_t align = read_leb_u32(&ip);
                ip++;
                uint32_t offset = read_leb_u32(&ip);
                uint32_t number = popU32();
                write_mem32(memory+base,number,offset);
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
                pushS32(b / a);
                break;
            }
            case i32_divu: {
                ip++;
                uint32_t a = popU32();
                uint32_t b = popU32();
                pushU32(b / a);
                break;
            }
            case i32_const: {
                ip++;
                uint32_t temp = read_leb_u32(&ip);
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
                pushF32(b / a);
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
            default:
                break;
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
    int param_num = deepType->param_count;
    int ret_num = deepType->ret_count;

//    current_env->sp-=param_num;//操作数栈指针下移
    current_env->local_vars = (uint32_t *) malloc(sizeof(uint32_t) * param_num);
    int vars_temp = param_num;
    while (vars_temp > 0) {
        int temp = *(--current_env->sp);
        current_env->local_vars[(vars_temp--) - 1] = temp;
    }

    //局部变量组
    LocalVars **locals = func->localvars;

    //为func函数创建帧
    DEEPInterpFrame *frame = (DEEPInterpFrame *) malloc(sizeof(DEEPInterpFrame));
    if (frame == NULL) {
        printf("Malloc area for normal_frame error!\r\n");
    }
    //初始化
    frame->sp = current_env->sp;
    frame->function = func;
    frame->prev_frame = current_env->cur_frame;

    //更新env中内容
    current_env->cur_frame = frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);

    //执行完毕退栈
    current_env->cur_frame = frame->prev_frame;
    //释放掉局部变量
    free(current_env->local_vars);
    free(frame);
    return;
}

//为main函数创建帧，执行main函数
int32_t call_main(DEEPExecEnv *current_env, DEEPModule *module) {

    //create DEEPFunction for main
    //find the index of main
    int main_index = -1;
    int export_count = module->export_count;
    for (int i = 0; i < export_count; i++) {
        if (strcmp((module->export_section[i])->name, "main") == 0) {
            main_index = module->export_section[i]->index;
        }
    }
    if (main_index < 0) {
        printf("the main function index failed!\r\n");
        return -1;
    }

    //为main函数创建DEEPFunction
    DEEPFunction *main_func = module->func_section[main_index];//module.start_index记录了main函数索引

    //为main函数创建帧
    DEEPInterpFrame *main_frame = (DEEPInterpFrame *) malloc(sizeof(struct DEEPInterpFrame));
    if (main_frame == NULL) {
        printf("Malloc area for main_frame error!\r\n");
    }
    //初始化
    main_frame->sp = current_env->sp;
    main_frame->function = main_func;

    //更新env中内容
    current_env->cur_frame = main_frame;

    //执行frame中函数
    //sp要下移，栈顶元素即为函数参数
    exec_instructions(current_env, module);
    free(main_frame);

    //返回栈顶元素
    return *(current_env->sp - 1);
}
>>>>>>> 84e04bc... Fix bugs.The order of the operands is reversed...Sorry
