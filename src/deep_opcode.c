#include <stdint.h>
#include <stddef.h>

#include "deep_opcode.h"

const char *printDEEPOpcode(uint32_t opcode) {
    switch (opcode)
    {
    case op_unreachable:
        return "unreachable";
    case op_nop:
        return "nop";
    case op_block:
        return "block";
    case op_loop:
        return "loop";
    case op_if:
        return "if";
    case op_else:
        return "else";
    case op_end:
        return "end";
    case op_br:
        return "br";
    case op_br_if:
        return "br_if";
    case op_br_table:
        return "br_table";
    case op_return:
        return "return";
    case op_call:
        return "call";
    case op_drop:
        return "drop";
    case op_select:
        return "select";
    case op_local_get:
        return "local.get";
    case op_local_set:
        return "local.set";
    case op_local_tee:
        return "local.tee";
    case op_global_get:
        return "global.get";
    case op_global_set:
        return "global.set";
    case i32_load:
        return "i32.load";
    case i64_load:
        return "i64.load";
    case f32_load:
        return "f32.load";
    case f64_load:
        return "f64.load";
    case i32_store:
        return "i32.store";
    case i64_store:
        return "i64.store";
    case f32_store:
        return "f32.store";
    case f64_store:
        return "f64.store";
    case i32_const:
        return "i32.const";
    case i64_const:
        return "i64.const";
    case f32_const:
        return "f32.const";
    case f64_const:
        return "f64.const";
    case i32_eqz:
        return "i32.eqz";
    case i32_eq:
        return "i32.eq";
    case i32_ne:
        return "i32.ne";
    case i32_lts:
        return "i32.lts";
    case i32_ltu:
        return "i32.ltu";
    case i32_gts:
        return "i32.gts";
    case i32_gtu:
        return "i32.gtu";
    case i32_les:
        return "i32.les";
    case i32_leu:
        return "i32.leu";
    case i32_ges:
        return "i32.ges";
    case i32_geu:
        return "i32.geu";
    case i64_eqz:
        return "i64.eqz";
    case i64_eq:
        return "i64.eq";
    case i64_ne:
        return "i64.ne";
    case i64_lts:
        return "i64.lts";
    case i64_ltu:
        return "i64.ltu";
    case i64_gts:
        return "i64.gts";
    case i64_gtu:
        return "i64.gtu";
    case i64_les:
        return "i64.les";
    case i64_leu:
        return "i64.leu";
    case i64_ges:
        return "i64.ges";
    case i64_geu:
        return "i64.geu";
    case f32_eq:
        return "f32.eq";
    case f32_ne:
        return "f32.ne";
    case f32_lt:
        return "f32.lt";
    case f32_gt:
        return "f32.gt";
    case f32_le:
        return "f32.le";
    case f32_ge:
        return "f32.ge";
    case f64_eq:
        return "f64.eq";
    case f64_ne:
        return "f64.ne";
    case f64_lt:
        return "f64.lt";
    case f64_gt:
        return "f64.gt";
    case f64_le:
        return "f64.le";
    case f64_ge:
        return "f64.ge";
    case i32_clz:
        return "i32.clz";
    case i32_ctz:
        return "i32.ctz";
    case i32_popcnt:
        return "i32.popcnt";
    case i32_add:
        return "i32.add";
    case i32_sub:
        return "i32.sub";
    case i32_mul:
        return "i32.mul";
    case i32_divs:
        return "i32.divs";
    case i32_divu:
        return "i32.divu";
    case i32_rems:
        return "i32.rems";
    case i32_remu:
        return "i32.remu";
    case i32_and:
        return "i32.and";
    case i32_or:
        return "i32.or";
    case i32_xor:
        return "i32.xor";
    case i32_shl:
        return "i32.shl";
    case i32_shrs:
        return "i32.shrs";
    case i32_shru:
        return "i32.shru";
    case i32_rotl:
        return "i32.rotl";
    case i32_rotr:
        return "i32.rotr";
    case i64_clz:
        return "i64.clz";
    case i64_ctz:
        return "i64.ctz";
    case i64_popcnt:
        return "i64.popcnt";
    case i64_add:
        return "i64.add";
    case i64_sub:
        return "i64.sub";
    case i64_mul:
        return "i64.mul";
    case i64_divs:
        return "i64.divs";
    case i64_divu:
        return "i64.divu";
    case i64_rems:
        return "i64.rems";
    case i64_remu:
        return "i64.remu";
    case i64_and:
        return "i64.and";
    case i64_or:
        return "i64.or";
    case i64_xor:
        return "i64.xor";
    case i64_shl:
        return "i64.shl";
    case i64_shrs:
        return "i64.shrs";
    case i64_shru:
        return "i64.shru";
    case i64_rotl:
        return "i64.rotl";
    case i64_rotr:
        return "i64.rotr";
    case f32_abs:
        return "f32.abs";
    case f32_neg:
        return "f32.neg";
    case f32_ceil:
        return "f32.ceil";
    case f32_floor:
        return "f32.floor";
    case f32_trunc:
        return "f32.trunc";
    case f32_nearest:
        return "f32.nearest";
    case f32_sqrt:
        return "f32.sqrt";
    case f32_add:
        return "f32.add";
    case f32_sub:
        return "f32.sub";
    case f32_mul:
        return "f32.mul";
    case f32_div:
        return "f32.div";
    case f32_min:
        return "f32.min";
    case f32_max:
        return "f32.max";
    case f32_copysign:
        return "f32.copysign";
    case f64_abs:
        return "f64.abs";
    case f64_neg:
        return "f64.neg";
    case f64_ceil:
        return "f64.ceil";
    case f64_floor:
        return "f64.floor";
    case f64_trunc:
        return "f64.trunc";
    case f64_nearest:
        return "f64.nearest";
    case f64_sqrt:
        return "f64.sqrt";
    case f64_add:
        return "f64.add";
    case f64_sub:
        return "f64.sub";
    case f64_mul:
        return "f64.mul";
    case f64_div:
        return "f64.div";
    case f64_min:
        return "f64.min";
    case f64_max:
        return "f64.max";
    case f64_copysign:
        return "f64.copysign";
    case i32_wrap_i64:
        return "i32.wrap_i64";
    case i32_trunc_f32_s:
        return "i32.trunc_f32_s";
    case i32_trunc_f32_u:
        return "i32.trunc_f32_u";
    case i32_trunc_f64_s:
        return "i32.trunc_f64_s";
    case i32_trunc_f64_u:
        return "i32.trunc_f64_u";
    case i64_extend_i32_s:
        return "i64.extend_i32_s";
    case i64_extend_i32_u:
        return "i64.extend_i32_u";
    case i64_trunc_f32_s:
        return "i64.trunc_f32_s";
    case i64_trunc_f32_u:
        return "i64.trunc_f32_u";
    case i64_trunc_f64_s:
        return "i64.trunc_f64_s";
    case i64_trunc_f64_u:
        return "i64.trunc_f64_u";
    case f32_convert_i32_s:
        return "f32.convert_i32_s";
    case f32_convert_i32_u:
        return "f32.convert_i32_u";
    case f32_convert_i64_s:
        return "f32.convert_i64_s";
    case f32_convert_i64_u:
        return "f32.convert_i64_u";
    case f32_demote_f64:
        return "f32.demote_f64";
    case f64_convert_i32_s:
        return "f64.convert_i32_s";
    case f64_convert_i32_u:
        return "f64.convert_i32_u";
    case f64_convert_i64_s:
        return "f64.convert_i64_s";
    case f64_convert_i64_u:
        return "f64.convert_i64_u";
    case f64_promote_f32:
        return "f64.promote_f32";
    case i32_reinterpret_f32:
        return "i32.reinterpret_f32";
    case i64_reinterpret_f64:
        return "i64.reinterpret_f64";
    case f32_reinterpret_i32:
        return "f32.reinterpret_i32";
    case f64_reinterpret_i64:
        return "f64.reinterpret_i64";
    default:
        return NULL;
    }
}
