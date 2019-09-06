#pragma once

#include <eosio/vm/host_function.hpp>
#include <eosio/vm/signals.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/wasm_stack.hpp>
#include <eosio/vm/watchdog.hpp>

#include <optional>
#include <string>
#include <utility>

namespace eosio { namespace vm {
   template <typename Host>
   class execution_context {
    public:
      execution_context(module& m) : _linear_memory(nullptr), _mod(m) {
         for (int i = 0; i < _mod.exports.size(); i++) _mod.import_functions.resize(_mod.get_imported_functions_size());
         _mod.function_sizes.resize(_mod.get_functions_total());
         const size_t import_size  = _mod.get_imported_functions_size();
         uint32_t     total_so_far = 0;
         for (int i = _mod.get_imported_functions_size(); i < _mod.function_sizes.size(); i++) {
            _mod.function_sizes[i] = total_so_far;
            total_so_far += _mod.code[i - import_size].code.size();
         }
      }

      inline int32_t grow_linear_memory(int32_t pages) {
         EOS_WB_ASSERT(!(_mod.memories[0].limits.flags && (_mod.memories[0].limits.maximum < pages)), wasm_interpreter_exception, "memory limit reached");
         const int32_t sz = _wasm_alloc->get_current_page();
         if (pages < 0 || !_mod.memories.size() || (_mod.memories[0].limits.flags && (_mod.memories[0].limits.maximum < sz + pages)))
            return -1;
         _wasm_alloc->alloc<char>(pages);
         return sz;
      }

      inline int32_t current_linear_memory() const { return _wasm_alloc->get_current_page(); }

      inline void call(uint32_t index) {
         // TODO validate index is valid
         if (index < _mod.get_imported_functions_size()) {
            // TODO validate only importing functions
            const auto& ft = _mod.types[_mod.imports[index].type.func_t];
            type_check(ft);
            _rhf(_state.host, *this, _mod.import_functions[index]);
            inc_pc();
         } else {
            // const auto& ft = _mod.types[_mod.functions[index - _mod.get_imported_functions_size()]];
            // type_check(ft);
            push_call(index);
            setup_locals(index);
            const uint32_t& pc = _mod.function_sizes[index];
            set_pc(pc);
            _state.current_offset = pc;
            _state.code_index     = index - _mod.get_imported_functions_size();
         }
      }

      void print_stack() {
         std::cout << "STACK { ";
         for (int i = 0; i < _os.size(); i++) {
            std::cout << "(" << i << ")";
            visit(overloaded { [&](i32_const_t el) { std::cout << "i32:" << el.data.ui << ", "; },
                               [&](i64_const_t el) { std::cout << "i64:" << el.data.ui << ", "; },
                               [&](f32_const_t el) { std::cout << "f32:" << el.data.f << ", "; },
                               [&](f64_const_t el) { std::cout << "f64:" << el.data.f << ", "; },
                               [&](auto el) { std::cout << "(INDEX " << el.index() << "), "; } }, _os.get(i));
         }
         std::cout << " }\n";
      }

      inline operand_stack& get_operand_stack() { return _os; }
      inline module&        get_module() { return _mod; }
      inline void           set_wasm_allocator(wasm_allocator* alloc) { _wasm_alloc = alloc; }
      inline auto           get_wasm_allocator() { return _wasm_alloc; }
      inline char*          linear_memory() { return _linear_memory; }
      inline uint32_t       table_elem(uint32_t i) { return _mod.tables[0].table[i]; }
      inline void           push_operand(const operand_stack_elem& el) { _os.push(el); }
      inline operand_stack_elem     get_operand(uint16_t index) const { return _os.get(_last_op_index + index); }
      inline void           eat_operands(uint16_t index) { _os.eat(index); }
      inline void           set_operand(uint16_t index, const operand_stack_elem& el) { _os.set(_last_op_index + index, el); }
      inline uint16_t       current_operands_index() const { return _os.current_index(); }
      inline void           push_call(const activation_frame& el) { _as.push(el); }
      inline activation_frame     pop_call() { return _as.pop(); }
      inline uint32_t       call_depth()const { return _as.size(); }
      template <bool Should_Exit=false>
      inline void           push_call(uint32_t index) {
         const auto& ftype = _mod.get_function_type(index);
         _last_op_index    = _os.size() - ftype.param_types.size();
         if constexpr (Should_Exit) {
            _as.push(activation_frame{ static_cast<uint32_t>(-1), static_cast<uint32_t>(-1), static_cast<uint32_t>(-1), static_cast<uint16_t>(_last_op_index),
                                    ftype.return_type });
         } else {
            _as.push(activation_frame{ _state.pc + 1, _state.current_offset, _state.code_index, static_cast<uint16_t>(_last_op_index),
                                    ftype.return_type });
         }
      }

      inline void apply_pop_call() {
         const auto& af = _as.pop();
         const uint8_t    ret_type = af.ret_type;
         const uint16_t   op_index = af.op_index;
         operand_stack_elem el;
         if (ret_type) {
            el = pop_operand();
            EOS_WB_ASSERT(el.is_a<i32_const_t>() && ret_type == types::i32 ||
                                   el.is_a<i64_const_t>() && ret_type == types::i64 ||
                                   el.is_a<f32_const_t>() && ret_type == types::f32 ||
                                   el.is_a<f64_const_t>() && ret_type == types::f64,
                             wasm_interpreter_exception, "wrong return type");
         }
         if (af.offset == -1 && af.pc == -1 && af.index == -1) {
            set_exiting_op(_state.exiting_loc);
            _state.pc = 0;
            _state.current_offset = 0;
            _state.code_index = 0;
         } else {
            _state.current_offset     = af.offset;
            _state.pc                 = af.pc;
            _state.code_index         = af.index;
            _last_op_index = _as.peek().op_index;

         }
         eat_operands(op_index);
         if (ret_type)
            push_operand(el);
      }
      inline operand_stack_elem  pop_operand() { return _os.pop(); }
      inline operand_stack_elem& peek_operand(size_t i = 0) { return _os.peek(i); }
      inline operand_stack_elem  get_global(uint32_t index) {
         EOS_WB_ASSERT(index < _mod.globals.size(), wasm_interpreter_exception, "global index out of range");
         const auto& gl = _mod.globals[index];
         switch (gl.type.content_type) {
            case types::i32: return i32_const_t{ *(uint32_t*)&gl.current.value.i32 };
            case types::i64: return i64_const_t{ *(uint64_t*)&gl.current.value.i64 };
            case types::f32: return f32_const_t{ gl.current.value.f32 };
            case types::f64: return f64_const_t{ gl.current.value.f64 };
            default: throw wasm_interpreter_exception{ "invalid global type" };
         }
      }

      inline void set_global(uint32_t index, const operand_stack_elem& el) {
         EOS_WB_ASSERT(index < _mod.globals.size(), wasm_interpreter_exception, "global index out of range");
         auto& gl = _mod.globals[index];
         EOS_WB_ASSERT(gl.type.mutability, wasm_interpreter_exception, "global is not mutable");
         visit(overloaded{ [&](const i32_const_t& i) {
                                  EOS_WB_ASSERT(gl.type.content_type == types::i32, wasm_interpreter_exception,
                                                "expected i32 global type");
                                  gl.current.value.i32 = i.data.ui;
                               },
                                [&](const i64_const_t& i) {
                                   EOS_WB_ASSERT(gl.type.content_type == types::i64, wasm_interpreter_exception,
                                                 "expected i64 global type");
                                   gl.current.value.i64 = i.data.ui;
                                },
                                [&](const f32_const_t& f) {
                                   EOS_WB_ASSERT(gl.type.content_type == types::f32, wasm_interpreter_exception,
                                                 "expected f32 global type");
                                   gl.current.value.f32 = f.data.ui;
                                },
                                [&](const f64_const_t& f) {
                                   EOS_WB_ASSERT(gl.type.content_type == types::f64, wasm_interpreter_exception,
                                                 "expected f64 global type");
                                   gl.current.value.f64 = f.data.ui;
                                },
                                [](auto) { throw wasm_interpreter_exception{ "invalid global type" }; } },
                    el);
      }

      inline bool is_true(const operand_stack_elem& el) {
         bool ret_val = false;
         visit(overloaded{ [&](const i32_const_t& i32) { ret_val = i32.data.ui; },
                           [&](auto) { throw wasm_invalid_element{ "should be an i32 type" }; } },
                    el);
         return ret_val;
      }

      inline void type_check(const func_type& ft) {
         for (int i = 0; i < ft.param_types.size(); i++) {
            const auto& op = peek_operand((ft.param_types.size() - 1) - i);
            visit(overloaded{ [&](const i32_const_t&) {
                                     EOS_WB_ASSERT(ft.param_types[i] == types::i32, wasm_interpreter_exception,
                                                   "function param type mismatch");
                                  },
                                   [&](const f32_const_t&) {
                                      EOS_WB_ASSERT(ft.param_types[i] == types::f32, wasm_interpreter_exception,
                                                    "function param type mismatch");
                                   },
                                   [&](const i64_const_t&) {
                                      EOS_WB_ASSERT(ft.param_types[i] == types::i64, wasm_interpreter_exception,
                                                    "function param type mismatch");
                                   },
                                   [&](const f64_const_t&) {
                                      EOS_WB_ASSERT(ft.param_types[i] == types::f64, wasm_interpreter_exception,
                                                    "function param type mismatch");
                                   },
                                   [&](auto) { throw wasm_interpreter_exception{ "function param invalid type" }; } },
                       op);
         }
      }

      inline uint32_t get_pc() const { return _state.pc; }
      inline void     set_pc(uint32_t pc) { _state.pc = pc; }
      inline void     set_relative_pc(uint32_t pc) { _state.pc = _state.current_offset + pc; }
      inline void     inc_pc() { _state.pc++; }
      inline uint32_t get_code_index() const { return _state.code_index; }
      inline uint32_t get_code_offset() const { return _state.pc - _state.current_offset; }
      inline void     exit(std::error_code err = std::error_code()) {
         _error_code = err;
         clear_exiting_op(_state.exiting_loc);
         _state.exiting_loc = { _state.code_index, (_state.pc+1)-_state.current_offset };
         set_exiting_op(_state.exiting_loc);
      }

      inline void reset() {
         _linear_memory = _wasm_alloc->get_base_ptr<char>();
         if (_mod.memories.size()) {
            grow_linear_memory(_mod.memories[0].limits.initial - _wasm_alloc->get_current_page());
         }

         for (int i = 0; i < _mod.data.size(); i++) {
            const auto& data_seg = _mod.data[i];
            // TODO validate only use memory idx 0 in parse
            auto addr = _linear_memory + data_seg.offset.value.i32;
            memcpy((char*)(addr), data_seg.data.raw(), data_seg.data.size());
         }

         // reset the mutable globals
         for (int i = 0; i < _mod.globals.size(); i++) {
            if (_mod.globals[i].type.mutability)
               _mod.globals[i].current = _mod.globals[i].init;
         }
         _state = execution_state{};
         _os.eat(_state.os_index);
         _as.eat(_state.as_index);

      }
      
      inline void set_exiting_op( const std::pair<uint32_t, uint32_t>& exiting_loc ) {
         if (exiting_loc.first != -1 && exiting_loc.second != -1) {
            _mod.code.at(exiting_loc.first).code.at(exiting_loc.second).set_exiting_which();
         }
      }

      inline void clear_exiting_op( const std::pair<uint32_t, uint32_t>& exiting_loc ) {
         if (exiting_loc.first != -1 && exiting_loc.second != -1) {
            _mod.code.at(exiting_loc.first).code.at(exiting_loc.second).clear_exiting_which();
         }
      }

      inline std::error_code get_error_code() const { return _error_code; }

      template <typename Visitor, typename... Args>
      inline std::optional<operand_stack_elem> execute_func_table(Host* host, Visitor&& visitor, uint32_t table_index,
                                                          Args... args) {
         return execute(host, std::forward<Visitor>(visitor), table_elem(table_index), std::forward<Args>(args)...);
      }

      template <typename Visitor, typename... Args>
      inline std::optional<operand_stack_elem> execute(Host* host, Visitor&& visitor, const std::string_view func,
                                               Args... args) {
         uint32_t func_index = _mod.get_exported_function(func);
         return execute(host, std::forward<Visitor>(visitor), func_index, std::forward<Args>(args)...);
      }

      template <typename Visitor, typename... Args>
      inline void execute_start(Host* host, Visitor&& visitor) {
         if (_mod.start != std::numeric_limits<uint32_t>::max())
            execute(host, std::forward<Visitor>(visitor), _mod.start);
      }

      template <typename Visitor, typename... Args>
      inline std::optional<operand_stack_elem> execute(Host* host, Visitor&& visitor, uint32_t func_index, Args... args) {
         EOS_WB_ASSERT(func_index < std::numeric_limits<uint32_t>::max(), wasm_interpreter_exception,
                       "cannot execute function, function not found");

         auto last_last_op_index = _last_op_index;

         clear_exiting_op( _state.exiting_loc );
         // save the state of the original calling context
         execution_state saved_state = _state;

         _linear_memory = _wasm_alloc->get_base_ptr<char>();

         _state.host             = host;
         _state.current_function = func_index;
         _state.code_index       = func_index - _mod.import_functions.size();
         _state.current_offset   = _mod.function_sizes[_state.current_function];
         _state.pc               = _state.current_offset;
         _state.exiting_loc      = {0, 0};
         _state.as_index         = _as.size();
         _state.os_index         = _os.size();

         push_args(args...);
         push_call<true>(func_index);
         type_check(_mod.types[_mod.functions[func_index - _mod.import_functions.size()]]);
         setup_locals(func_index);

         vm::invoke_with_signal_handler([&]() {
            execute(visitor);
         }, [](int sig) {
            switch(sig) {
             case SIGSEGV:
             case SIGBUS:
               break;
             default:
               /* TODO fix this */
               assert(!"??????");
            }
            throw wasm_memory_exception{ "wasm memory out-of-bounds" };
         });

         std::optional<operand_stack_elem> ret;
         if (_mod.get_function_type(func_index).return_count) {
            ret = pop_operand();
         }

         // revert the state back to original calling context
         clear_exiting_op( _state.exiting_loc );
         _os.eat(_state.os_index);
         _as.eat(_state.as_index);
         _state = saved_state;

         _last_op_index = last_last_op_index;

         return ret;
      }

      inline void jump(uint32_t pop_info, uint32_t new_pc) {
         _state.pc = _state.current_offset + new_pc;
         if ((pop_info & 0x80000000u)) {
            const auto& op = pop_operand();
            eat_operands(_os.size() - ((pop_info & 0x7FFFFFFFu) - 1));
            push_operand(op);
         } else {
            eat_operands(_os.size() - pop_info);
         }
      }

    private:
      template <typename Arg, typename... Args>
      void _push_args(Arg&& arg, Args&&... args) {
         if constexpr (to_wasm_type_v<std::decay_t<Arg>> == types::i32)
            push_operand({ i32_const_t{ static_cast<uint32_t>(arg) } });
         else if constexpr (to_wasm_type_v<std::decay_t<Arg>> == types::f32)
            push_operand(f32_const_t{ static_cast<float>(arg) });
         else if constexpr (to_wasm_type_v<std::decay_t<Arg>> == types::i64)
            push_operand(i64_const_t{ static_cast<uint64_t>(arg) });
         else
            push_operand(f64_const_t{ static_cast<double>(arg) });
         if constexpr (sizeof...(Args) > 0)
            _push_args(args...);
      }

      template <typename... Args>
      void push_args(Args&&... args) {
         if constexpr (sizeof...(Args) > 0)
            _push_args(args...);
      }

      inline void setup_locals(uint32_t index) {
         const auto& fn = _mod.code[index - _mod.get_imported_functions_size()];
         for (int i = 0; i < fn.locals.size(); i++) {
            for (int j = 0; j < fn.locals[i].count; j++)
               // computed g
               switch (fn.locals[i].type) {
                  case types::i32: push_operand(i32_const_t{ (uint32_t)0 }); break;
                  case types::i64: push_operand(i64_const_t{ (uint64_t)0 }); break;
                  case types::f32: push_operand(f32_const_t{ (uint32_t)0 }); break;
                  case types::f64: push_operand(f64_const_t{ (uint64_t)0 }); break;
                  default: throw wasm_interpreter_exception{ "invalid function param type" };
               }
         }
      }

#define CREATE_TABLE_ENTRY(NAME, CODE) &&ev_label_##NAME,
#define CREATE_EXITING_TABLE_ENTRY(NAME, CODE) &&ev_label_exiting_##NAME,
#define CREATE_LABEL(NAME, CODE)                                                                                  \
      ev_label_##NAME : visitor(ev_variant->template get<eosio::vm::EOS_VM_OPCODE_T(NAME)>());                    \
      ev_variant = &_mod.code.at_no_check(_state.code_index).code.at_no_check(_state.pc - _state.current_offset); \
      goto* dispatch_table[ev_variant->index()];
#define CREATE_EXITING_LABEL(NAME, CODE)                                                  \
      ev_label_exiting_##NAME :  \
      return;
#define CREATE_EMPTY_LABEL(NAME, CODE) ev_label_##NAME :  \
      throw wasm_interpreter_exception{"empty operand"};

      template <typename Visitor>
      void execute(Visitor&& visitor) {
         static void* dispatch_table[] = {
            EOS_VM_CONTROL_FLOW_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_BR_TABLE_OP(CREATE_TABLE_ENTRY)
            EOS_VM_RETURN_OP(CREATE_TABLE_ENTRY)
            EOS_VM_CALL_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_PARAMETRIC_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_VARIABLE_ACCESS_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_MEMORY_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_I32_CONSTANT_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_I64_CONSTANT_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_F32_CONSTANT_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_F64_CONSTANT_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_COMPARISON_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_NUMERIC_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_CONVERSION_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_SYNTHETIC_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_EMPTY_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_ERROR_OPS(CREATE_TABLE_ENTRY)
            EOS_VM_CONTROL_FLOW_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_BR_TABLE_OP(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_RETURN_OP(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_CALL_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_PARAMETRIC_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_VARIABLE_ACCESS_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_MEMORY_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_I32_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_I64_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_F32_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_F64_CONSTANT_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_COMPARISON_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_NUMERIC_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_CONVERSION_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_SYNTHETIC_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_EMPTY_OPS(CREATE_EXITING_TABLE_ENTRY)
            EOS_VM_ERROR_OPS(CREATE_EXITING_TABLE_ENTRY)
            &&__ev_last
         };
         auto* ev_variant = &_mod.code.at_no_check(_state.code_index).code.at_no_check(_state.pc - _state.current_offset);
         goto *dispatch_table[ev_variant->index()];
         while (1) {
             EOS_VM_CONTROL_FLOW_OPS(CREATE_LABEL);
             EOS_VM_BR_TABLE_OP(CREATE_LABEL);
             EOS_VM_RETURN_OP(CREATE_LABEL);
             EOS_VM_CALL_OPS(CREATE_LABEL);
             EOS_VM_PARAMETRIC_OPS(CREATE_LABEL);
             EOS_VM_VARIABLE_ACCESS_OPS(CREATE_LABEL);
             EOS_VM_MEMORY_OPS(CREATE_LABEL);
             EOS_VM_I32_CONSTANT_OPS(CREATE_LABEL);
             EOS_VM_I64_CONSTANT_OPS(CREATE_LABEL);
             EOS_VM_F32_CONSTANT_OPS(CREATE_LABEL);
             EOS_VM_F64_CONSTANT_OPS(CREATE_LABEL);
             EOS_VM_COMPARISON_OPS(CREATE_LABEL);
             EOS_VM_NUMERIC_OPS(CREATE_LABEL);
             EOS_VM_CONVERSION_OPS(CREATE_LABEL);
             EOS_VM_SYNTHETIC_OPS(CREATE_LABEL);
             EOS_VM_EMPTY_OPS(CREATE_EMPTY_LABEL);
             EOS_VM_ERROR_OPS(CREATE_LABEL);
             EOS_VM_CONTROL_FLOW_OPS(CREATE_EXITING_LABEL);
             EOS_VM_BR_TABLE_OP(CREATE_EXITING_LABEL);
             EOS_VM_RETURN_OP(CREATE_EXITING_LABEL);
             EOS_VM_CALL_OPS(CREATE_EXITING_LABEL);
             EOS_VM_PARAMETRIC_OPS(CREATE_EXITING_LABEL);
             EOS_VM_VARIABLE_ACCESS_OPS(CREATE_EXITING_LABEL);
             EOS_VM_MEMORY_OPS(CREATE_EXITING_LABEL);
             EOS_VM_I32_CONSTANT_OPS(CREATE_EXITING_LABEL);
             EOS_VM_I64_CONSTANT_OPS(CREATE_EXITING_LABEL);
             EOS_VM_F32_CONSTANT_OPS(CREATE_EXITING_LABEL);
             EOS_VM_F64_CONSTANT_OPS(CREATE_EXITING_LABEL);
             EOS_VM_COMPARISON_OPS(CREATE_EXITING_LABEL);
             EOS_VM_NUMERIC_OPS(CREATE_EXITING_LABEL);
             EOS_VM_CONVERSION_OPS(CREATE_EXITING_LABEL);
             EOS_VM_SYNTHETIC_OPS(CREATE_EXITING_LABEL);
             EOS_VM_EMPTY_OPS(CREATE_EXITING_LABEL);
             EOS_VM_ERROR_OPS(CREATE_EXITING_LABEL);
             __ev_last:
                throw wasm_interpreter_exception{"should never reach here"};
         }
      }

#undef CREATE_EMPTY_LABEL
#undef CREATE_EXITING_LABEL
#undef CREATE_LABEL
#undef CREATE_EXITING_TABLE_ENTRY
#undef CREATE_TABLE_ENTRY

      struct execution_state {
         Host* host                                = nullptr;
         uint32_t current_function                 = 0;
         std::pair<int64_t, int64_t> exiting_loc = {-1,-1};
	 uint32_t as_index         = 0;
	 uint32_t cs_index         = 0;
	 uint32_t os_index         = 0;
         uint32_t code_index       = 0;
         uint32_t current_offset   = 0;
         uint32_t pc               = 0;
         bool     initialized      = false;
      };

      bounded_allocator _base_allocator = {
         (constants::max_stack_size + constants::max_call_depth) * (std::max(sizeof(operand_stack_elem), sizeof(activation_frame)))
      };
      execution_state _state;
      uint16_t                        _last_op_index    = 0;
      char*                           _linear_memory    = nullptr;
      module&                         _mod;
      wasm_allocator*                 _wasm_alloc;
      operand_stack                   _os = { _base_allocator };
      call_stack                      _as = { _base_allocator };
      registered_host_functions<Host> _rhf;
      std::error_code                 _error_code;
   };
}} // namespace eosio::vm
