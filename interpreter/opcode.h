//
// Created by xj on 2021/3/30.
//

#ifndef C___PROJECTS_OPCODE_H
#define C___PROJECTS_OPCODE_H

#endif //C___PROJECTS_OPCODE_H

enum DEEPOpcode {
    DEEP_OP_END           = 0x0b, //end

    DEEP_OP_GET_LOCAL     = 0x20, //get_local
    DEEP_OP_SET_LOCAL     = 0x21, //set_local
    DEEP_OP_TEE_LOCAL     = 0x22, //tee_local

    DEEP_OP_I32_CONST     = 0x41, // i32.const

    DEEP_OP_I32_ADD       = 0x6a, // i32.add
    DEEP_OP_I32_SUB       = 0x6b, // i32.sub
    DEEP_OP_I32_MUL       = 0x6c, // i32.mul
    DEEP_OP_I32_DIV_S     = 0x6d, // i32.div_s
    DEEP_OP_I32_DIV_U     = 0x6e, // i32.div_u
};