#include <iostream>
#include <vector>

#include <eosio/vm/backend.hpp>
#include <eosio/vm/watchdog.hpp>

#include "utils.hpp"
#include <catch2/catch.hpp>

using namespace eosio;
using namespace eosio::vm;

extern wasm_allocator wa;

BACKEND_TEST_CASE("Test that we can load and run a wasm that uses 8191 stack elements",
                  "[8k_minus_1_stack_elems_test]") {
   // comes from the wasm
   /*
   (module
     (type (;0;) (func))
     (type (;1;) (func (param i32)))
     (func (;0;) (type 1) (param i32)
       (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64) get_local 0 i32.const 249 i32.eq br_if 0 (;@0;) get_local 0 i32.const 1 i32.add call 0) (func
   (;1;) (type 0) (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64) i32.const 0 call 0) (memory
   (;0;) 0) (export "fun" (func 1)))
   */
   std::vector<uint8_t> _8k_stack_size_1_under_wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00,
      0x03, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x00, 0x07, 0x07, 0x01, 0x03, 0x66, 0x75, 0x6e, 0x00,
      0x01, 0x0a, 0x1f, 0x02, 0x13, 0x01, 0x1f, 0x7e, 0x20, 0x00, 0x41, 0xf9, 0x01, 0x46, 0x0d, 0x00, 0x20, 0x00,
      0x41, 0x01, 0x6a, 0x10, 0x00, 0x0b, 0x09, 0x01, 0xbe, 0x01, 0x7e, 0x41, 0x00, 0x10, 0x00, 0x0b
   };

   using backend_t = backend<nullptr_t>;
   backend_t bkend(_8k_stack_size_1_under_wasm);
   bkend.set_wasm_allocator(&wa);
   bkend.initialize();
   bkend.execute_all(null_watchdog());
}

BACKEND_TEST_CASE("Test that we can load and run a wasm that uses 8192 stack elements", "[8k_stack_elems_test]") {
   // comes from the wasm
   /*
   (module
     (type (;0;) (func))
     (type (;1;) (func (param i32)))
     (func (;0;) (type 1) (param i32)
       (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64) get_local 0 i32.const 249 i32.eq br_if 0 (;@0;) get_local 0 i32.const 1 i32.add call 0) (func
   (;1;) (type 0) (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64
   i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64) i32.const 0 call 0) (memory
   (;0;) 0) (export "fun" (func 1)))
   */

   std::vector<uint8_t> _8k_stack_size_wasm = { 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
                                                 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x03, 0x03, 0x02, 0x01, 0x00, 0x05,
                                                 0x03, 0x01, 0x00, 0x00, 0x07, 0x07, 0x01, 0x03, 0x66, 0x75, 0x6e, 0x00,
                                                 0x01, 0x0a, 0x1f, 0x02, 0x13, 0x01, 0x9d, 0x03, 0x7e, 0x20, 0x00, 0x41,
                                                 0x14, 0x46, 0x0d, 0x00, 0x20, 0x00, 0x41, 0x01, 0x6a, 0x10, 0x00, 0x0b,
                                                 0x09, 0x01, 0xfe, 0x02, 0x7e, 0x41, 0x00, 0x10, 0x00, 0x0b };

   using backend_t = backend<nullptr_t>;
   backend_t bkend(_8k_stack_size_wasm);
   bkend.set_wasm_allocator(&wa);
   bkend.initialize();
   bkend.execute_all(null_watchdog());
}
