#pragma once

#include <cstdint>

/* clang-format off */
#define EOS_VM_CONTROL_FLOW_OPS(opcode_macro)   \
   opcode_macro(unreachable, 0x00)              \
   opcode_macro(nop, 0x01)                      \
   opcode_macro(block, 0x02)                    \
   opcode_macro(loop, 0x03)                     \
   opcode_macro(if_, 0x04)                      \
   opcode_macro(else_, 0x05)                    \
   opcode_macro(padding_cf_0, 0x06)             \
   opcode_macro(padding_cf_1, 0x07)             \
   opcode_macro(padding_cf_2, 0x08)             \
   opcode_macro(padding_cf_3, 0x09)             \
   opcode_macro(padding_cf_4, 0x0A)             \
   opcode_macro(end, 0x0B)                      \
   opcode_macro(br, 0x0C)                       \
   opcode_macro(br_if, 0x0D)
#define EOS_VM_BR_TABLE_OP(opcode_macro)        \
   opcode_macro(br_table, 0x0E)
#define EOS_VM_RETURN_OP(opcode_macro)          \
   opcode_macro(return_, 0x0F)
#define EOS_VM_CALL_OPS(opcode_macro)           \
   opcode_macro(call, 0x10)                     \
   opcode_macro(call_indirect, 0x11)            \
   opcode_macro(br_table_data, 0x12)
#define EOS_VM_CALL_IMM_OPS(opcode_macro)       \
   opcode_macro(call_imm, 0x13)                 \
   opcode_macro(call_indirect_imm, 0x14)        \
   opcode_macro(padding_call_1, 0x15)           \
   opcode_macro(padding_call_2, 0x16)           \
   opcode_macro(padding_call_3, 0x17)           \
   opcode_macro(padding_call_4, 0x18)           \
   opcode_macro(padding_call_5, 0x19)
#define EOS_VM_PARAMETRIC_OPS(opcode_macro)     \
   opcode_macro(drop, 0x1A)                     \
   opcode_macro(select, 0x1B)                   \
   opcode_macro(padding_param_0, 0x1C)          \
   opcode_macro(padding_param_1, 0x1D)          \
   opcode_macro(padding_param_2, 0x1E)          \
   opcode_macro(padding_param_3, 0x1F)
#define EOS_VM_VARIABLE_ACCESS_OPS(opcode_macro)\
   opcode_macro(get_local, 0x20)                \
   opcode_macro(set_local, 0x21)                \
   opcode_macro(tee_local, 0x22)                \
   opcode_macro(get_global, 0x23)               \
   opcode_macro(set_global, 0x24)               \
   opcode_macro(padding_va_0, 0x25)             \
   opcode_macro(padding_va_1, 0x26)             \
   opcode_macro(padding_va_2, 0x27)
#define EOS_VM_MEMORY_OPS(opcode_macro)         \
   opcode_macro(i32_load, 0x28)                 \
   opcode_macro(i64_load, 0x29)                 \
   opcode_macro(f32_load, 0x2A)                 \
   opcode_macro(f64_load, 0x2B)                 \
   opcode_macro(i32_load8_s, 0x2C)              \
   opcode_macro(i32_load8_u, 0x2D)              \
   opcode_macro(i32_load16_s, 0x2E)             \
   opcode_macro(i32_load16_u, 0x2F)             \
   opcode_macro(i64_load8_s, 0x30)              \
   opcode_macro(i64_load8_u, 0x31)              \
   opcode_macro(i64_load16_s, 0x32)             \
   opcode_macro(i64_load16_u, 0x33)             \
   opcode_macro(i64_load32_s, 0x34)             \
   opcode_macro(i64_load32_u, 0x35)             \
   opcode_macro(i32_store, 0x36)                \
   opcode_macro(i64_store, 0x37)                \
   opcode_macro(f32_store, 0x38)                \
   opcode_macro(f64_store, 0x39)                \
   opcode_macro(i32_store8, 0x3A)               \
   opcode_macro(i32_store16, 0x3B)              \
   opcode_macro(i64_store8, 0x3C)               \
   opcode_macro(i64_store16, 0x3D)              \
   opcode_macro(i64_store32, 0x3E)              \
   opcode_macro(current_memory, 0x3F)           \
   opcode_macro(grow_memory, 0x40)
#define EOS_VM_I32_CONSTANT_OPS(opcode_macro)   \
   opcode_macro(i32_const, 0x41)
#define EOS_VM_I64_CONSTANT_OPS(opcode_macro)   \
   opcode_macro(i64_const, 0x42)
#define EOS_VM_F32_CONSTANT_OPS(opcode_macro)   \
   opcode_macro(f32_const, 0x43)
#define EOS_VM_F64_CONSTANT_OPS(opcode_macro)   \
   opcode_macro(f64_const, 0x44)
#define EOS_VM_COMPARISON_OPS(opcode_macro)     \
   opcode_macro(i32_eqz, 0x45)                  \
   opcode_macro(i32_eq, 0x46)                   \
   opcode_macro(i32_ne, 0x47)                   \
   opcode_macro(i32_lt_s, 0x48)                 \
   opcode_macro(i32_lt_u, 0x49)                 \
   opcode_macro(i32_gt_s, 0x4A)                 \
   opcode_macro(i32_gt_u, 0x4B)                 \
   opcode_macro(i32_le_s, 0x4C)                 \
   opcode_macro(i32_le_u, 0x4D)                 \
   opcode_macro(i32_ge_s, 0x4E)                 \
   opcode_macro(i32_ge_u, 0x4F)                 \
   opcode_macro(i64_eqz, 0x50)                  \
   opcode_macro(i64_eq, 0x51)                   \
   opcode_macro(i64_ne, 0x52)                   \
   opcode_macro(i64_lt_s, 0x53)                 \
   opcode_macro(i64_lt_u, 0x54)                 \
   opcode_macro(i64_gt_s, 0x55)                 \
   opcode_macro(i64_gt_u, 0x56)                 \
   opcode_macro(i64_le_s, 0x57)                 \
   opcode_macro(i64_le_u, 0x58)                 \
   opcode_macro(i64_ge_s, 0x59)                 \
   opcode_macro(i64_ge_u, 0x5A)                 \
   opcode_macro(f32_eq, 0x5B)                   \
   opcode_macro(f32_ne, 0x5C)                   \
   opcode_macro(f32_lt, 0x5D)                   \
   opcode_macro(f32_gt, 0x5E)                   \
   opcode_macro(f32_le, 0x5F)                   \
   opcode_macro(f32_ge, 0x60)                   \
   opcode_macro(f64_eq, 0x61)                   \
   opcode_macro(f64_ne, 0x62)                   \
   opcode_macro(f64_lt, 0x63)                   \
   opcode_macro(f64_gt, 0x64)                   \
   opcode_macro(f64_le, 0x65)                   \
   opcode_macro(f64_ge, 0x66)
#define EOS_VM_NUMERIC_OPS(opcode_macro)        \
   opcode_macro(i32_clz, 0x67)                  \
   opcode_macro(i32_ctz, 0x68)                  \
   opcode_macro(i32_popcnt, 0x69)               \
   opcode_macro(i32_add, 0x6A)                  \
   opcode_macro(i32_sub, 0x6B)                  \
   opcode_macro(i32_mul, 0x6C)                  \
   opcode_macro(i32_div_s, 0x6D)                \
   opcode_macro(i32_div_u, 0x6E)                \
   opcode_macro(i32_rem_s, 0x6F)                \
   opcode_macro(i32_rem_u, 0x70)                \
   opcode_macro(i32_and, 0x71)                  \
   opcode_macro(i32_or, 0x72)                   \
   opcode_macro(i32_xor, 0x73)                  \
   opcode_macro(i32_shl, 0x74)                  \
   opcode_macro(i32_shr_s, 0x75)                \
   opcode_macro(i32_shr_u, 0x76)                \
   opcode_macro(i32_rotl, 0x77)                 \
   opcode_macro(i32_rotr, 0x78)                 \
   opcode_macro(i64_clz, 0x79)                  \
   opcode_macro(i64_ctz, 0x7A)                  \
   opcode_macro(i64_popcnt, 0x7B)               \
   opcode_macro(i64_add, 0x7C)                  \
   opcode_macro(i64_sub, 0x7D)                  \
   opcode_macro(i64_mul, 0x7E)                  \
   opcode_macro(i64_div_s, 0x7F)                \
   opcode_macro(i64_div_u, 0x80)                \
   opcode_macro(i64_rem_s, 0x81)                \
   opcode_macro(i64_rem_u, 0x82)                \
   opcode_macro(i64_and, 0x83)                  \
   opcode_macro(i64_or, 0x84)                   \
   opcode_macro(i64_xor, 0x85)                  \
   opcode_macro(i64_shl, 0x86)                  \
   opcode_macro(i64_shr_s, 0x87)                \
   opcode_macro(i64_shr_u, 0x88)                \
   opcode_macro(i64_rotl, 0x89)                 \
   opcode_macro(i64_rotr, 0x8A)                 \
   opcode_macro(f32_abs, 0x8B)                  \
   opcode_macro(f32_neg, 0x8C)                  \
   opcode_macro(f32_ceil, 0x8D)                 \
   opcode_macro(f32_floor, 0x8E)                \
   opcode_macro(f32_trunc, 0x8F)                \
   opcode_macro(f32_nearest, 0x90)              \
   opcode_macro(f32_sqrt, 0x91)                 \
   opcode_macro(f32_add, 0x92)                  \
   opcode_macro(f32_sub, 0x93)                  \
   opcode_macro(f32_mul, 0x94)                  \
   opcode_macro(f32_div, 0x95)                  \
   opcode_macro(f32_min, 0x96)                  \
   opcode_macro(f32_max, 0x97)                  \
   opcode_macro(f32_copysign, 0x98)             \
   opcode_macro(f64_abs, 0x99)                  \
   opcode_macro(f64_neg, 0x9A)                  \
   opcode_macro(f64_ceil, 0x9B)                 \
   opcode_macro(f64_floor, 0x9C)                \
   opcode_macro(f64_trunc, 0x9D)                \
   opcode_macro(f64_nearest, 0x9E)              \
   opcode_macro(f64_sqrt, 0x9F)                 \
   opcode_macro(f64_add, 0xA0)                  \
   opcode_macro(f64_sub, 0xA1)                  \
   opcode_macro(f64_mul, 0xA2)                  \
   opcode_macro(f64_div, 0xA3)                  \
   opcode_macro(f64_min, 0xA4)                  \
   opcode_macro(f64_max, 0xA5)                  \
   opcode_macro(f64_copysign, 0xA6)
#define EOS_VM_CONVERSION_OPS(opcode_macro)     \
   opcode_macro(i32_wrap_i64, 0xA7)             \
   opcode_macro(i32_trunc_s_f32, 0xA8)          \
   opcode_macro(i32_trunc_u_f32, 0xA9)          \
   opcode_macro(i32_trunc_s_f64, 0xAA)          \
   opcode_macro(i32_trunc_u_f64, 0xAB)          \
   opcode_macro(i64_extend_s_i32, 0xAC)         \
   opcode_macro(i64_extend_u_i32, 0xAD)         \
   opcode_macro(i64_trunc_s_f32, 0xAE)          \
   opcode_macro(i64_trunc_u_f32, 0xAF)          \
   opcode_macro(i64_trunc_s_f64, 0xB0)          \
   opcode_macro(i64_trunc_u_f64, 0xB1)          \
   opcode_macro(f32_convert_s_i32, 0xB2)        \
   opcode_macro(f32_convert_u_i32, 0xB3)        \
   opcode_macro(f32_convert_s_i64, 0xB4)        \
   opcode_macro(f32_convert_u_i64, 0xB5)        \
   opcode_macro(f32_demote_f64, 0xB6)           \
   opcode_macro(f64_convert_s_i32, 0xB7)        \
   opcode_macro(f64_convert_u_i32, 0xB8)        \
   opcode_macro(f64_convert_s_i64, 0xB9)        \
   opcode_macro(f64_convert_u_i64, 0xBA)        \
   opcode_macro(f64_promote_f32, 0xBB)          \
   opcode_macro(i32_reinterpret_f32, 0xBC)      \
   opcode_macro(i64_reinterpret_f64, 0xBD)      \
   opcode_macro(f32_reinterpret_i32, 0xBE)      \
   opcode_macro(f64_reinterpret_i64, 0xBF)
#define EOS_VM_EXIT_OP(opcode_macro)            \
   opcode_macro(exit, 0xC0)
#define EOS_VM_EMPTY_OPS(opcode_macro)          \
   opcode_macro(empty0xC1, 0xC1)                \
   opcode_macro(empty0xC2, 0xC2)                \
   opcode_macro(empty0xC3, 0xC3)                \
   opcode_macro(empty0xC4, 0xC4)                \
   opcode_macro(empty0xC5, 0xC5)                \
   opcode_macro(empty0xC6, 0xC6)                \
   opcode_macro(empty0xC7, 0xC7)                \
   opcode_macro(empty0xC8, 0xC8)                \
   opcode_macro(empty0xC9, 0xC9)                \
   opcode_macro(empty0xCA, 0xCA)                \
   opcode_macro(empty0xCB, 0xCB)                \
   opcode_macro(empty0xCC, 0xCC)                \
   opcode_macro(empty0xCD, 0xCD)                \
   opcode_macro(empty0xCE, 0xCE)                \
   opcode_macro(empty0xCF, 0xCF)                \
   opcode_macro(empty0xD0, 0xD0)                \
   opcode_macro(empty0xD1, 0xD1)                \
   opcode_macro(empty0xD2, 0xD2)                \
   opcode_macro(empty0xD3, 0xD3)                \
   opcode_macro(empty0xD4, 0xD4)                \
   opcode_macro(empty0xD5, 0xD5)                \
   opcode_macro(empty0xD6, 0xD6)                \
   opcode_macro(empty0xD7, 0xD7)                \
   opcode_macro(empty0xD8, 0xD8)                \
   opcode_macro(empty0xD9, 0xD9)                \
   opcode_macro(empty0xDA, 0xDA)                \
   opcode_macro(empty0xDB, 0xDB)                \
   opcode_macro(empty0xDC, 0xDC)                \
   opcode_macro(empty0xDD, 0xDD)                \
   opcode_macro(empty0xDE, 0xDE)                \
   opcode_macro(empty0xDF, 0xDF)                \
   opcode_macro(empty0xE0, 0xE0)                \
   opcode_macro(empty0xE1, 0xE1)                \
   opcode_macro(empty0xE2, 0xE2)                \
   opcode_macro(empty0xE3, 0xE3)                \
   opcode_macro(empty0xE4, 0xE4)                \
   opcode_macro(empty0xE5, 0xE5)                \
   opcode_macro(empty0xE6, 0xE6)                \
   opcode_macro(empty0xE7, 0xE7)                \
   opcode_macro(empty0xE8, 0xE8)                \
   opcode_macro(empty0xE9, 0xE9)                \
   opcode_macro(empty0xEA, 0xEA)                \
   opcode_macro(empty0xEB, 0xEB)                \
   opcode_macro(empty0xEC, 0xEC)                \
   opcode_macro(empty0xED, 0xED)                \
   opcode_macro(empty0xEE, 0xEE)                \
   opcode_macro(empty0xEF, 0xEF)                \
   opcode_macro(empty0xF0, 0xF0)                \
   opcode_macro(empty0xF1, 0xF1)                \
   opcode_macro(empty0xF2, 0xF2)                \
   opcode_macro(empty0xF3, 0xF3)                \
   opcode_macro(empty0xF4, 0xF4)                \
   opcode_macro(empty0xF5, 0xF5)                \
   opcode_macro(empty0xF6, 0xF6)                \
   opcode_macro(empty0xF7, 0xF7)                \
   opcode_macro(empty0xF8, 0xF8)                \
   opcode_macro(empty0xF9, 0xF9)                \
   opcode_macro(empty0xFA, 0xFA)                \
   opcode_macro(empty0xFB, 0xFB)                \
   opcode_macro(empty0xFC, 0xFC)                \
   opcode_macro(empty0xFD, 0xFD)                \
   opcode_macro(empty0xFE, 0xFE)
#define EOS_VM_ERROR_OPS(opcode_macro)          \
   opcode_macro(error, 0xFF)

/* clang-format on */

#define EOS_VM_CREATE_ENUM(name, code) name = code,

#define EOS_VM_CREATE_STRINGS(name, code) #name,

#define EOS_VM_CREATE_MAP(name, code) { code, #name },

#define EOS_VM_OPCODE_NAME_if_
#define EOS_VM_OPCODE_NAME_else_
#define EOS_VM_OPCODE_NAME_return_

#define EOS_VM_OPCODE_NAME_TEST() 1
#define EOS_VM_OPCODE_NAME_TEST_EOS_VM_OPCODE_NAME_TEST 0,
#define EOS_VM_OPCODE_NAME_TEST_1 1, ignore
#define EOS_VM_EXPAND(x) x
#define EOS_VM_CAT2(x, y) x ## y
#define EOS_VM_CAT(x, y) EOS_VM_CAT2(x, y)
#define EOS_VM_APPLY(f, args) f args
#define EOS_VM_FIX_OPCODE_NAME_0(name) name ## _t
#define EOS_VM_FIX_OPCODE_NAME_1(name) name ## t
#define EOS_VM_FIX_OPCODE_NAME(iskeyword, garbage) EOS_VM_FIX_OPCODE_NAME_ ## iskeyword

#define EOS_VM_OPCODE_T(name)                                                             \
   EOS_VM_APPLY(EOS_VM_FIX_OPCODE_NAME,                                                   \
      (EOS_VM_CAT(EOS_VM_OPCODE_NAME_TEST_,                                               \
           EOS_VM_EXPAND(EOS_VM_OPCODE_NAME_TEST EOS_VM_OPCODE_NAME_ ## name ()))))(name)

#define EOS_VM_CREATE_EXIT_TYPE(name, code)                                                                      \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      uint32_t pc;                                                                                                     \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_CONTROL_FLOW_TYPES(name, code)                                                                   \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() {}                                                                                       \
      EOS_VM_OPCODE_T(name)(uint32_t data) : data(data) {}                                                             \
      EOS_VM_OPCODE_T(name)(uint32_t d, uint32_t pc, uint16_t i, uint16_t oi)                                          \
        : data(d), pc(pc), index(i), op_index(oi) {}                                                                   \
      uint32_t data     = 0;                                                                                           \
      uint32_t pc       = 0;                                                                                           \
      uint16_t index    = 0;                                                                                           \
      uint16_t op_index = 0;                                                                                           \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_BR_TABLE_TYPE(name, code)                                                                        \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      struct elem_t { uint32_t pc; uint32_t stack_pop; };                                                              \
      elem_t* table;                                                                                                   \
      uint32_t  size;                                                                                                  \
      uint32_t  offset;                                                                                                \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_TYPES(name, code)                                                                                \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_CALL_TYPES(name, code)                                                                           \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      uint32_t index;                                                                                                  \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_CALL_IMM_TYPES(name, code)                                                                       \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      uint32_t index;                                                                                                  \
      uint16_t locals;                                                                                                 \
      uint16_t return_type;                                                                                            \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_VARIABLE_ACCESS_TYPES(name, code)                                                                \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      uint32_t index;                                                                                                  \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_MEMORY_TYPES(name, code)                                                                         \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      uint32_t flags_align;                                                                                            \
      uint32_t offset;                                                                                                 \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_I32_CONSTANT_TYPE(name, code)                                                                    \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      explicit EOS_VM_OPCODE_T(name)(uint32_t n) { data.ui = n; }                                                      \
      explicit EOS_VM_OPCODE_T(name)(int32_t n) { data.i = n; }                                                        \
      union {                                                                                                          \
         uint32_t ui;                                                                                                  \
         int32_t  i;                                                                                                   \
      } data;                                                                                                          \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_I64_CONSTANT_TYPE(name, code)                                                                    \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      explicit EOS_VM_OPCODE_T(name)(uint64_t n) { data.ui = n; }                                                      \
      explicit EOS_VM_OPCODE_T(name)(int64_t n) { data.i = n; }                                                        \
      union {                                                                                                          \
         uint64_t ui;                                                                                                  \
         int64_t  i;                                                                                                   \
      } data;                                                                                                          \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_F32_CONSTANT_TYPE(name, code)                                                                    \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      explicit EOS_VM_OPCODE_T(name)(uint32_t n) { data.ui = n; }                                                      \
      explicit EOS_VM_OPCODE_T(name)(float n) { data.f = n; }                                                          \
      union {                                                                                                          \
         uint32_t ui;                                                                                                  \
         float    f;                                                                                                   \
      } data;                                                                                                          \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_CREATE_F64_CONSTANT_TYPE(name, code)                                                                    \
   struct EOS_VM_OPCODE_T(name) {                                                                                      \
      EOS_VM_OPCODE_T(name)() = default;                                                                               \
      explicit EOS_VM_OPCODE_T(name)(uint64_t n) { data.ui = n; }                                                      \
      explicit EOS_VM_OPCODE_T(name)(double n) { data.f = n; }                                                         \
      union {                                                                                                          \
         uint64_t ui;                                                                                                  \
         double   f;                                                                                                   \
      } data;                                                                                                          \
      static constexpr uint8_t opcode = code;                                                                          \
   };

#define EOS_VM_IDENTITY(name, code) eosio::vm::EOS_VM_OPCODE_T(name),
#define EOS_VM_IDENTITY_END(name, code) eosio::vm::EOS_VM_OPCODE_T(name)
