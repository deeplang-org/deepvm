//
// Created by xj on 2021/3/30.
//

#include "interp.h"
#include "loader.cpp"
#include "opcode.h"
#include <iostream>
using namespace std;

//执行代码块指令
void exec_instructions(DEEPExecEnv* env){
    int* sp = env->cur_frame->sp;
    char* ip = env->cur_frame->function->code_begin;
    char* ip_end = ip + env->cur_frame->function->code_size - 1;
    while(ip < ip_end){
        //提取指令码
        //立即数存在的话，提取指令码时提取立即数
        int opcode = (int)*ip;
        int temp1,temp2;
        switch(opcode){
            case DEEP_OP_END:
                ip++;
                break;

            case DEEP_OP_I32_ADD:
                ip++;
                temp1 = *(--sp);
                *(sp-1) += temp1;
                break;

            case DEEP_OP_I32_SUB:
                ip++;
                temp1 = *(--sp);
                *(sp-1) += temp1;
                break;

            case DEEP_OP_I32_MUL:
                ip++;
                temp1 = *(--sp);
                *(sp-1) *= temp1;
                break;

            case DEEP_OP_I32_DIV_U:
                ip++;
                temp1 = *(--sp);
                temp2 = *(--sp);
                if(temp2 != 0){
                    *(sp++) = temp1/temp2;
                }
                break;

            case DEEP_OP_I32_CONST:
                //读取立即数
                ip++;
                int temp = read_leb_u32(&ip);
                //temp入栈
                *(sp++) = temp;
                break;
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
