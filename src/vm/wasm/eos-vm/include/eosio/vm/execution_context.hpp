#pragma once

#include <eosio/vm/host_function.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/wasm_stack.hpp>
#include <eosio/vm/watchdog.hpp>

#include <optional>
#include <string>

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
            //std::cout << "before host function call";
            _rhf(_host, *this, _mod.import_functions[index]);
            //std::cout << "after host function call";
            inc_pc();
         } else {
            // const auto& ft = _mod.types[_mod.functions[index - _mod.get_imported_functions_size()]];
            // type_check(ft);
            push_call(index);
            setup_locals(index);
            const uint32_t& pc = _mod.function_sizes[index];
            set_pc(pc);
            _current_offset = pc;
            _code_index     = index - _mod.get_imported_functions_size();
         }
      }

      void print_stack_as() {
         // std::cout << "LABEL { ";
         // for (int i = 0; i < _as.size(); i++) {
         //     auto af = std::get<activation_frame>(_as.get(i));

         //    std::cout << "(" << i << ")";
         //    std::cout << "pc:" << af.pc << "offset:" << af.offset 
         //              <<"index:" << af.index << "op_index:" << af.op_index 
         //              <<"ret_type:" << af.ret_type << "last_cs_size:" << af.last_cs_size ;
         //    std::cout << " }\n";
         //  }
      }


      void print_stack_label() {
         //std::cout << "LABEL { ";
         // for (int i = 0; i < _cs.size(); i++) {
         //    std::cout << "(" << i << ")";
         //    if (std::holds_alternative<block_t>(_cs.get(i))){
         //       auto label = std::get<block_t>(_cs.get(i));
         //       std::cout << "block:" << "data:" << label.data << "pc:" << label.pc <<"index:" << label.index << "op_index:" << label.op_index << ", ";
         //    }else if (std::holds_alternative<loop_t>(_cs.get(i))){
         //       auto label = std::get<loop_t>(_cs.get(i));
         //       std::cout << "loop:" << "data:" << label.data << "pc:" << label.pc <<"index:" << label.index << "op_index:" << label.op_index << ", ";
         //    }
         //    else if (std::holds_alternative<if__t>(_cs.get(i))){
         //       auto label = std::get<if__t>(_cs.get(i));
         //       std::cout << "if_:" << "data:" << label.data << "pc:" << label.pc <<"index:" << label.index << "op_index:" << label.op_index << ", ";
         //    }
         //    else if (std::holds_alternative<else__t>(_cs.get(i))){
         //       auto label = std::get<else__t>(_cs.get(i));
         //       std::cout << "else_:" << "data:" << label.data << "pc:" << label.pc <<"index:" << label.index << "op_index:" << label.op_index << ", ";
         //    }else if (std::holds_alternative<end_t>(_cs.get(i))){
         //       auto label = std::get<end_t>(_cs.get(i));
         //       std::cout << "end:" << "data:" << label.data << "pc:" << label.pc <<"index:" << label.index << "op_index:" << label.op_index << ", ";
         //    }else
         //       std::cout << "(INDEX " << _cs.get(i).index() << "), ";
         // }
         // std::cout << " }\n";
      }

      void print_stack() {
         std::cout << "STACK { ";
         for (int i = 0; i < _os.size(); i++) {
            std::cout << "(" << i << ")";
            if (std::holds_alternative<i32_const_t>(_os.get(i)))
               std::cout << "i32:" << std::get<i32_const_t>(_os.get(i)).data.ui << ", ";
            else if (std::holds_alternative<i64_const_t>(_os.get(i)))
               std::cout << "i64:" << std::get<i64_const_t>(_os.get(i)).data.ui << ", ";
            else if (std::holds_alternative<f32_const_t>(_os.get(i)))
               std::cout << "f32:" << std::get<f32_const_t>(_os.get(i)).data.f << ", ";
            else if (std::holds_alternative<f64_const_t>(_os.get(i)))
               std::cout << "f64:" << std::get<f64_const_t>(_os.get(i)).data.f << ", ";
            else
               std::cout << "(INDEX " << _os.get(i).index() << "), ";
         }
         std::cout << " }\n";
      }


      inline operand_stack& get_operand_stack() { return _os; }
      inline module&        get_module() { return _mod; }
      inline void           set_wasm_allocator(wasm_allocator* alloc) { _wasm_alloc = alloc; }
      inline auto           get_wasm_allocator() { return _wasm_alloc; }
      inline char*          linear_memory() { return _linear_memory; }
      inline uint32_t       table_elem(uint32_t i) { return _mod.tables[0].table[i]; }
      inline void           push_label(const stack_elem& el) { _cs.push(el); }
      inline uint16_t       current_label_index() const { return _cs.current_index(); }
      inline void           eat_labels(uint16_t index) { _cs.eat(index); }
      inline void           push_operand(const stack_elem& el) { _os.push(el); }
      inline stack_elem     get_operand(uint16_t index) const { return _os.get(_last_op_index + index); }
      inline void           eat_operands(uint16_t index) { _os.eat(index); }
      inline void           set_operand(uint16_t index, const stack_elem& el) { _os.set(_last_op_index + index, el); }
      inline uint16_t       current_operands_index() const { return _os.current_index(); }
      inline void           push_call(const stack_elem& el) { _as.push(el); }
      inline stack_elem     pop_call() { return _as.pop(); }
      inline void           push_call(uint32_t index) {
         const auto& ftype = _mod.get_function_type(index);
         _last_op_index    = _os.size() - ftype.param_types.size();
         // _as.push(activation_frame{ _pc + 1, _current_offset, _code_index, static_cast<uint16_t>(_last_op_index),
         //                            ftype.return_type });
         _as.push(activation_frame{ _pc + 1, _current_offset, _code_index, static_cast<uint16_t>(_last_op_index),
                                    ftype.return_type, _cs.size() });
         //xiaoyu 20190727
         // _cs.push(end_t{ 0, static_cast<uint32_t>(_current_offset + _mod.code[_code_index].code.size() - 1), 0,
         //                 static_cast<uint16_t>(_last_op_index) });

         print_stack_as();

      }

      inline void apply_pop_call() {

        // if ( _cs.size() )
        //     _cs.pop();

        //std::cout << "apply_pop_call:" << "1-------------------------------------------------------------------------------------" << "\n";
        //print_stack_label();

         if (_as.size()) {
            //print_stack_as();
            //const auto& af      = std::get<activation_frame>(_as.pop());
            auto af      = std::get<activation_frame>(_as.pop());
            // std::cout << "pc:" << af.pc << "offset:" << af.offset 
            // <<"index:" << af.index << "op_index:" << af.op_index 
            // <<"ret_type:" << af.ret_type << "last_cs_size:" << af.last_cs_size ;

            _current_offset     = af.offset;
            _pc                 = af.pc;
            _code_index         = af.index;
            uint8_t    ret_type = af.ret_type;
            uint16_t   op_index = af.op_index;
            stack_elem el;
            if (ret_type) {
               el = pop_operand();
               EOS_WB_ASSERT(is_a<i32_const_t>(el) && ret_type == types::i32 ||
                                   is_a<i64_const_t>(el) && ret_type == types::i64 ||
                                   is_a<f32_const_t>(el) && ret_type == types::f32 ||
                                   is_a<f64_const_t>(el) && ret_type == types::f64,
                             wasm_interpreter_exception, "wrong return type");
            }

            eat_operands(op_index);
            if (ret_type)
               push_operand(el);
            if (_as.size()) {
               _last_op_index = std::get<activation_frame>(_as.peek()).op_index;
            }

            //xiaoyu 20190727
            while ( _cs.size() > af.last_cs_size)
              _cs.pop();

            //print_stack_as();
        // std::cout << "last_cs_size:" << af.last_cs_size << "\n";
            //print_stack_label();

         }


        // std::cout << "apply_pop_call:" << "2-------------------------------------------------------------------------------------" << "\n";
        // print_stack_label();

        // if ( _cs.size() )
        //     _cs.pop();


      }
      inline stack_elem  pop_label() { return _cs.pop(); }
      inline stack_elem  pop_operand() { return _os.pop(); }
      inline stack_elem& peek_operand(size_t i = 0) { return _os.peek(i); }
      inline stack_elem  get_global(uint32_t index) {
         EOS_WB_ASSERT(index < _mod.globals.size(), wasm_interpreter_exception, "global index out of range");
         const auto& gl = _mod.globals[index];
         // computed g
         switch (gl.type.content_type) {
            case types::i32: return i32_const_t{ *(uint32_t*)&gl.init.value.i32 };
            case types::i64: return i64_const_t{ *(uint64_t*)&gl.init.value.i64 };
            case types::f32: return f32_const_t{ gl.init.value.f32 };
            case types::f64: return f64_const_t{ gl.init.value.f64 };
            default: throw wasm_interpreter_exception{ "invalid global type" };
         }
      }
      inline void set_global(uint32_t index, const stack_elem& el) {
         EOS_WB_ASSERT(index < _mod.globals.size(), wasm_interpreter_exception, "global index out of range");
         auto& gl = _mod.globals[index];
         EOS_WB_ASSERT(gl.type.mutability, wasm_interpreter_exception, "global is not mutable");
         std::visit(overloaded{ [&](const i32_const_t& i) {
                                  EOS_WB_ASSERT(gl.type.content_type == types::i32, wasm_interpreter_exception,
                                                "expected i32 global type");
                                  gl.init.value.i32 = i.data.ui;
                               },
                                [&](const i64_const_t& i) {
                                   EOS_WB_ASSERT(gl.type.content_type == types::i64, wasm_interpreter_exception,
                                                 "expected i64 global type");
                                   gl.init.value.i64 = i.data.ui;
                                },
                                [&](const f32_const_t& f) {
                                   EOS_WB_ASSERT(gl.type.content_type == types::f32, wasm_interpreter_exception,
                                                 "expected f32 global type");
                                   gl.init.value.f32 = f.data.ui;
                                },
                                [&](const f64_const_t& f) {
                                   EOS_WB_ASSERT(gl.type.content_type == types::f64, wasm_interpreter_exception,
                                                 "expected f64 global type");
                                   gl.init.value.f64 = f.data.ui;
                                },
                                [](auto) { throw wasm_interpreter_exception{ "invalid global type" }; } },
                    el);
      }

      inline bool is_true(const stack_elem& el) {
         bool ret_val = false;
         std::visit(overloaded{ [&](const i32_const_t& i32) { ret_val = i32.data.ui; },
                                [&](auto) { throw wasm_invalid_element{ "should be an i32 type" }; } },
                    el);
         return ret_val;
      }

      inline void type_check(const func_type& ft) {
         for (int i = 0; i < ft.param_types.size(); i++) {
            const auto& op = peek_operand((ft.param_types.size() - 1) - i);
            std::visit(overloaded{ [&](const i32_const_t&) {
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

      inline uint32_t get_pc() const { return _pc; }
      inline void     set_pc(uint32_t pc) { _pc = pc; }
      inline void     set_relative_pc(uint32_t pc) { _pc = _current_offset + pc; }
      inline void     inc_pc() { _pc++; }
      inline uint32_t get_code_index() const { return _code_index; }
      inline uint32_t get_code_offset() const { return _pc - _current_offset; }
      inline void     exit(std::error_code err = std::error_code()) {
         std::cout << "Exiting...\n";
         _error_code = err;
         _executing  = false;
      }
      inline std::error_code get_error_code() const { return _error_code; }
      inline bool            executing() const { return _executing; }

      template <typename Visitor, typename... Args>
      inline std::optional<stack_elem> execute_func_table(Host* host, Visitor&& visitor, uint32_t table_index,
                                                          Args... args) {
         return execute(host, std::forward<Visitor>(visitor), table_elem(table_index), std::forward<Args>(args)...);
      }

      template <typename Visitor, typename... Args>
      inline std::optional<stack_elem> execute(Host* host, Visitor&& visitor, const std::string_view func,
                                               Args... args) {
         uint32_t func_index = _mod.get_exported_function(func);
         return execute(host, std::forward<Visitor>(visitor), func_index, std::forward<Args>(args)...);
      }

      template <typename Visitor, typename... Args>
      inline std::optional<stack_elem> execute(Host* host, Visitor&& visitor, uint32_t func_index, Args... args) {
         EOS_WB_ASSERT(func_index < std::numeric_limits<uint32_t>::max(), wasm_interpreter_exception,
                       "cannot execute function, function not found");
         _host          = host;
         _linear_memory = _wasm_alloc->get_base_ptr<char>();
	 grow_linear_memory(_mod.memories[0].limits.initial);
         for (int i = 0; i < _mod.data.size(); i++) {
            const auto& data_seg = _mod.data[i];
            // TODO validate only use memory idx 0 in parse
            auto addr = _linear_memory + data_seg.offset.value.i32;
            memcpy((char*)(addr), data_seg.data.raw(), data_seg.data.size());
         }
         _current_function = func_index;
         _code_index       = func_index - _mod.import_functions.size();
         _current_offset   = _mod.function_sizes[_current_function];
         _exit_pc          = _current_offset + _mod.code[_code_index].code.size() - 1;
         _pc               = _exit_pc - 1; // set to exit for return

         _executing = true;
         _os.eat(0);
         _as.eat(0);
         _cs.eat(0);

         push_args(args...);
         push_call(func_index);
         // type_check(_mod.types[_mod.functions[func_index - _mod.import_functions.size()]]);
         setup_locals(func_index);
         _pc = _current_offset; // set to actual start of function

         execute(visitor);
         stack_elem ret;
         try {
            ret = pop_operand();
         } catch (...) { return {}; }
         _os.eat(0);
         return ret;
      }

      inline void jump(uint32_t label) {
         stack_elem el = pop_label();
         for (int i = 0; i < label; i++) el = pop_label();
         uint16_t op_index = 0;
         uint32_t ret      = 0;
         std::visit(
               overloaded{ [&](const end_t& e) {
                             _pc      = e.pc;
                             op_index = e.op_index;
                          },
                           [&](const block_t& bt) {
                              _pc      = _current_offset + bt.pc + 1;
                              ret      = bt.data;
                              op_index = bt.op_index;
                              //std::cout << "block_t jump pc: " << _current_offset << "_pc "<< _pc - _current_offset << "\n"; 
                           },
                           [&](const loop_t& lt) {
                              _pc      = _current_offset + lt.pc + 1;
                              ret      = lt.data;
                              op_index = lt.op_index;
                              _cs.push(el);
                           },
                           [&](const if__t& it) {
                              _pc      = _current_offset + it.pc + 1;
                              ret      = it.data;
                              op_index = it.op_index;
                           },
                           [&](auto) { throw wasm_invalid_element{ "invalid element when popping control stack" }; } },
               el);

         if (ret != types::pseudo) {
            const auto& op = pop_operand();
            eat_operands(op_index);
            push_operand(op);
         } else {
            eat_operands(op_index);
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

      template <typename Visitor>
      void execute(Visitor&& visitor) {
         //int op_count = 0;
         do {
            //op_count ++;
            uint32_t offset = _pc - _current_offset;
            if (_pc == _exit_pc && _as.size() <= 1) {
               _executing = false;
            }
            std::visit(visitor, _mod.code.at_no_check(_code_index).code.at_no_check(offset));
         } while (_executing);

         //std::cout << "opcodes:" << op_count <<std::endl;
      }

      bounded_allocator _base_allocator = {
         (constants::max_stack_size + constants::max_call_depth + constants::max_nested_structures) * sizeof(stack_elem)
      };
      uint32_t                        _pc               = 0;
      uint32_t                        _exit_pc          = 0;
      uint32_t                        _current_function = 0;
      uint32_t                        _code_index       = 0;
      uint32_t                        _current_offset   = 0;
      uint16_t                        _last_op_index    = 0;
      bool                            _executing        = false;
      char*                           _linear_memory    = nullptr;
      module&                         _mod;
      wasm_allocator*                 _wasm_alloc;
      Host*                           _host;
      control_stack                   _cs = { _base_allocator };
      operand_stack                   _os = { _base_allocator };
      call_stack                      _as = { _base_allocator };
      registered_host_functions<Host> _rhf;
      std::error_code                 _error_code;
   };
}} // namespace eosio::vm
