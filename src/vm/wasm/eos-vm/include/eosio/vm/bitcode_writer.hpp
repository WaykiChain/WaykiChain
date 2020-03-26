#pragma once

#include <eosio/vm/allocator.hpp>
#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/vector.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace eosio { namespace vm {

   class bitcode_writer {

      template<class I>
      decltype(auto) append_instr(I&& instr) {
         return (fb[op_index++] = instr).template get<std::decay_t<I>>();
      }

    public:
      explicit bitcode_writer(growable_allocator& alloc, std::size_t source_bytes, module& mod) :
         _allocator(alloc),
         _code_segment_base(alloc.start_code()),
         fb(alloc, source_bytes),
         _mod(&mod) {}
      ~bitcode_writer() { _allocator.end_code<false>(_code_segment_base); }
      void emit_unreachable() { fb[op_index++] = unreachable_t{}; };
      void emit_nop() { fb[op_index++] = nop_t{}; }
      uint32_t emit_end() { return op_index; }
      uint32_t* emit_return(uint32_t depth_change) {
         return emit_br(depth_change);
      }
      void emit_block() {}
      uint32_t emit_loop() { return op_index; }
      uint32_t* emit_if() { 
         if_t& instr = append_instr(if_t{});
         return &instr.pc;
      }
      uint32_t * emit_else(uint32_t * if_loc) {
         auto& else_ = append_instr(else_t{});
         *if_loc = _base_offset + op_index;
         return &else_.pc;
      }
      uint32_t * emit_br(uint32_t depth_change) {
         auto& instr = append_instr(br_t{});
         instr.data = depth_change;
         return &instr.pc;
      }
      uint32_t * emit_br_if(uint32_t depth_change) {
         auto& instr = append_instr(br_if_t{});
         instr.data = depth_change;
         return &instr.pc;
      }

      struct br_table_parser;
      friend struct br_table_parser;
      struct br_table_parser {
         br_table_parser(bitcode_writer& base, uint32_t table_size) :
            _this{ &base },
            _i{ 0 } {
            br_table_t& bt = _this->append_instr(br_table_t{});
            bt.offset = static_cast<uint32_t>(((table_size * sizeof(br_table_t::elem_t))/sizeof(opcode))+2);

            // point the branch table data to after the br_table instruction
            _br_tab = bt.table = reinterpret_cast<br_table_t::elem_t*>(&_this->fb[_this->op_index]);

            _this->op_index += bt.offset;

            // canary to throw if we have overbounded our allocated memory
            _this->fb[_this->op_index] = error_t{};
            bt.size           = table_size;
         }
         uint32_t * emit_case(uint32_t depth_change) {
            auto& elem = _br_tab[_i++];
            elem.stack_pop = depth_change;
            return &elem.pc;
         }
         // Must be called after all cases
         uint32_t* emit_default(uint32_t depth_change) {
            auto result = emit_case(depth_change);
            EOS_VM_ASSERT(_this->fb[_this->op_index].is_a<error_t>(), wasm_parse_exception, "overwrote br_table data");
            return result;
         }
         br_table_t::elem_t* _br_tab;
         bitcode_writer * _this;
         std::size_t _i;
         br_table_parser(const br_table_parser&) = delete;
         br_table_parser& operator=(const br_table_parser&) = delete;
      };
      auto emit_br_table(uint32_t table_size) { return br_table_parser{ *this, table_size }; }
      void emit_call(const func_type& ft, uint32_t funcnum) { fb[op_index++] = call_t{ funcnum }; }
      void emit_call_indirect(const func_type& ft, uint32_t functypeidx) { fb[op_index++] = call_indirect_t{ functypeidx }; }


      void emit_drop() { fb[op_index++] = drop_t{}; }
      void emit_select() { fb[op_index++] = select_t{}; }
      void emit_get_local(uint32_t localidx) { fb[op_index++] = get_local_t{localidx}; }
      void emit_set_local(uint32_t localidx) { fb[op_index++] = set_local_t{localidx}; }
      void emit_tee_local(uint32_t localidx) { fb[op_index++] = tee_local_t{localidx}; }
      void emit_get_global(uint32_t localidx) { fb[op_index++] = get_global_t{localidx}; }
      void emit_set_global(uint32_t localidx) { fb[op_index++] = set_global_t{localidx}; }

#define MEM_OP(op_name) \
      void emit_ ## op_name(uint32_t offset, uint32_t alignment) { fb[op_index++] = op_name ## _t{ offset, alignment }; }
#define LOAD_OP MEM_OP
#define STORE_OP MEM_OP
      LOAD_OP(i32_load)
      LOAD_OP(i64_load)
      LOAD_OP(f32_load)
      LOAD_OP(f64_load)
      LOAD_OP(i32_load8_s)
      LOAD_OP(i32_load16_s)
      LOAD_OP(i32_load8_u)
      LOAD_OP(i32_load16_u)
      LOAD_OP(i64_load8_s)
      LOAD_OP(i64_load16_s)
      LOAD_OP(i64_load32_s)
      LOAD_OP(i64_load8_u)
      LOAD_OP(i64_load16_u)
      LOAD_OP(i64_load32_u)
      STORE_OP(i32_store)
      STORE_OP(i64_store)
      STORE_OP(f32_store)
      STORE_OP(f64_store)
      STORE_OP(i32_store8)
      STORE_OP(i32_store16)
      STORE_OP(i64_store8)
      STORE_OP(i64_store16)
      STORE_OP(i64_store32)
#undef LOAD_OP
#undef STORE_OP
#undef MEM_OP

      void emit_current_memory() { fb[op_index++] = current_memory_t{}; }
      void emit_grow_memory() { fb[op_index++] = grow_memory_t{}; }

      void emit_i32_const(uint32_t value) { fb[op_index++] = i32_const_t{ value }; }
      void emit_i64_const(uint64_t value) { fb[op_index++] = i64_const_t{ value }; }
      void emit_f32_const(float value) { fb[op_index++] = f32_const_t{ value }; }
      void emit_f64_const(double value) { fb[op_index++] = f64_const_t{ value }; }

#define OP(opname) \
      void emit_ ## opname() { fb[op_index++] = opname ## _t{}; }
#define UNOP OP
#define BINOP OP


      UNOP(i32_eqz)
      BINOP(i32_eq)
      BINOP(i32_ne)
      BINOP(i32_lt_s)
      BINOP(i32_lt_u)
      BINOP(i32_gt_s)
      BINOP(i32_gt_u)
      BINOP(i32_le_s)
      BINOP(i32_le_u)
      BINOP(i32_ge_s)
      BINOP(i32_ge_u)
      UNOP(i64_eqz)
      BINOP(i64_eq)
      BINOP(i64_ne)
      BINOP(i64_lt_s)
      BINOP(i64_lt_u)
      BINOP(i64_gt_s)
      BINOP(i64_gt_u)
      BINOP(i64_le_s)
      BINOP(i64_le_u)
      BINOP(i64_ge_s)
      BINOP(i64_ge_u)
      BINOP(f32_eq)
      BINOP(f32_ne)
      BINOP(f32_lt)
      BINOP(f32_gt)
      BINOP(f32_le)
      BINOP(f32_ge)
      BINOP(f64_eq)
      BINOP(f64_ne)
      BINOP(f64_lt)
      BINOP(f64_gt)
      BINOP(f64_le)
      BINOP(f64_ge)

      UNOP(i32_clz)
      UNOP(i32_ctz)
      UNOP(i32_popcnt)
      BINOP(i32_add)
      BINOP(i32_sub)
      BINOP(i32_mul)
      BINOP(i32_div_s)
      BINOP(i32_div_u)
      BINOP(i32_rem_s)
      BINOP(i32_rem_u)
      BINOP(i32_and)
      BINOP(i32_or)
      BINOP(i32_xor)
      BINOP(i32_shl)
      BINOP(i32_shr_s)
      BINOP(i32_shr_u)
      BINOP(i32_rotl)
      BINOP(i32_rotr)
      UNOP(i64_clz)
      UNOP(i64_ctz)
      UNOP(i64_popcnt)
      BINOP(i64_add)
      BINOP(i64_sub)
      BINOP(i64_mul)
      BINOP(i64_div_s)
      BINOP(i64_div_u)
      BINOP(i64_rem_s)
      BINOP(i64_rem_u)
      BINOP(i64_and)
      BINOP(i64_or)
      BINOP(i64_xor)
      BINOP(i64_shl)
      BINOP(i64_shr_s)
      BINOP(i64_shr_u)
      BINOP(i64_rotl)
      BINOP(i64_rotr)

      UNOP(f32_abs)
      UNOP(f32_neg)
      UNOP(f32_ceil)
      UNOP(f32_floor)
      UNOP(f32_trunc)
      UNOP(f32_nearest)
      UNOP(f32_sqrt)
      BINOP(f32_add)
      BINOP(f32_sub)
      BINOP(f32_mul)
      BINOP(f32_div)
      BINOP(f32_min)
      BINOP(f32_max)
      BINOP(f32_copysign)
      UNOP(f64_abs)
      UNOP(f64_neg)
      UNOP(f64_ceil)
      UNOP(f64_floor)
      UNOP(f64_trunc)
      UNOP(f64_nearest)
      UNOP(f64_sqrt)
      BINOP(f64_add)
      BINOP(f64_sub)
      BINOP(f64_mul)
      BINOP(f64_div)
      BINOP(f64_min)
      BINOP(f64_max)
      BINOP(f64_copysign)
      
      UNOP(i32_wrap_i64)
      UNOP(i32_trunc_s_f32)
      UNOP(i32_trunc_u_f32)
      UNOP(i32_trunc_s_f64)
      UNOP(i32_trunc_u_f64)
      UNOP(i64_extend_s_i32)
      UNOP(i64_extend_u_i32)
      UNOP(i64_trunc_s_f32)
      UNOP(i64_trunc_u_f32)
      UNOP(i64_trunc_s_f64)
      UNOP(i64_trunc_u_f64)
      UNOP(f32_convert_s_i32)
      UNOP(f32_convert_u_i32)
      UNOP(f32_convert_s_i64)
      UNOP(f32_convert_u_i64)
      UNOP(f32_demote_f64)
      UNOP(f64_convert_s_i32)
      UNOP(f64_convert_u_i32)
      UNOP(f64_convert_s_i64)
      UNOP(f64_convert_u_i64)
      UNOP(f64_promote_f32)
      UNOP(i32_reinterpret_f32)
      UNOP(i64_reinterpret_f64)
      UNOP(f32_reinterpret_i32)
      UNOP(f64_reinterpret_i64)

#undef BINOP
#undef UNOP
#undef OP

      void emit_error() { fb[op_index++] = error_t{}; }
      
      void fix_branch(uint32_t* branch, uint32_t target) { if(branch) *branch = _base_offset + target; }
      void emit_prologue(const func_type& ft, const guarded_vector<local_entry>&, uint32_t idx) {
         op_index = 0;
         // pre-allocate for the function body code, so we have a big blob of memory to work with during function code parsing
         fb = guarded_vector<opcode>{_allocator, _mod->code[idx].size };
      }
     void emit_epilogue(const func_type& ft, const guarded_vector<local_entry>& locals, uint32_t idx) {
         fb.resize(op_index + 1);
         uint32_t locals_count = 0;
         for(uint32_t i = 0; i < locals.size(); ++i) {
            locals_count += locals[i].count;
         }
         fb[fb.size() - 1] = return_t{ static_cast<uint32_t>(locals_count + ft.param_types.size()), ft.return_count, 0, 0 };
      }

      void finalize(function_body& body) {
         fb.resize(op_index + 1);
         body.code = fb.raw();
         body.size = op_index + 1;
         _base_offset += body.size;
      }
    private:

      growable_allocator& _allocator;
      void * _code_segment_base;
      std::size_t op_index = 0;
      guarded_vector<opcode> fb;
      module* _mod;
      std::size_t _base_offset = 0;
   };

}}
