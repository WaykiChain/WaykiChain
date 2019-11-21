#include <algorithm>
#include <vector>
#include <iostream>
#include <iterator>
#include <cmath>
#include <cstdlib>
#include <catch2/catch.hpp>
#include <utils.hpp>
#include <wasm_config.hpp>
#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;

BACKEND_TEST_CASE( "Testing wasm <globals_0_wasm>", "[globals_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "globals.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "get-a")->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "get-b")->to_ui64() == UINT32_C(18446744073709551611));
   CHECK(bkend.call_with_return(nullptr, "env", "get-x")->to_ui32() == UINT32_C(4294967284));
   CHECK(bkend.call_with_return(nullptr, "env", "get-y")->to_ui64() == UINT32_C(18446744073709551601));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "get-1")->to_f32()) == UINT32_C(3225419776));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "get-2")->to_f64()) == UINT64_C(13839561654909534208));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "get-5")->to_f32()) == UINT32_C(3243245568));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "get-6")->to_f64()) == UINT64_C(13847442954257432576));
   CHECK(!bkend.call_with_return(nullptr, "env", "set-x", UINT32_C(6)));
   CHECK(!bkend.call_with_return(nullptr, "env", "set-y", UINT64_C(7)));
   CHECK(!bkend.call_with_return(nullptr, "env", "set-5", bit_cast<float>(UINT32_C(1090519040))));
   CHECK(!bkend.call_with_return(nullptr, "env", "set-6", bit_cast<double>(UINT64_C(4621256167635550208))));
   CHECK(bkend.call_with_return(nullptr, "env", "get-x")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "get-y")->to_ui64() == UINT32_C(7));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "get-5")->to_f32()) == UINT32_C(1090519040));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "get-6")->to_f64()) == UINT64_C(4621256167635550208));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-condition")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-first")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")->to_ui32() == UINT32_C(2));
   //breaks subsequent tests
   //CHECK_THROWS_AS(bkend(nullptr, "env", "as-call_indirect-last"), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-operand")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-value")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value")->to_ui32() == UINT32_C(6));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-value", UINT32_C(1))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value", UINT32_C(1))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-value")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-operand")->to_ui32() == UINT32_C(36));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-operand")->to_ui32() == UINT32_C(1));
}

/*
TEST_CASE( "Testing wasm <globals_14_wasm>", "[globals_14_wasm_tests]" ) {
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "globals.14.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}
*/

BACKEND_TEST_CASE( "Testing wasm <globals_17_wasm>", "[globals_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "globals.17.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <globals_2_wasm>", "[globals_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "globals.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <globals_3_wasm>", "[globals_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "globals.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

