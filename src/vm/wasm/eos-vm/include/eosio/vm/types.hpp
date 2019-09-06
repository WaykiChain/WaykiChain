#pragma once

/*
 * definitions from https://github.com/WebAssembly/design/blob/master/BinaryEncoding.md
 */

#include <eosio/vm/allocator.hpp>
#include <eosio/vm/guarded_ptr.hpp>
#include <eosio/vm/leb128.hpp>
#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/vector.hpp>

#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace eosio { namespace vm {
   enum types { i32 = 0x7f, i64 = 0x7e, f32 = 0x7d, f64 = 0x7c, anyfunc = 0x70, func = 0x60, pseudo = 0x40, ret_void };

   enum external_kind { Function = 0, Table = 1, Memory = 2, Global = 3 };

   typedef uint8_t value_type;
   typedef uint8_t block_type;
   typedef uint8_t elem_type;

   template <typename T>
   using guarded_vector = managed_vector<T, growable_allocator>;

   struct activation_frame {
      uint32_t pc;
      uint32_t offset;
      uint32_t index;
      uint16_t op_index;
      uint8_t  ret_type;
   };

   struct resizable_limits {
      bool     flags;
      uint32_t initial;
      uint32_t maximum = 0;
   };

   struct func_type {
      value_type                 form; // value for the func type constructor
      guarded_vector<value_type> param_types;
      uint8_t                    return_count;
      value_type                 return_type;
   };

   union expr_value {
      int32_t  i32;
      int64_t  i64;
      uint32_t f32;
      uint64_t f64;
   };

   struct init_expr {
      int8_t     opcode;
      expr_value value;
   };

   struct global_type {
      value_type content_type;
      bool       mutability;
   };

   struct global_variable {
      global_type type;
      init_expr   init;
      init_expr   current;
   };

   struct table_type {
      elem_type                element_type;
      resizable_limits         limits;
      guarded_vector<uint32_t> table;
   };

   struct memory_type {
      resizable_limits limits;
   };

   union import_type {
      import_type() {}
      uint32_t    func_t;
      table_type  table_t;
      memory_type mem_t;
      global_type global_t;
   };

   struct import_entry {
      guarded_vector<uint8_t> module_str;
      guarded_vector<uint8_t> field_str;
      external_kind           kind;
      import_type             type;
   };

   struct export_entry {
      guarded_vector<uint8_t> field_str;
      external_kind           kind;
      uint32_t                index;
   };

   struct elem_segment {
      uint32_t                 index;
      init_expr                offset;
      guarded_vector<uint32_t> elems;
   };

   struct local_entry {
      uint32_t   count;
      value_type type;
   };

   struct function_body {
      uint32_t                    body_size;
      guarded_vector<local_entry> locals;
      guarded_vector<opcode>      code;
   };

   struct data_segment {
      uint32_t                index;
      init_expr               offset;
      guarded_vector<uint8_t> data;
   };

   using wasm_code     = std::vector<uint8_t>;
   using wasm_code_ptr = guarded_ptr<uint8_t>;

   struct module {
      growable_allocator              allocator = { constants::initial_module_size };
      uint32_t                        start     = std::numeric_limits<uint32_t>::max();
      guarded_vector<func_type>       types     = { allocator, 0 };
      guarded_vector<import_entry>    imports   = { allocator, 0 };
      guarded_vector<uint32_t>        functions = { allocator, 0 };
      guarded_vector<table_type>      tables    = { allocator, 0 };
      guarded_vector<memory_type>     memories  = { allocator, 0 };
      guarded_vector<global_variable> globals   = { allocator, 0 };
      guarded_vector<export_entry>    exports   = { allocator, 0 };
      guarded_vector<elem_segment>    elements  = { allocator, 0 };
      guarded_vector<function_body>   code      = { allocator, 0 };
      guarded_vector<data_segment>    data      = { allocator, 0 };

      // not part of the spec for WASM
      guarded_vector<uint32_t> import_functions = { allocator, 0 };
      guarded_vector<uint32_t> function_sizes   = { allocator, 0 };

      uint32_t get_imported_functions_size() const {
         uint32_t number_of_imports = 0;
         for (int i = 0; i < imports.size(); i++) {
            if (imports[i].kind == external_kind::Function)
               number_of_imports++;
         }
         return number_of_imports;
      }
      inline uint32_t get_functions_size() const { return code.size(); }
      inline uint32_t get_functions_total() const { return get_imported_functions_size() + get_functions_size(); }

      auto& get_function_type(uint32_t index) const {
         if (index < get_imported_functions_size())
            return types[imports[index].type.func_t];
         return types[functions[index - get_imported_functions_size()]];
      }

      uint32_t get_exported_function(const std::string_view str) {
         uint32_t index = std::numeric_limits<uint32_t>::max();
         for (int i = 0; i < exports.size(); i++) {
            if (exports[i].kind == external_kind::Function && exports[i].field_str.size() == str.size() &&
                memcmp((const char*)str.data(), (const char*)exports[i].field_str.raw(), exports[i].field_str.size()) ==
                      0) {
               index = exports[i].index;
               break;
            }
         }
         return index;
      }
   };

}} // namespace eosio::vm
