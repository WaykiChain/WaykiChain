#pragma once

#include <eosio/vm/config.hpp>
#include <eosio/vm/base_visitor.hpp>
#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/softfloat.hpp>
#include <eosio/vm/stack_elem.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/wasm_stack.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace eosio { namespace vm {

   template <typename ExecutionContext>
   struct interpret_visitor : base_visitor {
      using base_visitor::operator();
      interpret_visitor(ExecutionContext& ec) : context(ec) {}
      ExecutionContext& context;

      ExecutionContext& get_context() { return context; }

      static inline constexpr void* align_address(void* addr, size_t align_amt) {
         if constexpr (should_align_memory_ops) {
            addr = (void*)(((uintptr_t)addr + (1 << align_amt) - 1) & ~((1 << align_amt) - 1));
            return addr;
         } else {
            return addr;
         }
      }
      template<typename T>
      static inline T read_unaligned(const void* addr) {
         T result;
         std::memcpy(&result, addr, sizeof(T));
         return result;
      }
      template<typename T>
      static void write_unaligned(void* addr, T value) {
         std::memcpy(addr, &value, sizeof(T));
      }

      [[gnu::always_inline]] inline void operator()(const unreachable_t& op) {
         context.inc_pc();
         throw wasm_interpreter_exception{ "unreachable" };
      }

      [[gnu::always_inline]] inline void operator()(const nop_t& op) { context.inc_pc(); }

      [[gnu::always_inline]] inline void operator()(const end_t& op) { context.inc_pc(); }
      [[gnu::always_inline]] inline void operator()(const return_t& op) { context.apply_pop_call(op.data, op.pc); }
      [[gnu::always_inline]] inline void operator()(block_t& op) { context.inc_pc(); }
      [[gnu::always_inline]] inline void operator()(loop_t& op) { context.inc_pc(); }
      [[gnu::always_inline]] inline void operator()(if_t& op) {
         context.inc_pc();
         const auto& oper = context.pop_operand();
         if (!oper.to_ui32()) {
            context.set_relative_pc(op.pc);
         }
      }
      [[gnu::always_inline]] inline void operator()(const else_t& op) { context.set_relative_pc(op.pc); }
      [[gnu::always_inline]] inline void operator()(const br_t& op) { context.jump(op.data, op.pc); }
      [[gnu::always_inline]] inline void operator()(const br_if_t& op) {
         const auto& val = context.pop_operand();
         if (context.is_true(val)) {
            context.jump(op.data, op.pc);
         } else {
            context.inc_pc();
         }
      }

      [[gnu::always_inline]] inline void operator()(const br_table_data_t& op) {
         context.inc_pc(op.index);
      }
      [[gnu::always_inline]] inline void operator()(const br_table_t& op) {
         const auto& in = context.pop_operand().to_ui32();
         const auto& entry = op.table[std::min(in, op.size)]; 
         context.jump(entry.stack_pop, entry.pc);
      }
      [[gnu::always_inline]] inline void operator()(const call_t& op) {
         context.call(op.index);
      }
      [[gnu::always_inline]] inline void operator()(const call_indirect_t& op) {
         const auto& index = context.pop_operand().to_ui32();
         uint32_t fn = context.table_elem(index);
         const auto& expected_type = context.get_module().types.at(op.index);
         const auto& actual_type = context.get_module().get_function_type(fn);
         EOS_VM_ASSERT(actual_type == expected_type, wasm_interpreter_exception, "bad call_indirect type");
         context.call(fn);
      }
      [[gnu::always_inline]] inline void operator()(const drop_t& op) {
         context.pop_operand();
         context.inc_pc();
      }
      [[gnu::always_inline]] inline void operator()(const select_t& op) {
         const auto& c  = context.pop_operand();
         const auto& v2 = context.pop_operand();
         if (c.to_ui32() == 0) {
            context.peek_operand() = v2;
         }
         context.inc_pc();
      }
      [[gnu::always_inline]] inline void operator()(const get_local_t& op) {
         context.inc_pc();
         context.push_operand(context.get_operand(op.index));
      }
      [[gnu::always_inline]] inline void operator()(const set_local_t& op) {
         context.inc_pc();
         context.set_operand(op.index, context.pop_operand());
      }
      [[gnu::always_inline]] inline void operator()(const tee_local_t& op) {
         context.inc_pc();
         const auto& oper = context.pop_operand();
         context.set_operand(op.index, oper);
         context.push_operand(oper);
      }
      [[gnu::always_inline]] inline void operator()(const get_global_t& op) {
         context.inc_pc();
         const auto& gl = context.get_global(op.index);
         context.push_operand(gl);
      }
      [[gnu::always_inline]] inline void operator()(const set_global_t& op) {
         context.inc_pc();
         const auto& oper = context.pop_operand();
         context.set_global(op.index, oper);
      }
      template<typename Op>
      inline void * pop_memop_addr(const Op& op) {
         const auto& ptr  = context.pop_operand();
         return align_address((context.linear_memory() + op.offset + ptr.to_ui32()), op.flags_align);
      }
      [[gnu::always_inline]] inline void operator()(const i32_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ read_unaligned<uint32_t>(_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load8_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<int32_t>(read_unaligned<int8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load16_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<int32_t>( read_unaligned<int16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load8_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<uint32_t>( read_unaligned<uint8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load16_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<uint32_t>( read_unaligned<uint16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint64_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load8_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<int64_t>( read_unaligned<int8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load16_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<int64_t>( read_unaligned<int16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load32_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<int64_t>( read_unaligned<int32_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load8_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load16_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load32_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint32_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const f32_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(f32_const_t{ read_unaligned<uint32_t>(_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const f64_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(f64_const_t{ read_unaligned<uint64_t>(_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, val.to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i32_store8_t& op) {
         context.inc_pc();
         const auto& val = context.pop_operand();
         void* store_loc = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint8_t>(val.to_ui32()));
      }
      [[gnu::always_inline]] inline void operator()(const i32_store16_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint16_t>(val.to_ui32()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint64_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store8_t& op) {
         context.inc_pc();
         const auto& val = context.pop_operand();
         void* store_loc = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint8_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store16_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint16_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store32_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint32_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const f32_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint32_t>(val.to_fui32()));
      }
      [[gnu::always_inline]] inline void operator()(const f64_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint64_t>(val.to_fui64()));
      }
      [[gnu::always_inline]] inline void operator()(const current_memory_t& op) {
         context.inc_pc();
         context.push_operand(i32_const_t{ context.current_linear_memory() });
      }
      [[gnu::always_inline]] inline void operator()(const grow_memory_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();
         oper       = context.grow_linear_memory(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const i64_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const f32_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const f64_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const i32_eqz_t& op) {
         context.inc_pc();
         auto& t = context.peek_operand().to_ui32();
         t       = t == 0;
      }
      [[gnu::always_inline]] inline void operator()(const i32_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs == rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs != rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_lt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs < rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_lt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs < rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_le_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs <= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_le_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs <= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_gt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs > rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_gt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs > rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ge_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs >= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ge_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs >= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_eqz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ oper.to_ui64() == 0 };
      }
      [[gnu::always_inline]] inline void operator()(const i64_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() == rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() != rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_lt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() < rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_lt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() < rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_le_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() <= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_le_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() <= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_gt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() > rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_gt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() > rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ge_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() >= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ge_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() >= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const f32_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_eq(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() == rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_ne(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() != rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_lt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_lt(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() < rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_gt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_gt(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() > rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_le_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_le(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() <= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_ge_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_ge(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() >= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_eq(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() == rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_ne(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() != rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_lt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_lt(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() < rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_gt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_gt(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() > rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_le_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_le(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() <= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_ge_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_ge(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() >= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const i32_clz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();
         // __builtin_clz(0) is undefined
         oper = oper == 0 ? 32 : __builtin_clz(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_ctz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();

         // __builtin_ctz(0) is undefined
         oper = oper == 0 ? 32 : __builtin_ctz(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_popcnt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();
         oper       = __builtin_popcount(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs += rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs -= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs *= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_div_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.div_s divide by zero");
         EOS_VM_ASSERT(!(lhs == std::numeric_limits<int32_t>::min() && rhs == -1), wasm_interpreter_exception,
                       "i32.div_s traps when I32_MAX/-1");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_div_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.div_u divide by zero");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rem_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.rem_s divide by zero");
         if (UNLIKELY(lhs == std::numeric_limits<int32_t>::min() && rhs == -1))
            lhs = 0;
         else
            lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rem_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.rem_u divide by zero");
         lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_and_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs &= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_or_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs |= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_xor_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs ^= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_shl_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs <<= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i32_shr_s_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i32_shr_u_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i32_rotl_t& op) {

         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui32();
         auto&                     lhs  = context.peek_operand().to_ui32();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs << c) | (lhs >> ((-c) & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i32_rotr_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui32();
         auto&                     lhs  = context.peek_operand().to_ui32();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs >> c) | (lhs << ((-c) & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i64_clz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui64();
         // __builtin_clzll(0) is undefined
         oper = oper == 0 ? 64 : __builtin_clzll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_ctz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui64();
         // __builtin_clzll(0) is undefined
         oper = oper == 0 ? 64 : __builtin_ctzll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_popcnt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui64();
         oper       = __builtin_popcountll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs += rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs -= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs *= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_div_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand().to_i64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.div_s divide by zero");
         EOS_VM_ASSERT(!(lhs == std::numeric_limits<int64_t>::min() && rhs == -1), wasm_interpreter_exception,
                       "i64.div_s traps when I64_MAX/-1");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_div_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.div_u divide by zero");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rem_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand().to_i64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.rem_s divide by zero");
         if (UNLIKELY(lhs == std::numeric_limits<int64_t>::min() && rhs == -1))
            lhs = 0;
         else
            lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rem_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.rem_s divide by zero");
         lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_and_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs &= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_or_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs |= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_xor_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs ^= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_shl_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs <<= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i64_shr_s_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_i64();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i64_shr_u_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i64_rotl_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui64();
         auto&                     lhs  = context.peek_operand().to_ui64();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs << c) | (lhs >> (-c & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i64_rotr_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui64();
         auto&                     lhs  = context.peek_operand().to_ui64();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs >> c) | (lhs << (-c & mask));
      }
      [[gnu::always_inline]] inline void operator()(const f32_abs_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_abs(oper);
         else
            oper = __builtin_fabsf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_neg_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_neg(oper);
         else
            oper = -oper;
      }
      [[gnu::always_inline]] inline void operator()(const f32_ceil_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_ceil(oper);
         else
            oper = __builtin_ceilf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_floor_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_floor(oper);
         else
            oper = __builtin_floorf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_trunc_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_trunc(oper);
         else
            oper = __builtin_trunc(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_nearest_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_nearest(oper);
         else
            oper = __builtin_nearbyintf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_sqrt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_sqrt(oper);
         else
            oper = __builtin_sqrtf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_add(lhs, rhs.to_f32());
         else
            lhs += rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_sub(lhs, rhs.to_f32());
         else
            lhs -= rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat) {
            lhs = _eosio_f32_mul(lhs, rhs.to_f32());
         } else
            lhs *= rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_div_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_div(lhs, rhs.to_f32());
         else
            lhs /= rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_min_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_min(lhs, rhs.to_f32());
         else
            lhs = __builtin_fminf(lhs, rhs.to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f32_max_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_max(lhs, rhs.to_f32());
         else
            lhs = __builtin_fmaxf(lhs, rhs.to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f32_copysign_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_copysign(lhs, rhs.to_f32());
         else
            lhs = __builtin_copysignf(lhs, rhs.to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f64_abs_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_abs(oper);
         else
            oper = __builtin_fabs(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_neg_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_neg(oper);
         else
            oper = -oper;
      }
      [[gnu::always_inline]] inline void operator()(const f64_ceil_t& op) {

         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_ceil(oper);
         else
            oper = __builtin_ceil(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_floor_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_floor(oper);
         else
            oper = __builtin_floor(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_trunc_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_trunc(oper);
         else
            oper = __builtin_trunc(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_nearest_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_nearest(oper);
         else
            oper = __builtin_nearbyint(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_sqrt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_sqrt(oper);
         else
            oper = __builtin_sqrt(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_add(lhs, rhs.to_f64());
         else
            lhs += rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_sub(lhs, rhs.to_f64());
         else
            lhs -= rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_mul(lhs, rhs.to_f64());
         else
            lhs *= rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_div_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_div(lhs, rhs.to_f64());
         else
            lhs /= rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_min_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_min(lhs, rhs.to_f64());
         else
            lhs = __builtin_fmin(lhs, rhs.to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const f64_max_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_max(lhs, rhs.to_f64());
         else
            lhs = __builtin_fmax(lhs, rhs.to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const f64_copysign_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_copysign(lhs, rhs.to_f64());
         else
            lhs = __builtin_copysign(lhs, rhs.to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const i32_wrap_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ static_cast<int32_t>(oper.to_i64()) };
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_s_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f32_trunc_i32s(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 2147483648.0f) || (af < -2147483648.0f)), wasm_interpreter_exception, "Error, f32.trunc_s/i32 overflow" );
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_s/i32 unrepresentable");
            oper = i32_const_t{ static_cast<int32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_u_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f32_trunc_i32u(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 4294967296.0f) || (af <= -1.0f)),wasm_interpreter_exception, "Error, f32.trunc_u/i32 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_u/i32 unrepresentable");
            oper = i32_const_t{ static_cast<uint32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_s_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f64_trunc_i32s(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 2147483648.0) || (af < -2147483648.0)), wasm_interpreter_exception, "Error, f64.trunc_s/i32 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_s/i32 unrepresentable");
            oper = i32_const_t{ static_cast<int32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_u_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f64_trunc_i32u(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 4294967296.0) || (af <= -1.0)), wasm_interpreter_exception, "Error, f64.trunc_u/i32 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_u/i32 unrepresentable");
            oper = i32_const_t{ static_cast<uint32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_extend_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ static_cast<int64_t>(oper.to_i32()) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_extend_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ static_cast<uint64_t>(oper.to_ui32()) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_s_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f32_trunc_i64s(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 9223372036854775808.0f) || (af < -9223372036854775808.0f)), wasm_interpreter_exception, "Error, f32.trunc_s/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_s/i64 unrepresentable");
            oper = i64_const_t{ static_cast<int64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_u_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f32_trunc_i64u(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 18446744073709551616.0f) || (af <= -1.0f)), wasm_interpreter_exception, "Error, f32.trunc_u/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_u/i64 unrepresentable");
            oper = i64_const_t{ static_cast<uint64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_s_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f64_trunc_i64s(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 9223372036854775808.0) || (af < -9223372036854775808.0)), wasm_interpreter_exception, "Error, f64.trunc_s/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_s/i64 unrepresentable");
            oper = i64_const_t{ static_cast<int64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_u_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f64_trunc_i64u(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 18446744073709551616.0) || (af <= -1.0)), wasm_interpreter_exception, "Error, f64.trunc_u/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_u/i64 unrepresentable");
            oper = i64_const_t{ static_cast<uint64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_i32_to_f32(oper.to_i32()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_i32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_ui32_to_f32(oper.to_ui32()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_ui32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_s_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_i64_to_f32(oper.to_i64()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_i64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_u_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_ui64_to_f32(oper.to_ui64()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_ui64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_demote_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_f64_demote(oper.to_f64()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_f64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_i32_to_f64(oper.to_i32()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_i32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_ui32_to_f64(oper.to_ui32()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_ui32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_s_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_i64_to_f64(oper.to_i64()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_i64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_u_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_ui64_to_f64(oper.to_ui64()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_ui64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_promote_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_f32_promote(oper.to_f32()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_f32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_reinterpret_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ oper.to_fui32() };
      }
      [[gnu::always_inline]] inline void operator()(const i64_reinterpret_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ oper.to_fui64() };
      }
      [[gnu::always_inline]] inline void operator()(const f32_reinterpret_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = f32_const_t{ oper.to_ui32() };
      }
      [[gnu::always_inline]] inline void operator()(const f64_reinterpret_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = f64_const_t{ oper.to_ui64() };
      }
   };

}} // namespace eosio::vm
