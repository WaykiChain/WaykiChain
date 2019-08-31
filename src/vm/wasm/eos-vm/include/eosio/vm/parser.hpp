#pragma once

#include <eosio/vm/constants.hpp>
#include <eosio/vm/outcome.hpp>
#include <eosio/vm/sections.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/vector.hpp>

#include <stack>
#include <vector>

namespace eosio { namespace vm {
   class binary_parser {
    public:
      binary_parser(growable_allocator& alloc) : _allocator(alloc) {}

      template <typename T>
      using vec = guarded_vector<T>;

      static inline uint8_t parse_varuint1(wasm_code_ptr& code) { return varuint<1>(code).to(); }

      static inline uint8_t parse_varuint7(wasm_code_ptr& code) { return varuint<7>(code).to(); }

      static inline uint32_t parse_varuint32(wasm_code_ptr& code) { return varuint<32>(code).to(); }

      static inline int8_t parse_varint7(wasm_code_ptr& code) { return varint<7>(code).to(); }

      static inline int32_t parse_varint32(wasm_code_ptr& code) { return varint<32>(code).to(); }

      static inline int64_t parse_varint64(wasm_code_ptr& code) { return varint<64>(code).to(); }

      inline module& parse_module(wasm_code& code, module& mod) {
         wasm_code_ptr cp(code.data(), 0);
         parse_module(cp, code.size(), mod);
         return mod;
      }

      inline module& parse_module2(wasm_code_ptr& code_ptr, size_t sz, module& mod) {
         parse_module(code_ptr, sz, mod);
         return mod;
      }

      void parse_module(wasm_code_ptr& code_ptr, size_t sz, module& mod) {
         _mod = &mod;
         EOS_WB_ASSERT(parse_magic(code_ptr) == constants::magic, wasm_parse_exception, "magic number did not match");
         EOS_WB_ASSERT(parse_version(code_ptr) == constants::version, wasm_parse_exception,
                       "version number did not match");
         for (int i = 0; i < section_id::num_of_elems; i++) {
            if (code_ptr.offset() == sz)
               return;
            code_ptr.add_bounds(constants::id_size);
            auto id = parse_section_id(code_ptr);
            code_ptr.add_bounds(constants::varuint32_size);
            auto len = parse_section_payload_len(code_ptr);
            code_ptr.fit_bounds(len);

            switch (id) {
               case section_id::custom_section: code_ptr += len; break;
               case section_id::type_section: parse_section<section_id::type_section>(code_ptr, mod.types); break;
               case section_id::import_section: parse_section<section_id::import_section>(code_ptr, mod.imports); break;
               case section_id::function_section:
                  parse_section<section_id::function_section>(code_ptr, mod.functions);
                  break;
               case section_id::table_section: parse_section<section_id::table_section>(code_ptr, mod.tables); break;
               case section_id::memory_section:
                  parse_section<section_id::memory_section>(code_ptr, mod.memories);
                  break;
               case section_id::global_section: parse_section<section_id::global_section>(code_ptr, mod.globals); break;
               case section_id::export_section: parse_section<section_id::export_section>(code_ptr, mod.exports); break;
               case section_id::start_section: parse_section<section_id::start_section>(code_ptr, mod.start); break;
               case section_id::element_section:
                  parse_section<section_id::element_section>(code_ptr, mod.elements);
                  break;
               case section_id::code_section: parse_section<section_id::code_section>(code_ptr, mod.code); break;
               case section_id::data_section: parse_section<section_id::data_section>(code_ptr, mod.data); break;
               default: EOS_WB_ASSERT(false, wasm_parse_exception, "error invalid section id");
            }
         }
      }

      inline uint32_t parse_magic(wasm_code_ptr& code) {
         code.add_bounds(constants::magic_size);
         const auto magic = *((uint32_t*)code.raw());
         code += sizeof(uint32_t);
         return magic;
      }
      inline uint32_t parse_version(wasm_code_ptr& code) {
         code.add_bounds(constants::version_size);
         const auto version = *((uint32_t*)code.raw());
         code += sizeof(uint32_t);
         return version;
      }
      inline uint8_t  parse_section_id(wasm_code_ptr& code) { return *code++; }
      inline uint32_t parse_section_payload_len(wasm_code_ptr& code) { return parse_varuint32(code); }

      void parse_import_entry(wasm_code_ptr& code, import_entry& entry) {
         auto len         = parse_varuint32(code);
         entry.module_str = decltype(entry.module_str){ _allocator, len };
         entry.module_str.copy(code.raw(), len);
         code += len;
         len             = parse_varuint32(code);
         entry.field_str = decltype(entry.field_str){ _allocator, len };
         entry.field_str.copy(code.raw(), len);
         code += len;
         entry.kind = (external_kind)(*code++);
         auto type  = parse_varuint32(code);
         switch ((uint8_t)entry.kind) {
            case external_kind::Function: entry.type.func_t = type; break;
            default: EOS_WB_ASSERT(false, wasm_unsupported_import_exception, "only function imports are supported");
         }
      }

      void parse_table_type(wasm_code_ptr& code, table_type& tt) {
         tt.element_type   = *code++;
         tt.limits.flags   = *code++;
         tt.limits.initial = parse_varuint32(code);
         if (tt.limits.flags) {
            tt.limits.maximum = parse_varuint32(code);
            tt.table          = decltype(tt.table){ _allocator, tt.limits.maximum };
            for (int i = 0; i < tt.limits.maximum; i++) tt.table[i] = std::numeric_limits<uint32_t>::max();
         } else {
            tt.table = decltype(tt.table){ _allocator, tt.limits.initial };
            for (int i = 0; i < tt.limits.initial; i++) tt.table[i] = std::numeric_limits<uint32_t>::max();
         }
      }

      void parse_global_variable(wasm_code_ptr& code, global_variable& gv) {
         uint8_t ct           = *code++;
         gv.type.content_type = ct;
         EOS_WB_ASSERT(ct == types::i32 || ct == types::i64 || ct == types::f32 || ct == types::f64,
                       wasm_parse_exception, "invalid global content type");

         gv.type.mutability = *code++;
         parse_init_expr(code, gv.init);
      }

      void parse_memory_type(wasm_code_ptr& code, memory_type& mt) {
         mt.limits.flags   = *code++;
         mt.limits.initial = parse_varuint32(code);
         if (mt.limits.flags) {
            mt.limits.maximum = parse_varuint32(code);
         }
      }

      void parse_export_entry(wasm_code_ptr& code, export_entry& entry) {
         auto len        = parse_varuint32(code);
         entry.field_str = decltype(entry.field_str){ _allocator, len };
         entry.field_str.copy(code.raw(), len);
         code += len;
         entry.kind  = (external_kind)(*code++);
         entry.index = parse_varuint32(code);
      }

      void parse_func_type(wasm_code_ptr& code, func_type& ft) {
         ft.form                              = *code++;
         decltype(ft.param_types) param_types = { _allocator, parse_varuint32(code) };
         for (size_t i = 0; i < param_types.size(); i++) {
            uint8_t pt        = *code++;
            param_types.at(i) = pt;
            EOS_WB_ASSERT(pt == types::i32 || pt == types::i64 || pt == types::f32 || pt == types::f64,
                          wasm_parse_exception, "invalid function param type");
         }
         ft.param_types  = std::move(param_types);
         ft.return_count = *code++;
         EOS_WB_ASSERT(ft.return_count < 2, wasm_parse_exception, "invalid function return count");
         if (ft.return_count > 0)
            ft.return_type = *code++;
      }

      void parse_elem_segment(wasm_code_ptr& code, elem_segment& es) {
         table_type* tt = nullptr;
         for (int i = 0; i < _mod->tables.size(); i++) {
            if (_mod->tables[i].element_type == types::anyfunc)
               tt = &(_mod->tables[i]);
         }
         EOS_WB_ASSERT(tt != nullptr, wasm_parse_exception, "table not declared");
         es.index = parse_varuint32(code);
         EOS_WB_ASSERT(es.index == 0, wasm_parse_exception, "only table index of 0 is supported");
         parse_init_expr(code, es.offset);
         uint32_t           size  = parse_varuint32(code);
         decltype(es.elems) elems = { _allocator, size };
         for (uint32_t i = 0; i < size; i++) {
            uint32_t index                     = parse_varuint32(code);
            tt->table[es.offset.value.i32 + i] = index;
            elems.at(i)                        = index;
         }
         es.elems = std::move(elems);
      }

      void parse_init_expr(wasm_code_ptr& code, init_expr& ie) {
         ie.opcode = *code++;
         switch (ie.opcode) {
            case opcodes::i32_const: ie.value.i32 = parse_varint32(code); break;
            case opcodes::i64_const: ie.value.i64 = parse_varint64(code); break;
            case opcodes::f32_const:
               ie.value.i32 = *code.raw();
               code += sizeof(uint32_t);
               break;
            case opcodes::f64_const:
               ie.value.i64 = *code.raw();
               code += sizeof(uint64_t);
               break;
            default:
               EOS_WB_ASSERT(false, wasm_parse_exception,
                             "initializer expression can only acception i32.const, i64.const, f32.const and f64.const");
         }
         EOS_WB_ASSERT((*code++) == opcodes::end, wasm_parse_exception, "no end op found");
      }

      void parse_function_body(wasm_code_ptr& code, function_body& fb) {
         const auto&         body_size = parse_varuint32(code);
         const auto&         before    = code.offset();
         const auto&         local_cnt = parse_varuint32(code);
         decltype(fb.locals) locals    = { _allocator, local_cnt };
         // parse the local entries
         for (size_t i = 0; i < local_cnt; i++) {
            locals.at(i).count = parse_varuint32(code);
            locals.at(i).type  = *code++;
         }
         fb.locals = std::move(locals);

         size_t            bytes = body_size - (code.offset() - before); // -1 is 'end' 0xb byte
         decltype(fb.code) _code = { _allocator, bytes };
         wasm_code_ptr     fb_code(code.raw(), bytes);
         parse_function_body_code(fb_code, bytes, _code);
         code += bytes - 1;
         EOS_WB_ASSERT(*code++ == 0x0B, wasm_parse_exception, "failed parsing function body, expected 'end'");
         _code[_code.size() - 1] = fend_t{};
         fb.code                 = std::move(_code);
      }

      void parse_function_body_code(wasm_code_ptr& code, size_t bounds, guarded_vector<opcode>& fb) {
         size_t op_index       = 0;
         auto   parse_br_table = [&](wasm_code_ptr& code, br_table_t& bt) {
            size_t                   table_size = parse_varuint32(code);
            guarded_vector<uint32_t> br_tab{ _allocator, table_size };
            for (size_t i = 0; i < table_size; i++) br_tab[i] = parse_varuint32(code);
            bt.table          = br_tab.raw();
            bt.size           = table_size;
            bt.default_target = parse_varuint32(code);
         };

         std::stack<uint32_t> pc_stack;

         while (code.offset() < bounds) {
            EOS_WB_ASSERT(pc_stack.size() <= constants::max_nested_structures, wasm_parse_exception,
                          "nested structures validation failure");

            switch (*code++) {
               case opcodes::unreachable: fb[op_index++] = unreachable_t{}; break;
               case opcodes::nop: fb[op_index++] = nop_t{}; break;
               case opcodes::end: {
                  if (pc_stack.size()) {
                     auto& el = fb[pc_stack.top()];
                     std::visit(overloaded{ [=](block_t& bt) { bt.pc = op_index; },
                                            [=](loop_t& lt) { lt.pc = pc_stack.top(); },
                                            [=](if__t& it) { it.pc = op_index; },
                                            [=](else__t& et) { et.pc = op_index; },
                                            [=](auto&&) {
                                               throw wasm_invalid_element{ "invalid element when popping pc stack" };
                                            } },
                                el);
                     pc_stack.pop();
                  }
                  fb[op_index++] = end_t{};
                  break;
                  break;
               }
               case opcodes::return_: fb[op_index++] = return__t{}; break;
               case opcodes::block:
                  pc_stack.push(op_index);
                  fb[op_index++] = block_t{ *code++ };
                  break;
                  break;
               case opcodes::loop:
                  pc_stack.push(op_index);
                  fb[op_index++] = loop_t{ *code++ };
                  break;
                  break;
               case opcodes::if_:
                  pc_stack.push(op_index);
                  fb[op_index++] = if__t{ *code++ };
                  break;
                  break;
               case opcodes::else_: {
                  auto old_index = pc_stack.top();
                  pc_stack.pop();
                  pc_stack.push(op_index);
                  auto& _if      = std::get<if__t>(fb[old_index]);
                  _if.pc         = op_index;
                  fb[op_index++] = else__t{};
                  break;
                  break;
               }
               case opcodes::br: fb[op_index++] = br_t{ parse_varuint32(code) }; break;
               case opcodes::br_if: fb[op_index++] = br_if_t{ parse_varuint32(code) }; break;
               case opcodes::br_table: {
                  br_table_t bt;
                  parse_br_table(code, bt);
                  fb[op_index++] = bt;
               } break;
               case opcodes::call: fb[op_index++] = call_t{ parse_varuint32(code) }; break;
               case opcodes::call_indirect:
                  fb[op_index++] = call_indirect_t{ parse_varuint32(code) };
                  code++;
                  break;
               case opcodes::drop: fb[op_index++] = drop_t{}; break;
               case opcodes::select: fb[op_index++] = select_t{}; break;
               case opcodes::get_local: fb[op_index++] = get_local_t{ parse_varuint32(code) }; break;
               case opcodes::set_local: fb[op_index++] = set_local_t{ parse_varuint32(code) }; break;
               case opcodes::tee_local: fb[op_index++] = tee_local_t{ parse_varuint32(code) }; break;
               case opcodes::get_global: fb[op_index++] = get_global_t{ parse_varuint32(code) }; break;
               case opcodes::set_global: fb[op_index++] = set_global_t{ parse_varuint32(code) }; break;
               case opcodes::i32_load:
                  fb[op_index++] = i32_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load:
                  fb[op_index++] = i64_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::f32_load:
                  fb[op_index++] = f32_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::f64_load:
                  fb[op_index++] = f64_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_load8_s:
                  fb[op_index++] = i32_load8_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_load16_s:
                  fb[op_index++] = i32_load16_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_load8_u:
                  fb[op_index++] = i32_load8_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_load16_u:
                  fb[op_index++] = i32_load16_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load8_s:
                  fb[op_index++] = i64_load8_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load16_s:
                  fb[op_index++] = i64_load16_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load32_s:
                  fb[op_index++] = i64_load32_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load8_u:
                  fb[op_index++] = i64_load8_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load16_u:
                  fb[op_index++] = i64_load16_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_load32_u:
                  fb[op_index++] = i64_load32_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_store:
                  fb[op_index++] = i32_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_store:
                  fb[op_index++] = i64_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::f32_store:
                  fb[op_index++] = f32_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::f64_store:
                  fb[op_index++] = f64_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_store8:
                  fb[op_index++] = i32_store8_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i32_store16:
                  fb[op_index++] = i32_store16_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_store8:
                  fb[op_index++] = i64_store8_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_store16:
                  fb[op_index++] = i64_store16_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::i64_store32:
                  fb[op_index++] = i64_store32_t{ parse_varuint32(code), parse_varuint32(code) };
                  break;
               case opcodes::current_memory:
                  fb[op_index++] = current_memory_t{};
                  code++;
                  break;
               case opcodes::grow_memory:
                  fb[op_index++] = grow_memory_t{};
                  code++;
                  break;
               case opcodes::i32_const: fb[op_index++] = i32_const_t{ parse_varint32(code) }; break;
               case opcodes::i64_const: fb[op_index++] = i64_const_t{ parse_varint64(code) }; break;
               case opcodes::f32_const:
                  fb[op_index++] = f32_const_t{ *(float*)code.raw() };
                  code += 4;
                  break;
               case opcodes::f64_const:
                  fb[op_index++] = f64_const_t{ *(double*)code.raw() };
                  code += 8;
                  break;
               case opcodes::i32_eqz: fb[op_index++] = i32_eqz_t{}; break;
               case opcodes::i32_eq: fb[op_index++] = i32_eq_t{}; break;
               case opcodes::i32_ne: fb[op_index++] = i32_ne_t{}; break;
               case opcodes::i32_lt_s: fb[op_index++] = i32_lt_s_t{}; break;
               case opcodes::i32_lt_u: fb[op_index++] = i32_lt_u_t{}; break;
               case opcodes::i32_gt_s: fb[op_index++] = i32_gt_s_t{}; break;
               case opcodes::i32_gt_u: fb[op_index++] = i32_gt_u_t{}; break;
               case opcodes::i32_le_s: fb[op_index++] = i32_le_s_t{}; break;
               case opcodes::i32_le_u: fb[op_index++] = i32_le_u_t{}; break;
               case opcodes::i32_ge_s: fb[op_index++] = i32_ge_s_t{}; break;
               case opcodes::i32_ge_u: fb[op_index++] = i32_ge_u_t{}; break;
               case opcodes::i64_eqz: fb[op_index++] = i64_eqz_t{}; break;
               case opcodes::i64_eq: fb[op_index++] = i64_eq_t{}; break;
               case opcodes::i64_ne: fb[op_index++] = i64_ne_t{}; break;
               case opcodes::i64_lt_s: fb[op_index++] = i64_lt_s_t{}; break;
               case opcodes::i64_lt_u: fb[op_index++] = i64_lt_u_t{}; break;
               case opcodes::i64_gt_s: fb[op_index++] = i64_gt_s_t{}; break;
               case opcodes::i64_gt_u: fb[op_index++] = i64_gt_u_t{}; break;
               case opcodes::i64_le_s: fb[op_index++] = i64_le_s_t{}; break;
               case opcodes::i64_le_u: fb[op_index++] = i64_le_u_t{}; break;
               case opcodes::i64_ge_s: fb[op_index++] = i64_ge_s_t{}; break;
               case opcodes::i64_ge_u: fb[op_index++] = i64_ge_u_t{}; break;
               case opcodes::f32_eq: fb[op_index++] = f32_eq_t{}; break;
               case opcodes::f32_ne: fb[op_index++] = f32_ne_t{}; break;
               case opcodes::f32_lt: fb[op_index++] = f32_lt_t{}; break;
               case opcodes::f32_gt: fb[op_index++] = f32_gt_t{}; break;
               case opcodes::f32_le: fb[op_index++] = f32_le_t{}; break;
               case opcodes::f32_ge: fb[op_index++] = f32_ge_t{}; break;
               case opcodes::f64_eq: fb[op_index++] = f64_eq_t{}; break;
               case opcodes::f64_ne: fb[op_index++] = f64_ne_t{}; break;
               case opcodes::f64_lt: fb[op_index++] = f64_lt_t{}; break;
               case opcodes::f64_gt: fb[op_index++] = f64_gt_t{}; break;
               case opcodes::f64_le: fb[op_index++] = f64_le_t{}; break;
               case opcodes::f64_ge: fb[op_index++] = f64_ge_t{}; break;
               case opcodes::i32_clz: fb[op_index++] = i32_clz_t{}; break;
               case opcodes::i32_ctz: fb[op_index++] = i32_ctz_t{}; break;
               case opcodes::i32_popcnt: fb[op_index++] = i32_popcnt_t{}; break;
               case opcodes::i32_add: fb[op_index++] = i32_add_t{}; break;
               case opcodes::i32_sub: fb[op_index++] = i32_sub_t{}; break;
               case opcodes::i32_mul: fb[op_index++] = i32_mul_t{}; break;
               case opcodes::i32_div_s: fb[op_index++] = i32_div_s_t{}; break;
               case opcodes::i32_div_u: fb[op_index++] = i32_div_u_t{}; break;
               case opcodes::i32_rem_s: fb[op_index++] = i32_rem_s_t{}; break;
               case opcodes::i32_rem_u: fb[op_index++] = i32_rem_u_t{}; break;
               case opcodes::i32_and: fb[op_index++] = i32_and_t{}; break;
               case opcodes::i32_or: fb[op_index++] = i32_or_t{}; break;
               case opcodes::i32_xor: fb[op_index++] = i32_xor_t{}; break;
               case opcodes::i32_shl: fb[op_index++] = i32_shl_t{}; break;
               case opcodes::i32_shr_s: fb[op_index++] = i32_shr_s_t{}; break;
               case opcodes::i32_shr_u: fb[op_index++] = i32_shr_u_t{}; break;
               case opcodes::i32_rotl: fb[op_index++] = i32_rotl_t{}; break;
               case opcodes::i32_rotr: fb[op_index++] = i32_rotr_t{}; break;
               case opcodes::i64_clz: fb[op_index++] = i64_clz_t{}; break;
               case opcodes::i64_ctz: fb[op_index++] = i64_ctz_t{}; break;
               case opcodes::i64_popcnt: fb[op_index++] = i64_popcnt_t{}; break;
               case opcodes::i64_add: fb[op_index++] = i64_add_t{}; break;
               case opcodes::i64_sub: fb[op_index++] = i64_sub_t{}; break;
               case opcodes::i64_mul: fb[op_index++] = i64_mul_t{}; break;
               case opcodes::i64_div_s: fb[op_index++] = i64_div_s_t{}; break;
               case opcodes::i64_div_u: fb[op_index++] = i64_div_u_t{}; break;
               case opcodes::i64_rem_s: fb[op_index++] = i64_rem_s_t{}; break;
               case opcodes::i64_rem_u: fb[op_index++] = i64_rem_u_t{}; break;
               case opcodes::i64_and: fb[op_index++] = i64_and_t{}; break;
               case opcodes::i64_or: fb[op_index++] = i64_or_t{}; break;
               case opcodes::i64_xor: fb[op_index++] = i64_xor_t{}; break;
               case opcodes::i64_shl: fb[op_index++] = i64_shl_t{}; break;
               case opcodes::i64_shr_s: fb[op_index++] = i64_shr_s_t{}; break;
               case opcodes::i64_shr_u: fb[op_index++] = i64_shr_u_t{}; break;
               case opcodes::i64_rotl: fb[op_index++] = i64_rotl_t{}; break;
               case opcodes::i64_rotr: fb[op_index++] = i64_rotr_t{}; break;
               case opcodes::f32_abs: fb[op_index++] = f32_abs_t{}; break;
               case opcodes::f32_neg: fb[op_index++] = f32_neg_t{}; break;
               case opcodes::f32_ceil: fb[op_index++] = f32_ceil_t{}; break;
               case opcodes::f32_floor: fb[op_index++] = f32_floor_t{}; break;
               case opcodes::f32_trunc: fb[op_index++] = f32_trunc_t{}; break;
               case opcodes::f32_nearest: fb[op_index++] = f32_nearest_t{}; break;
               case opcodes::f32_sqrt: fb[op_index++] = f32_sqrt_t{}; break;
               case opcodes::f32_add: fb[op_index++] = f32_add_t{}; break;
               case opcodes::f32_sub: fb[op_index++] = f32_sub_t{}; break;
               case opcodes::f32_mul: fb[op_index++] = f32_mul_t{}; break;
               case opcodes::f32_div: fb[op_index++] = f32_div_t{}; break;
               case opcodes::f32_min: fb[op_index++] = f32_min_t{}; break;
               case opcodes::f32_max: fb[op_index++] = f32_max_t{}; break;
               case opcodes::f32_copysign: fb[op_index++] = f32_copysign_t{}; break;
               case opcodes::f64_abs: fb[op_index++] = f64_abs_t{}; break;
               case opcodes::f64_neg: fb[op_index++] = f64_neg_t{}; break;
               case opcodes::f64_ceil: fb[op_index++] = f64_ceil_t{}; break;
               case opcodes::f64_floor: fb[op_index++] = f64_floor_t{}; break;
               case opcodes::f64_trunc: fb[op_index++] = f64_trunc_t{}; break;
               case opcodes::f64_nearest: fb[op_index++] = f64_nearest_t{}; break;
               case opcodes::f64_sqrt: fb[op_index++] = f64_sqrt_t{}; break;
               case opcodes::f64_add: fb[op_index++] = f64_add_t{}; break;
               case opcodes::f64_sub: fb[op_index++] = f64_sub_t{}; break;
               case opcodes::f64_mul: fb[op_index++] = f64_mul_t{}; break;
               case opcodes::f64_div: fb[op_index++] = f64_div_t{}; break;
               case opcodes::f64_min: fb[op_index++] = f64_min_t{}; break;
               case opcodes::f64_max: fb[op_index++] = f64_max_t{}; break;
               case opcodes::f64_copysign: fb[op_index++] = f64_copysign_t{}; break;
               case opcodes::i32_wrap_i64: fb[op_index++] = i32_wrap_i64_t{}; break;
               case opcodes::i32_trunc_s_f32: fb[op_index++] = i32_trunc_s_f32_t{}; break;
               case opcodes::i32_trunc_u_f32: fb[op_index++] = i32_trunc_u_f32_t{}; break;
               case opcodes::i32_trunc_s_f64: fb[op_index++] = i32_trunc_s_f64_t{}; break;
               case opcodes::i32_trunc_u_f64: fb[op_index++] = i32_trunc_u_f64_t{}; break;
               case opcodes::i64_extend_s_i32: fb[op_index++] = i64_extend_s_i32_t{}; break;
               case opcodes::i64_extend_u_i32: fb[op_index++] = i64_extend_u_i32_t{}; break;
               case opcodes::i64_trunc_s_f32: fb[op_index++] = i64_trunc_s_f32_t{}; break;
               case opcodes::i64_trunc_u_f32: fb[op_index++] = i64_trunc_u_f32_t{}; break;
               case opcodes::i64_trunc_s_f64: fb[op_index++] = i64_trunc_s_f64_t{}; break;
               case opcodes::i64_trunc_u_f64: fb[op_index++] = i64_trunc_u_f64_t{}; break;
               case opcodes::f32_convert_s_i32: fb[op_index++] = f32_convert_s_i32_t{}; break;
               case opcodes::f32_convert_u_i32: fb[op_index++] = f32_convert_u_i32_t{}; break;
               case opcodes::f32_convert_s_i64: fb[op_index++] = f32_convert_s_i64_t{}; break;
               case opcodes::f32_convert_u_i64: fb[op_index++] = f32_convert_u_i64_t{}; break;
               case opcodes::f32_demote_f64: fb[op_index++] = f32_demote_f64_t{}; break;
               case opcodes::f64_convert_s_i32: fb[op_index++] = f64_convert_s_i32_t{}; break;
               case opcodes::f64_convert_u_i32: fb[op_index++] = f64_convert_u_i32_t{}; break;
               case opcodes::f64_convert_s_i64: fb[op_index++] = f64_convert_s_i64_t{}; break;
               case opcodes::f64_convert_u_i64: fb[op_index++] = f64_convert_u_i64_t{}; break;
               case opcodes::f64_promote_f32: fb[op_index++] = f64_promote_f32_t{}; break;
               case opcodes::i32_reinterpret_f32: fb[op_index++] = i32_reinterpret_f32_t{}; break;
               case opcodes::i64_reinterpret_f64: fb[op_index++] = i64_reinterpret_f64_t{}; break;
               case opcodes::f32_reinterpret_i32: fb[op_index++] = f32_reinterpret_i32_t{}; break;
               case opcodes::f64_reinterpret_i64: fb[op_index++] = f64_reinterpret_i64_t{}; break;
               case opcodes::error: fb[op_index++] = error_t{}; break;
            }
         }
         fb.resize(op_index);
      }

      void parse_data_segment(wasm_code_ptr& code, data_segment& ds) {
         ds.index = parse_varuint32(code);
         parse_init_expr(code, ds.offset);
         ds.data = decltype(ds.data){ _allocator, parse_varuint32(code) };
         ds.data.copy(code.raw(), ds.data.size());
         code += ds.data.size();
      }

      template <typename Elem, typename ParseFunc>
      inline void parse_section_impl(wasm_code_ptr& code, vec<Elem>& elems, ParseFunc&& elem_parse) {
         auto count = parse_varuint32(code);
         elems      = vec<Elem>{ _allocator, count };
         for (size_t i = 0; i < count; i++) { elem_parse(code, elems.at(i)); }
      }

      template <uint8_t id>
      inline void parse_section_header(wasm_code_ptr& code) {
         code.add_bounds(constants::id_size);
         auto _id = parse_section_id(code);
         // ignore custom sections
         if (_id == section_id::custom_section) {
            code.add_bounds(constants::varuint32_size);
            code += parse_section_payload_len(code);
            code.fit_bounds(constants::id_size);
            _id = parse_section_id(code);
         }
         EOS_WB_ASSERT(_id == id, wasm_parse_exception, "Section id does not match");
         code.add_bounds(constants::varuint32_size);
         code.fit_bounds(parse_section_payload_len(code));
      }

      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                             code,
                                vec<typename std::enable_if_t<id == section_id::type_section, func_type>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, func_type& ft) { parse_func_type(code, ft); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                  code,
                                vec<typename std::enable_if_t<id == section_id::import_section, import_entry>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, import_entry& ie) { parse_import_entry(code, ie); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                code,
                                vec<typename std::enable_if_t<id == section_id::function_section, uint32_t>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, uint32_t& elem) { elem = parse_varuint32(code); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                               code,
                                vec<typename std::enable_if_t<id == section_id::table_section, table_type>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, table_type& tt) { parse_table_type(code, tt); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                 code,
                                vec<typename std::enable_if_t<id == section_id::memory_section, memory_type>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, memory_type& mt) { parse_memory_type(code, mt); });
      }
      template <uint8_t id>
      inline void
      parse_section(wasm_code_ptr&                                                                     code,
                    vec<typename std::enable_if_t<id == section_id::global_section, global_variable>>& elems) {
         parse_section_impl(code, elems,
                            [&](wasm_code_ptr& code, global_variable& gv) { parse_global_variable(code, gv); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                  code,
                                vec<typename std::enable_if_t<id == section_id::export_section, export_entry>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, export_entry& ee) { parse_export_entry(code, ee); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                        code,
                                typename std::enable_if_t<id == section_id::start_section, uint32_t>& start) {
         start = parse_varuint32(code);
      }
      template <uint8_t id>
      inline void
      parse_section(wasm_code_ptr&                                                                   code,
                    vec<typename std::enable_if_t<id == section_id::element_section, elem_segment>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, elem_segment& es) { parse_elem_segment(code, es); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                 code,
                                vec<typename std::enable_if_t<id == section_id::code_section, function_body>>& elems) {
         parse_section_impl(code, elems,
                            [&](wasm_code_ptr& code, function_body& fb) { parse_function_body(code, fb); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                code,
                                vec<typename std::enable_if_t<id == section_id::data_section, data_segment>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, data_segment& ds) { parse_data_segment(code, ds); });
      }

      template <size_t N>
      varint<N> parse_varint(const wasm_code& code, size_t index) {
         varint<N> result(0);
         result.set(code, index);
         return result;
      }

      template <size_t N>
      varuint<N> parse_varuint(const wasm_code& code, size_t index) {
         varuint<N> result(0);
         result.set(code, index);
         return result;
      }

    private:
      growable_allocator& _allocator;
      module*             _mod; // non-owning weak pointer
   };
}} // namespace eosio::vm
