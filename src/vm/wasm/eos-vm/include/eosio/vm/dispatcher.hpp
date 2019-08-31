#pragma once

#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/variant.hpp>

/* clang-format off */
#define DISPATCH(VISITOR, MODULE)                                       \
   {                                                                    \
      decltype(VISITOR)& ev_visitor = VISITOR;                          \
      decltype(MODULE)& ev_module = MODULE;                             \
      static void* dispatch_table[] = {                                 \
         CONTROL_FLOW_OPS(CREATE_TABLE_ENTRY)                           \
         BR_TABLE_OP(CREATE_TABLE_ENTRY)                                \
         RETURN_OP(CREATE_TABLE_ENTRY)                                  \
         CALL_OPS(CREATE_TABLE_ENTRY)                                   \
         PARAMETRIC_OPS(CREATE_TABLE_ENTRY)                             \
         VARIABLE_ACCESS_OPS(CREATE_TABLE_ENTRY)                        \
         MEMORY_OPS(CREATE_TABLE_ENTRY)                                 \
         I32_CONSTANT_OPS(CREATE_TABLE_ENTRY)                           \
         I64_CONSTANT_OPS(CREATE_TABLE_ENTRY)                           \
         F32_CONSTANT_OPS(CREATE_TABLE_ENTRY)                           \
         F64_CONSTANT_OPS(CREATE_TABLE_ENTRY)                           \
         COMPARISON_OPS(CREATE_TABLE_ENTRY)                             \
         NUMERIC_OPS(CREATE_TABLE_ENTRY)                                \
         CONVERSION_OPS(CREATE_TABLE_ENTRY)                             \
         SYNTHETIC_OPS(CREATE_TABLE_ENTRY)                              \
         ERROR_OPS(CREATE_TABLE_ENTRY)                                  \
         EMPTY_OPS(CREATE_TABLE_ENTRY)                                  \
         CONTROL_FLOW_OPS(CREATE_EXITING_TABLE_ENTRY)                   \
         BR_TABLE_OP(CREATE_EXITING_TABLE_ENTRY)                        \
         RETURN_OP(CREATE_EXITING_TABLE_ENTRY)                          \
         CALL_OPS(CREATE_EXITING_TABLE_ENTRY)                           \
         PARAMETRIC_OPS(CREATE_EXITING_TABLE_ENTRY)                     \
         VARIABLE_ACCESS_OPS(CREATE_EXITING_TABLE_ENTRY)                \
         MEMORY_OPS(CREATE_EXITING_TABLE_ENTRY)                         \
         I32_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)                   \
         I64_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)                   \
         F32_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)                   \
         F64_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)                   \
         COMPARISON_OPS(CREATE_EXITING_TABLE_ENTRY)                     \
         NUMERIC_OPS(CREATE_EXITING_TABLE_ENTRY)                        \
         CONVERSION_OPS(CREATE_EXITING_TABLE_ENTRY)                     \
         SYNTHETIC_OPS(CREATE_EXITING_TABLE_ENTRY)                      \
         EMPTY_OPS(CREATE_EXITING_TABLE_ENTRY)                          \
         ERROR_OPS(CREATE_EXITING_TABLE_ENTRY)                          \
         &&__ev_last                                                    \
      };                                                                \
      goto *dispatch_table[ev_module.code.at_no_check(_code_index).code.at_no_check(_pc - _current_offset).index()]; \
      while (1) {                                                       \
         auto& ev_variant = ev_module.code.at_no_check(_code_index).code.at_no_check(_pc - _current_offset); \
         CONTROL_FLOW_OPS(CREATE_LABEL);                                \
         BR_TABLE_OP(CREATE_LABEL);                                     \
         RETURN_OP(CREATE_LABEL);                                       \
         CALL_OPS(CREATE_LABEL);                                        \
         PARAMETRIC_OPS(CREATE_LABEL);                                  \
         VARIABLE_ACCESS_OPS(CREATE_LABEL);                             \
         MEMORY_OPS(CREATE_LABEL);                                      \
         I32_CONSTANT_OPS(CREATE_LABEL);                                \
         I64_CONSTANT_OPS(CREATE_LABEL);                                \
         F32_CONSTANT_OPS(CREATE_LABEL);                                \
         F64_CONSTANT_OPS(CREATE_LABEL);                                \
         COMPARISON_OPS(CREATE_LABEL);                                  \
         NUMERIC_OPS(CREATE_LABEL);                                     \
         CONVERSION_OPS(CREATE_LABEL);                                  \
         SYNTHETIC_OPS(CREATE_LABEL);                                   \
         ERROR_OPS(CREATE_LABEL);                                       \
         EMPTY_OPS(CREATE_EMPTY_LABEL);                                 \
         CONTROL_FLOW_OPS(CREATE_EXITING_LABEL);                        \
         BR_TABLE_OP(CREATE_EXITING_LABEL);                             \
         RETURN_OP(CREATE_EXITING_LABEL);                               \
         CALL_OPS(CREATE_EXITING_LABEL);                                \
         PARAMETRIC_OPS(CREATE_EXITING_LABEL);                          \
         VARIABLE_ACCESS_OPS(CREATE_EXITING_LABEL);                     \
         MEMORY_OPS(CREATE_EXITING_LABEL);                              \
         I32_CONSTANT_OPS(CREATE_EXITING_LABEL);                        \
         I64_CONSTANT_OPS(CREATE_EXITING_LABEL);                        \
         F32_CONSTANT_OPS(CREATE_EXITING_LABEL);                        \
         F64_CONSTANT_OPS(CREATE_EXITING_LABEL);                        \
         COMPARISON_OPS(CREATE_EXITING_LABEL);                          \
         NUMERIC_OPS(CREATE_EXITING_LABEL);                             \
         CONVERSION_OPS(CREATE_EXITING_LABEL);                          \
         SYNTHETIC_OPS(CREATE_EXITING_LABEL);                           \
         EMPTY_OPS(CREATE_EXITING_EMPTY_LABEL);                         \
         ERROR_OPS(CREATE_EXITING_LABEL);                               \
      __ev_last:                                                        \
         throw wasm_interpreter_exception{};                            \
      }                                                                 \
   }
/* clang-format on */
