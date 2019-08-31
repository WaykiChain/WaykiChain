#pragma once

#include <eosio/vm/base_visitor.hpp>
#include <eosio/vm/execution_context.hpp>
#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/stack_elem.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/wasm_stack.hpp>

#include <iostream>
#include <sstream>
#include <variant>

#include <eosio/vm/config.hpp>
#include <eosio/vm/softfloat.hpp>

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

      [[gnu::always_inline]] inline void operator()(const unreachable_t& op) {
         context.inc_pc();
         throw wasm_interpreter_exception{ "unreachable" };
      }

      [[gnu::always_inline]] inline void operator()(const nop_t& op) { context.inc_pc(); }

      [[gnu::always_inline]] inline void operator()(const fend_t& op) { 
         context.apply_pop_call(); 
         context.print_stack_label();
      }

      [[gnu::always_inline]] inline void operator()(const end_t& op) {
         const auto& label    = context.pop_label();
         uint16_t    op_index = 0;
         uint8_t     ret_type = 0;
         std::visit(overloaded{ [&](const block_t& b) {
                                  op_index = b.op_index;
                                  ret_type = static_cast<uint8_t>(b.data);
                               },
                                [&](const loop_t& l) {
                                   op_index = l.op_index;
                                   ret_type = static_cast<uint8_t>(l.data);
                                },
                                [&](const if__t& i) {
                                   op_index = i.op_index;
                                   ret_type = static_cast<uint8_t>(i.data);
                                },
                                [&](auto) { throw wasm_interpreter_exception{ "expected control structure" }; } },
                    label);

         if (ret_type != types::pseudo) {
            const auto& op = context.pop_operand();
            context.eat_operands(op_index);
            context.push_operand(op);
         } else {
            context.eat_operands(op_index);
         }

         context.inc_pc();

         context.print_stack_label();
      }
      [[gnu::always_inline]] inline void operator()(const return__t& op) { 
         context.apply_pop_call(); 
         context.print_stack_label();
      }
      [[gnu::always_inline]] inline void operator()(block_t& op) {
         context.inc_pc();
         op.index    = context.current_label_index();
         op.op_index = context.current_operands_index();
         context.push_label(op);

         context.print_stack_label();
      }
      [[gnu::always_inline]] inline void operator()(loop_t& op) {
         context.inc_pc();
         op.index    = context.current_label_index();
         op.op_index = context.current_operands_index();
         context.push_label(op);

         context.print_stack_label();
      }
      [[gnu::always_inline]] inline void operator()(if__t& op) {
         context.inc_pc();
         op.index         = context.current_label_index();
         const auto& oper = context.pop_operand();
         if (!to_ui32(oper)) {
            context.set_relative_pc(op.pc + 1);
         }
         op.op_index = context.current_operands_index();
         context.push_label(op);
      }
      [[gnu::always_inline]] inline void operator()(const else__t& op) { context.set_relative_pc(op.pc); }
      [[gnu::always_inline]] inline void operator()(const br_t& op) { context.jump(op.data); }
      [[gnu::always_inline]] inline void operator()(const br_if_t& op) {
         const auto& val = context.pop_operand();
         if (context.is_true(val)) {
            context.jump(op.data);
            //std::cout << "br_if_t jump label:" << "data:" << op.data << "pc:" << op.pc <<"index:" << op.index << "op_index:" << op.op_index << "\n";
            context.print_stack_label();
         } else {
            context.inc_pc();
         }
      }
      [[gnu::always_inline]] inline void operator()(const br_table_t& op) {
         const auto& in = to_ui32(context.pop_operand());
         if (in < op.size)
            context.jump(op.table[in]);
         else
            context.jump(op.default_target);
      }
      [[gnu::always_inline]] inline void operator()(const call_t& op) {
         context.call(op.index);
         //context.print_stack_label();
         // TODO place these in parser
         // EOS_WB_ASSERT(b.index < funcs_size, wasm_interpreter_exception, "call index out of bounds");
      }
      [[gnu::always_inline]] inline void operator()(const call_indirect_t& op) {
         const auto& index = to_ui32(context.pop_operand());
         context.call(context.table_elem(index));
      }
      [[gnu::always_inline]] inline void operator()(const drop_t& op) {
         context.pop_operand();
         context.inc_pc();
      }
      [[gnu::always_inline]] inline void operator()(const select_t& op) {
         const auto& c  = context.pop_operand();
         const auto& v2 = context.pop_operand();
         if (to_ui32(c) == 0) {
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
      [[gnu::always_inline]] inline void operator()(const i32_load_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint32_t*   _ptr = (uint32_t*)align_address((uint32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(i32_const_t{ *_ptr });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load8_s_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         int8_t*     _ptr =
               (int8_t*)align_address((int8_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i32_const_t{ static_cast<int32_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load16_s_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         int16_t*    _ptr =
               (int16_t*)align_address((int16_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i32_const_t{ static_cast<int32_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load8_u_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         uint8_t*    _ptr =
               (uint8_t*)align_address((uint8_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i32_const_t{ static_cast<uint32_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load16_u_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint16_t*   _ptr = (uint16_t*)align_address((uint16_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(i32_const_t{ static_cast<uint32_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint64_t*   _ptr = (uint64_t*)align_address((uint64_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(i64_const_t{ static_cast<uint64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load8_s_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         int8_t*     _ptr =
               (int8_t*)align_address((int8_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i64_const_t{ static_cast<int64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load16_s_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         int16_t*    _ptr =
               (int16_t*)align_address((int16_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i64_const_t{ static_cast<int64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load32_s_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         int32_t*    _ptr =
               (int32_t*)align_address((int32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i64_const_t{ static_cast<int64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load8_u_t& op) {
         context.inc_pc();
         const auto& ptr = context.pop_operand();
         uint8_t*    _ptr =
               (uint8_t*)align_address((uint8_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         context.push_operand(i64_const_t{ static_cast<uint64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load16_u_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint16_t*   _ptr = (uint16_t*)align_address((uint16_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(i64_const_t{ static_cast<uint64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load32_u_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint32_t*   _ptr = (uint32_t*)align_address((uint32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(i64_const_t{ static_cast<uint64_t>(*_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const f32_load_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint32_t*   _ptr = (uint32_t*)align_address((uint32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(f32_const_t{ *_ptr });
      }
      [[gnu::always_inline]] inline void operator()(const f64_load_t& op) {
         context.inc_pc();
         const auto& ptr  = context.pop_operand();
         uint64_t*   _ptr = (uint64_t*)align_address((uint64_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                   op.flags_align);
         context.push_operand(f64_const_t{ *_ptr });
      }
      [[gnu::always_inline]] inline void operator()(const i32_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint32_t* store_loc = (uint32_t*)align_address((uint32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = to_ui32(val);
      }
      [[gnu::always_inline]] inline void operator()(const i32_store8_t& op) {
         context.inc_pc();
         const auto& val = context.pop_operand();
         const auto& ptr = context.pop_operand();
         uint8_t*    store_loc =
               (uint8_t*)align_address((uint8_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         *store_loc = static_cast<uint8_t>(to_ui32(val));
      }
      [[gnu::always_inline]] inline void operator()(const i32_store16_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint16_t* store_loc = (uint16_t*)align_address((uint16_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = static_cast<uint16_t>(to_ui32(val));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint64_t* store_loc = (uint64_t*)align_address((uint64_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = static_cast<uint64_t>(to_ui64(val));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store8_t& op) {
         context.inc_pc();
         const auto& val = context.pop_operand();
         const auto& ptr = context.pop_operand();
         uint8_t*    store_loc =
               (uint8_t*)align_address((uint8_t*)(context.linear_memory() + op.offset + to_ui32(ptr)), op.flags_align);
         *store_loc = static_cast<uint8_t>(to_ui64(val));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store16_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint16_t* store_loc = (uint16_t*)align_address((uint16_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = static_cast<uint16_t>(to_ui64(val));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store32_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint32_t* store_loc = (uint32_t*)align_address((uint32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = static_cast<uint32_t>(to_ui64(val));
      }
      [[gnu::always_inline]] inline void operator()(const f32_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint32_t* store_loc = (uint32_t*)align_address((uint32_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = static_cast<uint32_t>(to_fui32(val));
      }
      [[gnu::always_inline]] inline void operator()(const f64_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         const auto& ptr     = context.pop_operand();
         uint64_t* store_loc = (uint64_t*)align_address((uint64_t*)(context.linear_memory() + op.offset + to_ui32(ptr)),
                                                        op.flags_align);
         *store_loc          = static_cast<uint64_t>(to_fui64(val));
      }
      [[gnu::always_inline]] inline void operator()(const current_memory_t& op) {
         context.inc_pc();
         context.push_operand(i32_const_t{ context.current_linear_memory() });
      }
      [[gnu::always_inline]] inline void operator()(const grow_memory_t& op) {
         context.inc_pc();
         auto& oper = to_ui32(context.peek_operand());
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
         auto& t = to_ui32(context.peek_operand());
         t       = t == 0;
      }
      [[gnu::always_inline]] inline void operator()(const i32_eq_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs             = lhs == rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ne_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs             = lhs != rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_lt_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         lhs             = lhs < rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_lt_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs             = lhs < rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_le_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         lhs             = lhs <= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_le_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs             = lhs <= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_gt_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         lhs             = lhs > rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_gt_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs             = lhs > rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ge_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         lhs             = lhs >= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ge_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs             = lhs >= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_eqz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ to_ui64(oper) == 0 };
      }
      [[gnu::always_inline]] inline void operator()(const i64_eq_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_ui64(lhs) == rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ne_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_ui64(lhs) != rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_lt_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_i64(lhs) < rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_lt_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_ui64(lhs) < rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_le_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_i64(lhs) <= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_le_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_ui64(lhs) <= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_gt_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_i64(lhs) > rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_gt_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_ui64(lhs) > rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ge_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_i64(lhs) >= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ge_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ to_ui64(lhs) >= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const f32_eq_t& op) {
         context.inc_pc();
         const auto& rhs = to_f32(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_eq(to_f32(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f32(lhs) == rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_ne_t& op) {
         context.inc_pc();
         const auto& rhs = to_f32(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_ne(to_f32(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f32(lhs) != rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_lt_t& op) {
         context.inc_pc();
         const auto& rhs = to_f32(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_lt(to_f32(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f32(lhs) < rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_gt_t& op) {
         context.inc_pc();
         const auto& rhs = to_f32(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_gt(to_f32(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f32(lhs) > rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_le_t& op) {
         context.inc_pc();
         const auto& rhs = to_f32(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_le(to_f32(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f32(lhs) <= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_ge_t& op) {
         context.inc_pc();
         const auto& rhs = to_f32(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_ge(to_f32(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f32(lhs) >= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_eq_t& op) {
         context.inc_pc();
         const auto& rhs = to_f64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_eq(to_f64(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f64(lhs) == rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_ne_t& op) {
         context.inc_pc();
         const auto& rhs = to_f64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_ne(to_f64(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f64(lhs) != rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_lt_t& op) {
         context.inc_pc();
         const auto& rhs = to_f64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_lt(to_f64(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f64(lhs) < rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_gt_t& op) {
         context.inc_pc();
         const auto& rhs = to_f64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_gt(to_f64(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f64(lhs) > rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_le_t& op) {
         context.inc_pc();
         const auto& rhs = to_f64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_le(to_f64(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f64(lhs) <= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_ge_t& op) {
         context.inc_pc();
         const auto& rhs = to_f64(context.pop_operand());
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_ge(to_f64(lhs), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(to_f64(lhs) >= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const i32_clz_t& op) {
         context.inc_pc();
         auto& oper = to_ui32(context.peek_operand());
         // __builtin_clz(0) is undefined
         oper = oper == 0 ? 32 : __builtin_clz(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_ctz_t& op) {
         context.inc_pc();
         auto& oper = to_ui32(context.peek_operand());

         // __builtin_ctz(0) is undefined
         oper = oper == 0 ? 32 : __builtin_ctz(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_popcnt_t& op) {
         context.inc_pc();
         auto& oper = to_ui32(context.peek_operand());
         oper       = __builtin_popcount(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_add_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs += rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_sub_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs -= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_mul_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs *= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_div_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.div_s divide by zero");
         EOS_WB_ASSERT(!(lhs == std::numeric_limits<int32_t>::min() && rhs == -1), wasm_interpreter_exception,
                       "i32.div_s traps when I32_MAX/-1");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_div_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.div_u divide by zero");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rem_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.rem_s divide by zero");
         if (UNLIKELY(lhs == std::numeric_limits<int32_t>::min() && rhs == -1))
            lhs = 0;
         else
            lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rem_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.rem_u divide by zero");
         lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_and_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs &= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_or_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs |= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_xor_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs ^= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_shl_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs <<= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_shr_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_i32(context.peek_operand());
         lhs >>= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_shr_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui32(context.pop_operand());
         auto&       lhs = to_ui32(context.peek_operand());
         lhs >>= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rotl_t& op) {

         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto&               rhs  = to_ui32(context.pop_operand());
         auto&                     lhs  = to_ui32(context.peek_operand());
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs << c) | (lhs >> ((-c) & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i32_rotr_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto&               rhs  = to_ui32(context.pop_operand());
         auto&                     lhs  = to_ui32(context.peek_operand());
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs >> c) | (lhs << ((-c) & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i64_clz_t& op) {
         context.inc_pc();
         auto& oper = to_ui64(context.peek_operand());
         // __builtin_clzll(0) is undefined
         oper = oper == 0 ? 64 : __builtin_clzll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_ctz_t& op) {
         context.inc_pc();
         auto& oper = to_ui64(context.peek_operand());
         // __builtin_clzll(0) is undefined
         oper = oper == 0 ? 64 : __builtin_ctzll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_popcnt_t& op) {
         context.inc_pc();
         auto& oper = to_ui64(context.peek_operand());
         oper       = __builtin_popcountll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_add_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs += rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_sub_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs -= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_mul_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs *= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_div_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i64(context.pop_operand());
         auto&       lhs = to_i64(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.div_s divide by zero");
         EOS_WB_ASSERT(!(lhs == std::numeric_limits<int64_t>::min() && rhs == -1), wasm_interpreter_exception,
                       "i64.div_s traps when I64_MAX/-1");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_div_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.div_u divide by zero");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rem_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_i64(context.pop_operand());
         auto&       lhs = to_i64(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.rem_s divide by zero");
         if (UNLIKELY(lhs == std::numeric_limits<int64_t>::min() && rhs == -1))
            lhs = 0;
         else
            lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rem_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         EOS_WB_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.rem_s divide by zero");
         lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_and_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs &= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_or_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs |= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_xor_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs ^= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_shl_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs <<= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_shr_s_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_i64(context.peek_operand());
         lhs >>= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_shr_u_t& op) {
         context.inc_pc();
         const auto& rhs = to_ui64(context.pop_operand());
         auto&       lhs = to_ui64(context.peek_operand());
         lhs >>= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rotl_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto&               rhs  = to_ui64(context.pop_operand());
         auto&                     lhs  = to_ui64(context.peek_operand());
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs << c) | (lhs >> (-c & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i64_rotr_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto&               rhs  = to_ui64(context.pop_operand());
         auto&                     lhs  = to_ui64(context.peek_operand());
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs >> c) | (lhs << (-c & mask));
      }
      [[gnu::always_inline]] inline void operator()(const f32_abs_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_abs(oper);
         else
            oper = __builtin_fabsf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_neg_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_neg(oper);
         else
            oper = (-1.0f) * oper;
      }
      [[gnu::always_inline]] inline void operator()(const f32_ceil_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_ceil(oper);
         else
            oper = __builtin_ceilf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_floor_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_floor(oper);
         else
            oper = __builtin_floorf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_trunc_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_trunc(oper);
         else
            oper = __builtin_trunc(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_nearest_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_nearest(oper);
         else
            oper = __builtin_nearbyintf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_sqrt_t& op) {
         context.inc_pc();
         auto& oper = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f32_sqrt(oper);
         else
            oper = __builtin_sqrtf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f32_add(lhs, to_f32(rhs));
         else
            lhs += to_f32(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f32_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f32_sub(lhs, to_f32(rhs));
         else
            lhs -= to_f32(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f32_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat) {
            lhs = _eosio_f32_mul(lhs, to_f32(rhs));
         } else
            lhs *= to_f32(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f32_div_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f32_div(lhs, to_f32(rhs));
         else
            lhs /= to_f32(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f32_min_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f32_min(lhs, to_f32(rhs));
         else
            lhs = __builtin_fminf(lhs, to_f32(rhs));
      }
      [[gnu::always_inline]] inline void operator()(const f32_max_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f32_max(lhs, to_f32(rhs));
         else
            lhs = __builtin_fmaxf(lhs, to_f32(rhs));
      }
      [[gnu::always_inline]] inline void operator()(const f32_copysign_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f32(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f32_copysign(lhs, to_f32(rhs));
         else
            lhs = __builtin_copysignf(lhs, to_f32(rhs));
      }
      [[gnu::always_inline]] inline void operator()(const f64_abs_t& op) {
         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_abs(oper);
         else
            oper = __builtin_fabs(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_neg_t& op) {
         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_neg(oper);
         else
            oper = (-1.0) * oper;
      }
      [[gnu::always_inline]] inline void operator()(const f64_ceil_t& op) {

         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_ceil(oper);
         else
            oper = __builtin_ceil(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_floor_t& op) {
         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_floor(oper);
         else
            oper = __builtin_floor(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_trunc_t& op) {
         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_trunc(oper);
         else
            oper = __builtin_trunc(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_nearest_t& op) {
         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_nearest(oper);
         else
            oper = __builtin_nearbyint(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_sqrt_t& op) {
         context.inc_pc();
         auto& oper = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            oper = _eosio_f64_sqrt(oper);
         else
            oper = __builtin_sqrt(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_add(lhs, to_f64(rhs));
         else
            lhs += to_f64(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f64_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_sub(lhs, to_f64(rhs));
         else
            lhs -= to_f64(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f64_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_mul(lhs, to_f64(rhs));
         else
            lhs *= to_f64(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f64_div_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_div(lhs, to_f64(rhs));
         else
            lhs /= to_f64(rhs);
      }
      [[gnu::always_inline]] inline void operator()(const f64_min_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_min(lhs, to_f64(rhs));
         else
            lhs = __builtin_fmin(lhs, to_f64(rhs));
      }
      [[gnu::always_inline]] inline void operator()(const f64_max_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_max(lhs, to_f64(rhs));
         else
            lhs = __builtin_fmax(lhs, to_f64(rhs));
      }
      [[gnu::always_inline]] inline void operator()(const f64_copysign_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = to_f64(context.peek_operand());
         if constexpr (use_softfloat)
            lhs = _eosio_f64_copysign(lhs, to_f64(rhs));
         else
            lhs = __builtin_copysign(lhs, to_f64(rhs));
      }
      [[gnu::always_inline]] inline void operator()(const i32_wrap_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ static_cast<int32_t>(to_i64(oper)) };
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_s_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f32_trunc_i32s(to_f32(oper)) };
         } else {
            oper = i32_const_t{ static_cast<int32_t>(to_f32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_u_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f32_trunc_i32u(to_f32(oper)) };
         } else {
            oper = i32_const_t{ static_cast<uint32_t>(to_f32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_s_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f64_trunc_i32s(to_f64(oper)) };
         } else {
            oper = i32_const_t{ static_cast<int32_t>(to_f64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_u_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f64_trunc_i32u(to_f64(oper)) };
         } else {
            oper = i32_const_t{ static_cast<uint32_t>(to_f64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_extend_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ static_cast<int64_t>(to_i32(oper)) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_extend_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ static_cast<uint64_t>(to_ui32(oper)) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_s_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f32_trunc_i64s(to_f32(oper)) };
         } else {
            oper = i64_const_t{ static_cast<int64_t>(to_f32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_u_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f32_trunc_i64u(to_f32(oper)) };
         } else {
            oper = i64_const_t{ static_cast<uint64_t>(to_f32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_s_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f64_trunc_i64s(to_f64(oper)) };
         } else {
            oper = i64_const_t{ static_cast<int64_t>(to_f64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_u_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f64_trunc_i64u(to_f64(oper)) };
         } else {
            oper = i64_const_t{ static_cast<uint64_t>(to_f64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_i32_to_f32(to_i32(oper)) };
         } else {
            oper = f32_const_t{ static_cast<float>(to_i32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_ui32_to_f32(to_ui32(oper)) };
         } else {
            oper = f32_const_t{ static_cast<float>(to_ui32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_s_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_i64_to_f32(to_i64(oper)) };
         } else {
            oper = f32_const_t{ static_cast<float>(to_i64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_u_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_ui64_to_f32(to_ui64(oper)) };
         } else {
            oper = f32_const_t{ static_cast<float>(to_ui64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_demote_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_f64_demote(to_f64(oper)) };
         } else {
            oper = f32_const_t{ static_cast<float>(to_f64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_i32_to_f64(to_i32(oper)) };
         } else {
            oper = f64_const_t{ static_cast<double>(to_i32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_ui32_to_f64(to_ui32(oper)) };
         } else {
            oper = f64_const_t{ static_cast<double>(to_ui32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_s_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_i64_to_f64(to_i64(oper)) };
         } else {
            oper = f64_const_t{ static_cast<double>(to_i64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_u_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_ui64_to_f64(to_ui64(oper)) };
         } else {
            oper = f64_const_t{ static_cast<double>(to_ui64(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_promote_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_f32_promote(to_f32(oper)) };
         } else {
            oper = f64_const_t{ static_cast<double>(to_f32(oper)) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_reinterpret_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ to_fui32(oper) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_reinterpret_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ to_fui64(oper) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_reinterpret_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = f32_const_t{ to_ui32(oper) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_reinterpret_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = f64_const_t{ to_ui64(oper) };
      }
      [[gnu::always_inline]] inline void operator()(const error_t& op) { context.inc_pc(); }
   };

}} // namespace eosio::vm
