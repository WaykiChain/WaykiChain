#pragma once

#include <eosio/vm/constants.hpp>
#include <eosio/vm/outcome.hpp>
#include <eosio/vm/sections.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/vector.hpp>

#include <set>
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
         gv.current = gv.init;
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
	 if (entry.kind == external_kind::Function) {
             _export_indices.insert(entry.index);
	 }
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

      void parse_function_body(wasm_code_ptr& code, function_body& fb, std::size_t idx) {
         const auto&         fn_type   = _mod->types.at(_mod->functions.at(idx));

         const auto&         body_size = parse_varuint32(code);
         const auto&         before    = code.offset();
         const auto&         local_cnt = parse_varuint32(code);
         _current_function_index++;
         decltype(fb.locals) locals    = { _allocator, local_cnt };
         // parse the local entries
         for (size_t i = 0; i < local_cnt; i++) {
            locals.at(i).count = parse_varuint32(code);
            locals.at(i).type  = *code++;
         }
         fb.locals = std::move(locals);

         // -1 is 'end' 0xb byte and one extra slot for an exiting instruction to be held during execution, this is used to drive the pc past normal execution
         size_t            bytes = (body_size - (code.offset() - before));
         decltype(fb.code) _code = { _allocator, bytes };
         wasm_code_ptr     fb_code(code.raw(), bytes);
         parse_function_body_code(fb_code, bytes, _code, fn_type);
         code += bytes - 1;
         EOS_WB_ASSERT(*code++ == 0x0B, wasm_parse_exception, "failed parsing function body, expected 'end'");
         _code[_code.size() - 1] = fend_t{};
         fb.code                 = std::move(_code);
      }

      // The control stack holds either address of the target of the
      // label (for backward jumps) or a list of instructions to be
      // updated (for forward jumps).
      //
      // Inside an if: The first element refers to the `if` and should
      // jump to `else`.  The remaining elements should branch to `end`
      struct pc_element_t {
         uint32_t operand_depth;
         uint32_t expected_result;
         bool is_unreachable;
         std::variant<uint32_t, std::vector<uint32_t*>> relocations;
      };

      void parse_function_body_code(wasm_code_ptr& code, size_t bounds, guarded_vector<opcode>& fb, const func_type& ft) {
         size_t op_index       = 0;

         // Initialize the control stack with the current function as the sole element
         uint32_t operand_depth = 0;
         std::vector<pc_element_t> pc_stack{{
               operand_depth,
               ft.return_count?ft.return_type:static_cast<uint32_t>(types::pseudo),
               false,
               std::vector<uint32_t*>{}}};

         // writes the continuation of a label to address.  If the continuation
         // is not yet available, address will be recorded in the relocations
         // list for label.
         //
         // Also writes the number of operands that need to be popped to
         // operand_depth_change.  If the label has a return value it will
         // be counted in this, and the high bit will be set to signal
         // its presence.
         auto handle_branch_target = [&](uint32_t label, uint32_t* address, uint32_t* operand_depth_change) {
            EOS_WB_ASSERT(label < pc_stack.size(), wasm_parse_exception, "invalid label");
            pc_element_t& branch_target = pc_stack[pc_stack.size() - label - 1];
            uint32_t original_operand_depth = branch_target.operand_depth;
            uint32_t target = 0xDEADBEEF;
            uint32_t current_operand_depth = operand_depth;
            if(branch_target.expected_result != types::pseudo) {
               // FIXME: Reusing the high bit imposes an additional constraint
               // on the maximum depth of the operand stack.  This isn't an
               // actual problem right now, because the stack is hard-coded
               // to 8192 elements, but it would be better to avoid spreading
               // this assumption around the code.
               original_operand_depth |= 0x80000000;
            }
            std::visit(overloaded{ [&](uint32_t target) { *address = target; },
                                   [&](std::vector<uint32_t*>& relocations) { relocations.push_back(address); } },
               branch_target.relocations);
            *operand_depth_change = operand_depth - original_operand_depth;
         };
         
         auto parse_br_table = [&](wasm_code_ptr& code, br_table_t& bt) {
            size_t                   table_size = parse_varuint32(code);
            guarded_vector<br_table_t::elem_t> br_tab{ _allocator, table_size + 1 };
            for (size_t i = 0; i < table_size + 1; i++) {
               auto& elem = br_tab.at(i);
               handle_branch_target(parse_varuint32(code), &elem.pc, &elem.stack_pop);
            }
            bt.table          = br_tab.raw();
            bt.size           = table_size;
         };

         // appends an instruction to fb and returns a reference to it.
         auto append_instr = [&](auto&& instr) -> decltype(auto) {
            fb[op_index] = instr;
            return fb[op_index++].get<std::decay_t<decltype(instr)>>();
         };


         // Unconditional branches effectively make the state of the
         // stack unconstrained.  FIXME: Note that the unreachable instructions
         // still need to be validated, for consistency, which this impementation
         // fails to do.  Also, note that this variable is strictly for validation
         // purposes, and nested unreachable control structures have this reset
         // to "reachable," since their bodies get validated normally.
         bool is_in_unreachable = false;
         auto start_unreachable = [&]() {
            // We need enough room to push/pop any number of operands.
            operand_depth = 0x80000000;
            is_in_unreachable = true;
         };
         auto is_unreachable = [&]() -> bool {
            return is_in_unreachable;
         };
         auto start_reachable = [&]() { is_in_unreachable = false; };

         // Handles branches to the end of the scope and pops the pc_stack
         auto exit_scope = [&]() {
            if (pc_stack.size()) { // There must be at least one element
               if(auto* relocations = std::get_if<std::vector<uint32_t*>>(&pc_stack.back().relocations)) {
                  for(uint32_t* branch_op : *relocations) {
                     *branch_op = op_index;
                  }
               }
               unsigned expected_operand_depth = pc_stack.back().operand_depth;
               if (pc_stack.back().expected_result != types::pseudo) {
                  ++expected_operand_depth;
               }
               if (!is_unreachable())
                  EOS_WB_ASSERT(operand_depth == expected_operand_depth, wasm_parse_exception, "incorrect stack depth at end");
               operand_depth = expected_operand_depth;
               is_in_unreachable = pc_stack.back().is_unreachable;
               pc_stack.pop_back();
            } else {
               throw wasm_invalid_element{ "unexpected end instruction" };
            }
         };

         // Tracks the operand stack
         auto push_operand = [&](/* uint8_t type */) {
             EOS_WB_ASSERT(operand_depth < 0xFFFFFFFF, wasm_parse_exception, "Wasm stack overflow.");
             ++operand_depth;
         };
         auto pop_operand = [&]() {
            EOS_WB_ASSERT(operand_depth > 0, wasm_parse_exception, "Not enough items on the stack.");
            --operand_depth;
         };
         auto pop_operands = [&](uint32_t num_to_pop) {
            EOS_WB_ASSERT(operand_depth >= num_to_pop, wasm_parse_exception, "Not enough items on the stack.");
            operand_depth -= num_to_pop;
         };

         while (code.offset() < bounds) {
            EOS_WB_ASSERT(pc_stack.size() <= constants::max_nested_structures, wasm_parse_exception,
                          "nested structures validation failure");
            switch (*code++) {
               case opcodes::unreachable: fb[op_index++] = unreachable_t{}; start_unreachable(); break;
               case opcodes::nop: fb[op_index++] = nop_t{}; break;
               case opcodes::end: {
                  exit_scope();
                  break;
               }
               case opcodes::return_: {
                  uint32_t label = pc_stack.size() - 1;
                  return_t& instr = append_instr(return_t{});
                  handle_branch_target(label, &instr.pc, &instr.data);
                  start_unreachable();
	       } break;
               case opcodes::block: {
                  uint32_t expected_result = *code++;
                  pc_stack.push_back({operand_depth, expected_result, is_in_unreachable, std::vector<uint32_t*>{}});
                  start_reachable();
               } break;
               case opcodes::loop: {
                  uint32_t expected_result = *code++;
                  pc_stack.push_back({operand_depth, expected_result, is_in_unreachable, op_index});
                  start_reachable();
               } break;
               case opcodes::if_: {
                  uint32_t expected_result = *code++;
                  if_t& instr = append_instr(if_t{});
                  pop_operand();
                  pc_stack.push_back({operand_depth, expected_result, is_in_unreachable, std::vector{&instr.pc}});
                  start_reachable();
               } break;
               case opcodes::else_: {
                  auto& old_index = pc_stack.back();
                  auto& relocations = std::get<std::vector<uint32_t*>>(old_index.relocations);
                  uint32_t* _if_pc      = relocations[0];
                  // reset the operand stack to the same state as the if
                  if (!is_unreachable()) {
                     EOS_WB_ASSERT((old_index.expected_result != types::pseudo) + old_index.operand_depth == operand_depth,
                                   wasm_parse_exception, "Malformed if body");
                  }
                  operand_depth = old_index.operand_depth;
                  start_reachable();
                  // Overwrite the branch from the `if` with the `else`.
                  // We're left with a normal relocation list where everything
                  // branches to the corresponding `end`
                  auto& else_ = append_instr(else_t{});
                  relocations[0] = &else_.pc;
                  // The branch from the if skips just past the else
                  *_if_pc = op_index;
                  break;
               }
               case opcodes::br: {
                  uint32_t label = parse_varuint32(code);
                  br_t& instr = append_instr(br_t{});
                  handle_branch_target(label, &instr.pc, &instr.data);
                  start_unreachable();
               } break;
               case opcodes::br_if: {
                  uint32_t label = parse_varuint32(code);
                  br_if_t& instr = append_instr(br_if_t{});
                  pop_operand();
                  handle_branch_target(label, &instr.pc, &instr.data);
               } break;
               case opcodes::br_table: {
                  br_table_t bt;
                  pop_operand();
                  parse_br_table(code, bt);
                  fb[op_index++] = bt;
                  start_unreachable();
               } break;
               case opcodes::call: {
                  uint32_t funcnum = parse_varuint32(code);
                  const func_type& ft = _mod->get_function_type(funcnum);
                  pop_operands(ft.param_types.size());
                  EOS_WB_ASSERT(ft.return_count <= 1, wasm_parse_exception, "unsupported");
                  if(ft.return_count == 1)
                     push_operand();
                  fb[op_index++] = call_t{ funcnum };
               } break;
               case opcodes::call_indirect: {
                  uint32_t functypeidx = parse_varuint32(code);
                  const func_type& ft = _mod->types.at(functypeidx);
                  pop_operand();
                  pop_operands(ft.param_types.size());
                  EOS_WB_ASSERT(ft.return_count <= 1, wasm_parse_exception, "unsupported");
                  if(ft.return_count == 1)
                     push_operand();
                  fb[op_index++] = call_indirect_t{ functypeidx };
                  code++; // 0x00
                  break;
               }
               case opcodes::drop: fb[op_index++] = drop_t{}; pop_operand(); break;
               case opcodes::select: fb[op_index++] = select_t{}; pop_operands(3); push_operand(); break;
               case opcodes::get_local: fb[op_index++] = get_local_t{ parse_varuint32(code) }; push_operand(); break;
               case opcodes::set_local: fb[op_index++] = set_local_t{ parse_varuint32(code) }; pop_operand(); break;
               case opcodes::tee_local: fb[op_index++] = tee_local_t{ parse_varuint32(code) }; break;
               case opcodes::get_global: fb[op_index++] = get_global_t{ parse_varuint32(code) }; push_operand(); break;
               case opcodes::set_global: fb[op_index++] = set_global_t{ parse_varuint32(code) }; pop_operand(); break;
               case opcodes::i32_load:
                  fb[op_index++] = i32_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load:
                  fb[op_index++] = i64_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::f32_load:
                  fb[op_index++] = f32_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::f64_load:
                  fb[op_index++] = f64_load_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i32_load8_s:
                  fb[op_index++] = i32_load8_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i32_load16_s:
                  fb[op_index++] = i32_load16_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i32_load8_u:
                  fb[op_index++] = i32_load8_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i32_load16_u:
                  fb[op_index++] = i32_load16_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load8_s:
                  fb[op_index++] = i64_load8_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load16_s:
                  fb[op_index++] = i64_load16_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load32_s:
                  fb[op_index++] = i64_load32_s_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load8_u:
                  fb[op_index++] = i64_load8_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load16_u:
                  fb[op_index++] = i64_load16_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i64_load32_u:
                  fb[op_index++] = i64_load32_u_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operand();
                  push_operand();
                  break;
               case opcodes::i32_store:
                  fb[op_index++] = i32_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::i64_store:
                  fb[op_index++] = i64_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::f32_store:
                  fb[op_index++] = f32_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::f64_store:
                  fb[op_index++] = f64_store_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::i32_store8:
                  fb[op_index++] = i32_store8_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::i32_store16:
                  fb[op_index++] = i32_store16_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::i64_store8:
                  fb[op_index++] = i64_store8_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::i64_store16:
                  fb[op_index++] = i64_store16_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::i64_store32:
                  fb[op_index++] = i64_store32_t{ parse_varuint32(code), parse_varuint32(code) };
                  pop_operands(2);
                  break;
               case opcodes::current_memory:
                  fb[op_index++] = current_memory_t{};
                  push_operand();
                  code++;
                  break;
               case opcodes::grow_memory:
                  fb[op_index++] = grow_memory_t{};
                  pop_operand();
                  push_operand();
                  code++;
                  break;
               case opcodes::i32_const: fb[op_index++] = i32_const_t{ parse_varint32(code) }; push_operand(); break;
               case opcodes::i64_const: fb[op_index++] = i64_const_t{ parse_varint64(code) }; push_operand(); break;
               case opcodes::f32_const:
                  fb[op_index++] = f32_const_t{ *(float*)code.raw() };
                  code += 4;
                  push_operand();
                  break;
               case opcodes::f64_const:
                  fb[op_index++] = f64_const_t{ *(double*)code.raw() };
                  code += 8;
                  push_operand();
                  break;
               case opcodes::i32_eqz: fb[op_index++] = i32_eqz_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_eq: fb[op_index++] = i32_eq_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_ne: fb[op_index++] = i32_ne_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_lt_s: fb[op_index++] = i32_lt_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_lt_u: fb[op_index++] = i32_lt_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_gt_s: fb[op_index++] = i32_gt_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_gt_u: fb[op_index++] = i32_gt_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_le_s: fb[op_index++] = i32_le_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_le_u: fb[op_index++] = i32_le_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_ge_s: fb[op_index++] = i32_ge_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_ge_u: fb[op_index++] = i32_ge_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_eqz: fb[op_index++] = i64_eqz_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_eq: fb[op_index++] = i64_eq_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_ne: fb[op_index++] = i64_ne_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_lt_s: fb[op_index++] = i64_lt_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_lt_u: fb[op_index++] = i64_lt_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_gt_s: fb[op_index++] = i64_gt_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_gt_u: fb[op_index++] = i64_gt_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_le_s: fb[op_index++] = i64_le_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_le_u: fb[op_index++] = i64_le_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_ge_s: fb[op_index++] = i64_ge_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_ge_u: fb[op_index++] = i64_ge_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_eq: fb[op_index++] = f32_eq_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_ne: fb[op_index++] = f32_ne_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_lt: fb[op_index++] = f32_lt_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_gt: fb[op_index++] = f32_gt_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_le: fb[op_index++] = f32_le_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_ge: fb[op_index++] = f32_ge_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_eq: fb[op_index++] = f64_eq_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_ne: fb[op_index++] = f64_ne_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_lt: fb[op_index++] = f64_lt_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_gt: fb[op_index++] = f64_gt_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_le: fb[op_index++] = f64_le_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_ge: fb[op_index++] = f64_ge_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_clz: fb[op_index++] = i32_clz_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_ctz: fb[op_index++] = i32_ctz_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_popcnt: fb[op_index++] = i32_popcnt_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_add: fb[op_index++] = i32_add_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_sub: fb[op_index++] = i32_sub_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_mul: fb[op_index++] = i32_mul_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_div_s: fb[op_index++] = i32_div_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_div_u: fb[op_index++] = i32_div_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_rem_s: fb[op_index++] = i32_rem_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_rem_u: fb[op_index++] = i32_rem_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_and: fb[op_index++] = i32_and_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_or: fb[op_index++] = i32_or_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_xor: fb[op_index++] = i32_xor_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_shl: fb[op_index++] = i32_shl_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_shr_s: fb[op_index++] = i32_shr_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_shr_u: fb[op_index++] = i32_shr_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_rotl: fb[op_index++] = i32_rotl_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_rotr: fb[op_index++] = i32_rotr_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_clz: fb[op_index++] = i64_clz_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_ctz: fb[op_index++] = i64_ctz_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_popcnt: fb[op_index++] = i64_popcnt_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_add: fb[op_index++] = i64_add_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_sub: fb[op_index++] = i64_sub_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_mul: fb[op_index++] = i64_mul_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_div_s: fb[op_index++] = i64_div_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_div_u: fb[op_index++] = i64_div_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_rem_s: fb[op_index++] = i64_rem_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_rem_u: fb[op_index++] = i64_rem_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_and: fb[op_index++] = i64_and_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_or: fb[op_index++] = i64_or_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_xor: fb[op_index++] = i64_xor_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_shl: fb[op_index++] = i64_shl_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_shr_s: fb[op_index++] = i64_shr_s_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_shr_u: fb[op_index++] = i64_shr_u_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_rotl: fb[op_index++] = i64_rotl_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i64_rotr: fb[op_index++] = i64_rotr_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_abs: fb[op_index++] = f32_abs_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_neg: fb[op_index++] = f32_neg_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_ceil: fb[op_index++] = f32_ceil_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_floor: fb[op_index++] = f32_floor_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_trunc: fb[op_index++] = f32_trunc_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_nearest: fb[op_index++] = f32_nearest_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_sqrt: fb[op_index++] = f32_sqrt_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_add: fb[op_index++] = f32_add_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_sub: fb[op_index++] = f32_sub_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_mul: fb[op_index++] = f32_mul_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_div: fb[op_index++] = f32_div_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_min: fb[op_index++] = f32_min_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_max: fb[op_index++] = f32_max_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f32_copysign: fb[op_index++] = f32_copysign_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_abs: fb[op_index++] = f64_abs_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_neg: fb[op_index++] = f64_neg_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_ceil: fb[op_index++] = f64_ceil_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_floor: fb[op_index++] = f64_floor_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_trunc: fb[op_index++] = f64_trunc_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_nearest: fb[op_index++] = f64_nearest_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_sqrt: fb[op_index++] = f64_sqrt_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_add: fb[op_index++] = f64_add_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_sub: fb[op_index++] = f64_sub_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_mul: fb[op_index++] = f64_mul_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_div: fb[op_index++] = f64_div_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_min: fb[op_index++] = f64_min_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_max: fb[op_index++] = f64_max_t{}; pop_operands(2); push_operand(); break;
               case opcodes::f64_copysign: fb[op_index++] = f64_copysign_t{}; pop_operands(2); push_operand(); break;
               case opcodes::i32_wrap_i64: fb[op_index++] = i32_wrap_i64_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_trunc_s_f32: fb[op_index++] = i32_trunc_s_f32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_trunc_u_f32: fb[op_index++] = i32_trunc_u_f32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_trunc_s_f64: fb[op_index++] = i32_trunc_s_f64_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_trunc_u_f64: fb[op_index++] = i32_trunc_u_f64_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_extend_s_i32: fb[op_index++] = i64_extend_s_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_extend_u_i32: fb[op_index++] = i64_extend_u_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_trunc_s_f32: fb[op_index++] = i64_trunc_s_f32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_trunc_u_f32: fb[op_index++] = i64_trunc_u_f32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_trunc_s_f64: fb[op_index++] = i64_trunc_s_f64_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_trunc_u_f64: fb[op_index++] = i64_trunc_u_f64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_convert_s_i32: fb[op_index++] = f32_convert_s_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_convert_u_i32: fb[op_index++] = f32_convert_u_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_convert_s_i64: fb[op_index++] = f32_convert_s_i64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_convert_u_i64: fb[op_index++] = f32_convert_u_i64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_demote_f64: fb[op_index++] = f32_demote_f64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_convert_s_i32: fb[op_index++] = f64_convert_s_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_convert_u_i32: fb[op_index++] = f64_convert_u_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_convert_s_i64: fb[op_index++] = f64_convert_s_i64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_convert_u_i64: fb[op_index++] = f64_convert_u_i64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_promote_f32: fb[op_index++] = f64_promote_f32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i32_reinterpret_f32: fb[op_index++] = i32_reinterpret_f32_t{}; pop_operand(); push_operand(); break;
               case opcodes::i64_reinterpret_f64: fb[op_index++] = i64_reinterpret_f64_t{}; pop_operand(); push_operand(); break;
               case opcodes::f32_reinterpret_i32: fb[op_index++] = f32_reinterpret_i32_t{}; pop_operand(); push_operand(); break;
               case opcodes::f64_reinterpret_i64: fb[op_index++] = f64_reinterpret_i64_t{}; pop_operand(); push_operand(); break;
               case opcodes::error: fb[op_index++] = error_t{}; break;
            }
         }
         fb.resize(op_index + 1);
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
         for (size_t i = 0; i < count; i++) { elem_parse(code, elems.at(i), i); }
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
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, func_type& ft, std::size_t /*idx*/) { parse_func_type(code, ft); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                  code,
                                vec<typename std::enable_if_t<id == section_id::import_section, import_entry>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, import_entry& ie, std::size_t /*idx*/) { parse_import_entry(code, ie); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                code,
                                vec<typename std::enable_if_t<id == section_id::function_section, uint32_t>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, uint32_t& elem, std::size_t /*idx*/) { elem = parse_varuint32(code); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                               code,
                                vec<typename std::enable_if_t<id == section_id::table_section, table_type>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, table_type& tt, std::size_t /*idx*/) { parse_table_type(code, tt); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                 code,
                                vec<typename std::enable_if_t<id == section_id::memory_section, memory_type>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, memory_type& mt, std::size_t /*idx*/) { parse_memory_type(code, mt); });
      }
      template <uint8_t id>
      inline void
      parse_section(wasm_code_ptr&                                                                     code,
                    vec<typename std::enable_if_t<id == section_id::global_section, global_variable>>& elems) {
         parse_section_impl(code, elems,
                            [&](wasm_code_ptr& code, global_variable& gv, std::size_t /*idx*/) { parse_global_variable(code, gv); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                  code,
                                vec<typename std::enable_if_t<id == section_id::export_section, export_entry>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, export_entry& ee, std::size_t /*idx*/) { parse_export_entry(code, ee); });
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
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, elem_segment& es, std::size_t /*idx*/) { parse_elem_segment(code, es); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                 code,
                                vec<typename std::enable_if_t<id == section_id::code_section, function_body>>& elems) {
         parse_section_impl(code, elems,
                            [&](wasm_code_ptr& code, function_body& fb, std::size_t idx) { parse_function_body(code, fb, idx); });
      }
      template <uint8_t id>
      inline void parse_section(wasm_code_ptr&                                                                code,
                                vec<typename std::enable_if_t<id == section_id::data_section, data_segment>>& elems) {
         parse_section_impl(code, elems, [&](wasm_code_ptr& code, data_segment& ds, std::size_t /*idx*/) { parse_data_segment(code, ds); });
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
      int64_t             _current_function_index = -1;
      std::set<uint32_t>  _export_indices;
   };
}} // namespace eosio::vm
