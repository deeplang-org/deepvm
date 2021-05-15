//
// Created by xj on 2021/3/30.
//
#ifndef _DEEP_OPCODE_H
#define _DEEP_OPCODE_H
enum DEEPOpcode {
    op_end = 0x0b,
    op_call = 0x10,
    //变量指令
    op_local_get = 0x20,
    op_local_set = 0x21,
    op_local_tee = 0x22,
    op_global_get = 0x23,
    op_global_set = 0x24,
    //内存指令
    i32_load = 0x28,
    i32_store = 0x36,
    //常数指令
    i32_const = 0x41,
    f32_const = 0x42,
    //测试指令
    i32_eqz = 0x54,
    //二元整数算数指令
    i32_add = 0x6A,
    i32_sub = 0x6B,
    i32_mul = 0x6C,
    i32_divs = 0x6D,
    i32_divu = 0x6E,
    i32_rems = 0x6F,
    i32_remu = 0x70,
    i32_and = 0x71,
    i32_or = 0x72,
    i32_xor = 0x73,
    i32_shl = 0x74,
    i32_shrs = 0x75,
    i32_shru = 0x76,
    i32_rotl = 0x77,
    i32_rotr = 0x78,
    //二元浮点数算数指令
    f32_add = 0x92,
    f32_sub = 0x93,
    f32_mul = 0x94,
    f32_div = 0x95,
    f32_min = 0x96,
    f32_max = 0x97,
    f32_copysign = 0x98,
    //一元算数指令
    i32_clz = 0x67,
    i32_ctz = 0x68,
    i32_popcnt = 0x69,
    f32_abs = 0x8b,
    f32_neg = 0x8c,
    f32_ceil = 0x8d,
    f32_floor = 0x8e,
    f32_trunc = 0x8f,
    f32_nearest = 0x90,
    f32_sqrt = 0x91,
    //比较指令
    i32_eq = 0x46,
    i32_ne = 0x47,
    i32_its = 0x48,
    i32_itu = 0x49,
    i32_gts = 0x4a,
    i32_gtu = 0x4b,
    i32_les = 0x4c,
    i32_leu = 0x4d,
    i32_ges = 0x4e,
    i32_geu = 0x4f,
};

#endif /* _DEEP_OPCODE_H */