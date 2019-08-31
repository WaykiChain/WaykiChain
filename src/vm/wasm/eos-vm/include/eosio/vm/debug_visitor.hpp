#pragma once

#include <eosio/vm/interpret_visitor.hpp>

namespace eosio { namespace vm {

template <typename ExecutionCTX>
struct debug_visitor : public interpret_visitor<ExecutionCTX> {
   using interpret_visitor<ExecutionCTX>::operator();
   debug_visitor(ExecutionCTX& ctx) : interpret_visitor<ExecutionCTX>(ctx) {}
   ExecutionCTX& get_context() { return interpret_visitor<ExecutionCTX>::get_context(); }
   CONTROL_FLOW_OPS(DBG_VISIT)
   BR_TABLE_OP(DBG_VISIT)
   RETURN_OP(DBG_VISIT)
   CALL_OPS(DBG_VISIT)
   PARAMETRIC_OPS(DBG_VISIT)
   VARIABLE_ACCESS_OPS(DBG_VISIT)
   MEMORY_OPS(DBG_VISIT)
   I32_CONSTANT_OPS(DBG_VISIT)
   I64_CONSTANT_OPS(DBG_VISIT)
   F32_CONSTANT_OPS(DBG_VISIT)
   F64_CONSTANT_OPS(DBG_VISIT)
   COMPARISON_OPS(DBG_VISIT)
   NUMERIC_OPS(DBG_VISIT)
   CONVERSION_OPS(DBG_VISIT)
   SYNTHETIC_OPS(DBG_VISIT)
   ERROR_OPS(DBG_VISIT)
};

struct debug_visitor2 {
   CONTROL_FLOW_OPS(DBG2_VISIT)
   BR_TABLE_OP(DBG2_VISIT)
   RETURN_OP(DBG2_VISIT)
   CALL_OPS(DBG2_VISIT)
   PARAMETRIC_OPS(DBG2_VISIT)
   VARIABLE_ACCESS_OPS(DBG2_VISIT)
   MEMORY_OPS(DBG2_VISIT)
   I32_CONSTANT_OPS(DBG2_VISIT)
   I64_CONSTANT_OPS(DBG2_VISIT)
   F32_CONSTANT_OPS(DBG2_VISIT)
   F64_CONSTANT_OPS(DBG2_VISIT)
   COMPARISON_OPS(DBG2_VISIT)
   NUMERIC_OPS(DBG2_VISIT)
   CONVERSION_OPS(DBG2_VISIT)
   SYNTHETIC_OPS(DBG2_VISIT)
   ERROR_OPS(DBG2_VISIT)
};

}} // ns eosio::wasm_backend
