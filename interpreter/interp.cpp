//
// Created by xj on 2021/3/30.
//

#include "interp.h"
#include "loader.cpp"
#include "opcode.h"
#include <stdio.h>
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
                ip++;
                uint32_t temp = read_leb_u32(&ip);
                pushU32(temp);
                break;
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
