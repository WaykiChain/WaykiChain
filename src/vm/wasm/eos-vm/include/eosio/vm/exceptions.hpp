#pragma once

#include <eosio/vm/utils.hpp>

#include <exception>
#include <string>

#define EOS_WB_ASSERT( expr, exc_type, msg ) \
   if (!UNLIKELY(expr)) {                    \
      throw exc_type{msg};                   \
   }

namespace eosio { namespace vm {
   struct exception : public std::exception {
      virtual const char* what()const throw()=0;
      virtual const char* detail()const throw()=0;
   };
}}

#define DECLARE_EXCEPTION(name, _code, _what)                                     \
   struct name : public eosio::vm::exception {                          \
      name(const char* msg) : msg(msg) {}                                         \
      virtual const char* what()const throw() { return _what; }                   \
      virtual const char* detail()const throw() { return msg; }                   \
      uint32_t code()const { return _code; }                                      \
      const char* msg;                                                            \
   };

namespace eosio { namespace vm {
   DECLARE_EXCEPTION( wasm_interpreter_exception,        4000000, "wasm interpreter exception" )
   DECLARE_EXCEPTION( wasm_section_length_exception,     4000001, "wasm section length exception" )
   DECLARE_EXCEPTION( wasm_bad_alloc,                    4000002, "wasm allocation failed" )
   DECLARE_EXCEPTION( wasm_double_free,                  4000003, "wasm free failed" )
   DECLARE_EXCEPTION( wasm_vector_oob_exception,         4000004, "wasm vector out of bounds" )
   DECLARE_EXCEPTION( wasm_unsupported_import_exception, 4000005, "wasm interpreter only accepts function imports" )
   DECLARE_EXCEPTION( wasm_parse_exception,              4000006, "wasm parse exception" )
   DECLARE_EXCEPTION( wasm_memory_exception,             4000007, "wasm memory exception" )
   DECLARE_EXCEPTION( stack_memory_exception,            4000008, "stack memory exception" )
   DECLARE_EXCEPTION( wasm_invalid_element,              4000009, "wasm invalid_element" )
   DECLARE_EXCEPTION( wasm_link_exception,               4000010, "wasm linked function failure" )
   DECLARE_EXCEPTION( guarded_ptr_exception,             4010000, "pointer out of bounds" )
}} // eosio::vm
