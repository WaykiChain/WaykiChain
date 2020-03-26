#pragma once

#include <eosio/wasm_backend/opcodes.hpp>
#include <iostream>
#include <variant>

namespace eosio { namespace wasm_backend {
struct disassembly_visitor {
   void print(const std::string& s) {
      //std::string tb(tab_width, '\t');
      std::cout << s << '\n';
   }
   void operator()(unreachable_t) {
      print("unreachable");
   }
   void operator()(nop_t) {
      print("nop");
   }
   void operator()(end_t) {
      print("end");
      tab_width--;
   }
   void operator()(return__t) {
      print("return");
   }
   void operator()(block_t) {
      print("block");
      tab_width++;
   }
   void operator()(loop_t) {
      print("loop");
      tab_width++;
   }
   void operator()(if__t) {
      print("if");
      tab_width++;
   }
   void operator()(else__t) {
      print("else");
   }
   void operator()(br_t b) {
      print("br : "+std::to_string(b.data));
   }
   void operator()(br_if_t b) {
      print("br.if : "+std::to_string(b.data));
   }
   void operator()(br_table_t b) {
      print("br.table : "); //+std::to_string(b.data);
   }
   void operator()(call_t b) {
      print("call : "+std::to_string(b.index));
   }
   void operator()(call_indirect_t b) {
      print("call_indirect : "+std::to_string(b.index));
   }
   void operator()(drop_t b) {
      print("drop");
   }
   void operator()(select_t b) {
      print("select");
   }
   void operator()(get_local_t b) {
      print("get_local : "+std::to_string(b.index));
   }
   void operator()(set_local_t b) {
      print("set_local : "+std::to_string(b.index));
   }
   void operator()(tee_local_t b) {
      print("tee_local : "+std::to_string(b.index));
   }
   void operator()(get_global_t b) {
      print("get_global : "+std::to_string(b.index));
   }
   void operator()(set_global_t b) {
      print("set_global : "+std::to_string(b.index));
   }
   void operator()(i32_load_t b) {
      print("i32.load : "); //+std::to_string(b.index);
   }
   void operator()(i32_load8_s_t b) {
      print("i32.load8_s : "); //+std::to_string(b.index);
   }
   void operator()(i32_load16_s_t b) {
      print("i32.load16_s : "); //+std::to_string(b.index);
   }
   void operator()(i32_load8_u_t b) {
      print("i32.load8_u : "); //+std::to_string(b.index);
   }
   void operator()(i32_load16_u_t b) {
      print("i32.load16_u : "); //+std::to_string(b.index);
   }
   void operator()(i64_load_t b) {
      print("i64.load : "); //+std::to_string(b.index);
   }
   void operator()(i64_load8_s_t b) {
      print("i64.load8_s : "); //+std::to_string(b.index);
   }
   void operator()(i64_load16_s_t b) {
      print("i64.load16_s : "); //+std::to_string(b.index);
   }
   void operator()(i64_load32_s_t b) {
      print("i64.load32_s : "); //+std::to_string(b.index);
   }
   void operator()(i64_load8_u_t b) {
      print("i64.load8_u : "); //+std::to_string(b.index);
   }
   void operator()(i64_load16_u_t b) {
      print("i64.load16_u : "); //+std::to_string(b.index);
   }
   void operator()(i64_load32_u_t b) {
      print("i64.load32_u : "); //+std::to_string(b.index);
   }
   void operator()(f32_load_t b) {
      print("f32.load : "); //+std::to_string(b.index);
   }
   void operator()(f64_load_t b) {
      print("f64.load : "); //+std::to_string(b.index);
   }
   void operator()(i32_store_t b) {
      print("i32.store : "); //+std::to_string(b.index);
   }
   void operator()(i32_store8_t b) {
      print("i32.store8 : "); //+std::to_string(b.index);
   }
   void operator()(i32_store16_t b) {
      print("i32.store16 : "); //+std::to_string(b.index);
   }
   void operator()(i64_store_t b) {
      print("i64.store : "); //+std::to_string(b.index);
   }
   void operator()(i64_store8_t b) {
      print("i64.store8 : "); //+std::to_string(b.index);
   }
   void operator()(i64_store16_t b) {
      print("i64.store16 : "); //+std::to_string(b.index);
   }
   void operator()(i64_store32_t b) {
      print("i64.store32 : "); //+std::to_string(b.index);
   }
   void operator()(f32_store_t b) {
      print("f32.store : "); //+std::to_string(b.index);
   }
   void operator()(f64_store_t b) {
      print("f64.store : "); //+std::to_string(b.index);
   }
   void operator()(current_memory_t b) {
      print("current_memory ");
   }
   void operator()(grow_memory_t b) {
      print("grow_memory ");
   }
   void operator()(i32_const_t b) {
      print("i32.const : "+std::to_string(b.data));
   }
   void operator()(i64_const_t b) {
      print("i64.const : "+std::to_string(b.data));
   }
   void operator()(f32_const_t b) {
      print("f32.const : "+std::to_string(*((float*)&b.data)));
   }
   void operator()(f64_const_t b) {
      print("f64.const : "+std::to_string(*((double*)&b.data)));
   }
   void operator()(i32_eqz_t b) {
      print("i32.eqz");
   }
   void operator()(i32_eq_t b) {
      print("i32.eq");
   }
   void operator()(i32_ne_t b) {
      print("i32.ne");
   }
   void operator()(i32_lt_s_t b) {
      print("i32.lt_s");
   }
   void operator()(i32_lt_u_t b) {
      print("i32.lt_u");
   }
   void operator()(i32_le_s_t b) {
      print("i32.le_s");
   }
   void operator()(i32_le_u_t b) {
      print("i32.le_u");
   }
   void operator()(i32_gt_s_t b) {
      print("i32.gt_s");
   }
   void operator()(i32_gt_u_t b) {
      print("i32.gt_u");
   }
   void operator()(i32_ge_s_t b) {
      print("i32.ge_s");
   }
   void operator()(i32_ge_u_t b) {
      print("i32.ge_u");
   }
   void operator()(i64_eqz_t b) {
      print("i64.eqz");
   }
   void operator()(i64_eq_t b) {
      print("i64.eq");
   }
   void operator()(i64_ne_t b) {
      print("i64.ne");
   }
   void operator()(i64_lt_s_t b) {
      print("i64.lt_s");
   }
   void operator()(i64_lt_u_t b) {
      print("i64.lt_u");
   }
   void operator()(i64_le_s_t b) {
      print("i64.le_s");
   }
   void operator()(i64_le_u_t b) {
      print("i64.le_u");
   }
   void operator()(i64_gt_s_t b) {
      print("i64.gt_s");
   }
   void operator()(i64_gt_u_t b) {
      print("i64.gt_u");
   }
   void operator()(i64_ge_s_t b) {
      print("i64.ge_s");
   }
   void operator()(i64_ge_u_t b) {
      print("i64.ge_u");
   }
   void operator()(f32_eq_t b) {
      print("f32.eq");
   }
   void operator()(f32_ne_t b) {
      print("f32.ne");
   }
   void operator()(f32_lt_t b) {
      print("f32.lt");
   }
   void operator()(f32_gt_t b) {
      print("f32.gt");
   }
   void operator()(f32_le_t b) {
      print("f32.le");
   }
   void operator()(f32_ge_t b) {
      print("f32.ge");
   }
   void operator()(f64_eq_t b) {
      print("f64.eq");
   }
   void operator()(f64_ne_t b) {
      print("f64.ne");
   }
   void operator()(f64_lt_t b) {
      print("f64.lt");
   }
   void operator()(f64_gt_t b) {
      print("f64.gt");
   }
   void operator()(f64_le_t b) {
      print("f64.le");
   }
   void operator()(f64_ge_t b) {
      print("f64.ge");
   }
   void operator()(i32_clz_t) {
      print("i32.clz");
   }
   void operator()(i32_ctz_t) {
      print("i32.ctz");
   }
   void operator()(i32_popcnt_t) {
      print("i32.popcnt");
   }
   void operator()(i32_add_t) {
      print("i32.add");
   }
   void operator()(i32_sub_t) {
      print("i32.sub");
   }
   void operator()(i32_mul_t) {
      print("i32.mul");
   }
   void operator()(i32_div_s_t) {
      print("i32.div_s");
   }
   void operator()(i32_div_u_t) {
      print("i32.div_u");
   }
   void operator()(i32_rem_s_t) {
      print("i32.rem_s");
   }
   void operator()(i32_rem_u_t) {
      print("i32.rem_u");
   }
   void operator()(i32_and_t) {
      print("i32.and");
   }
   void operator()(i32_or_t) {
      print("i32.or");
   }
   void operator()(i32_xor_t) {
      print("i32.xor");
   }
   void operator()(i32_shl_t) {
      print("i32.shl");
   }
   void operator()(i32_shr_s_t) {
      print("i32.shr_s");
   }
   void operator()(i32_shr_u_t) {
      print("i32.shr_u");
   }
   void operator()(i32_rotl_t) {
      print("i32.rotl");
   }
   void operator()(i32_rotr_t) {
      print("i32.rotr");
   }
   void operator()(i64_clz_t) {
      print("i64.clz");
   }
   void operator()(i64_ctz_t) {
      print("i64.ctz");
   }
   void operator()(i64_popcnt_t) {
      print("i64.popcnt");
   }
   void operator()(i64_add_t) {
      print("i64.add");
   }
   void operator()(i64_sub_t) {
      print("i64.sub");
   }
   void operator()(i64_mul_t) {
      print("i64.mul");
   }
   void operator()(i64_div_s_t) {
      print("i64.div_s");
   }
   void operator()(i64_div_u_t) {
      print("i64.div_u");
   }
   void operator()(i64_rem_s_t) {
      print("i64.rem_s");
   }
   void operator()(i64_rem_u_t) {
      print("i64.rem_u");
   }
   void operator()(i64_and_t) {
      print("i64.and");
   }
   void operator()(i64_or_t) {
      print("i64.or");
   }
   void operator()(i64_xor_t) {
      print("i64.xor");
   }
   void operator()(i64_shl_t) {
      print("i64.shl");
   }
   void operator()(i64_shr_s_t) {
      print("i64.shr_s");
   }
   void operator()(i64_shr_u_t) {
      print("i64.shr_u");
   }
   void operator()(i64_rotl_t) {
      print("i64.rotl");
   }
   void operator()(i64_rotr_t) {
      print("i64.rotr");
   }
   void operator()(f32_abs_t) {
      print("f32.abs");
   }
   void operator()(f32_neg_t) {
      print("f32.neg");
   }
   void operator()(f32_ceil_t) {
      print("f32.ceil");
   }
   void operator()(f32_floor_t) {
      print("f32.floor");
   }
   void operator()(f32_trunc_t) {
      print("f32.trunc");
   }
   void operator()(f32_nearest_t) {
      print("f32.nearest");
   }
   void operator()(f32_sqrt_t) {
      print("f32.sqrt");
   }
   void operator()(f32_add_t) {
      print("f32.add");
   }
   void operator()(f32_sub_t) {
      print("f32.sub");
   }
   void operator()(f32_mul_t) {
      print("f32.mul");
   }
   void operator()(f32_div_t) {
      print("f32.div");
   }
   void operator()(f32_min_t) {
      print("f32.min");
   }
   void operator()(f32_max_t) {
      print("f32.max");
   }
   void operator()(f32_copysign_t) {
      print("f32.copysign");
   }
   void operator()(f64_abs_t) {
      print("f64.abs");
   }
   void operator()(f64_neg_t) {
      print("f64.neg");
   }
   void operator()(f64_ceil_t) {
      print("f64.ceil");
   }
   void operator()(f64_floor_t) {
      print("f64.floor");
   }
   void operator()(f64_trunc_t) {
      print("f64.trunc");
   }
   void operator()(f64_nearest_t) {
      print("f64.nearest");
   }
   void operator()(f64_sqrt_t) {
      print("f64.sqrt");
   }
   void operator()(f64_add_t) {
      print("f64.add");
   }
   void operator()(f64_sub_t) {
      print("f64.sub");
   }
   void operator()(f64_mul_t) {
      print("f64.mul");
   }
   void operator()(f64_div_t) {
      print("f64.div");
   }
   void operator()(f64_min_t) {
      print("f64.min");
   }
   void operator()(f64_max_t) {
      print("f64.max");
   }
   void operator()(f64_copysign_t) {
      print("f64.copysign");
   }
   void operator()(i32_wrap_i64_t) {
      print("i32.wrap_i64");
   }
   void operator()(i32_trunc_s_f32_t) {
      print("i32.trunc_s_f32");
   }
   void operator()(i32_trunc_u_f32_t) {
      print("i32.trunc_u_f32");
   }
   void operator()(i32_trunc_s_f64_t) {
      print("i32.trunc_s_f64");
   }
   void operator()(i32_trunc_u_f64_t) {
      print("i32.trunc_u_f64");
   }
   void operator()(i64_extend_s_i32_t) {
      print("i64.extend_s_i32");
   }
   void operator()(i64_extend_u_i32_t) {
      print("i64.extend_u_i32");
   }
   void operator()(i64_trunc_s_f32_t) {
      print("i64.trunc_s_f32");
   }
   void operator()(i64_trunc_u_f32_t) {
      print("i64.trunc_u_f32");
   }
   void operator()(i64_trunc_s_f64_t) {
      print("i64.trunc_s_f64");
   }
   void operator()(i64_trunc_u_f64_t) {
      print("i64.trunc_u_f64");
   }
   void operator()(f32_convert_s_i32_t) {
      print("f32.convert_s_i32");
   }
   void operator()(f32_convert_u_i32_t) {
      print("f32.convert_u_i32");
   }
   void operator()(f32_convert_s_i64_t) {
      print("f32.convert_s_i64");
   }
   void operator()(f32_convert_u_i64_t) {
      print("f32.convert_u_i64");
   }
   void operator()(f32_demote_f64_t) {
      print("f32.demote_f64");
   }
   void operator()(f64_convert_s_i32_t) {
      print("f64.convert_s_i32");
   }
   void operator()(f64_convert_u_i32_t) {
      print("f64.convert_u_i32");
   }
   void operator()(f64_convert_s_i64_t) {
      print("f64.convert_s_i64");
   }
   void operator()(f64_convert_u_i64_t) {
      print("f64.convert_u_i64");
   }
   void operator()(f64_promote_f32_t) {
      print("f64.promote_f32");
   }
   void operator()(i32_reinterpret_f32_t) {
      print("i32.reinterpret_f32");
   }
   void operator()(i64_reinterpret_f64_t) {
      print("i64.reinterpret_f64");
   }
   void operator()(f32_reinterpret_i32_t) {
      print("f32.reinterpret_i32");
   }
   void operator()(f64_reinterpret_i64_t) {
      print("f64.reinterpret_i64");
   }
   void operator()(error_t) {
      print("error");
   }

   uint32_t tab_width;
};

}} // ns eosio::wasm_backend
