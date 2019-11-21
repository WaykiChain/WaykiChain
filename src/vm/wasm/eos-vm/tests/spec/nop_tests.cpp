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

BACKEND_TEST_CASE( "Testing wasm <nop_0_wasm>", "[nop_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "nop.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "as-func-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-func-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-func-last")->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-func-everywhere")->to_ui32() == UINT32_C(4));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-first", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-last", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-everywhere", UINT32_C(0)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(3))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid1", UINT32_C(3))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid2", UINT32_C(3))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last", UINT32_C(3))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-everywhere", UINT32_C(3))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-first")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-last")->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-everywhere")->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last")->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-everywhere")->to_ui32() == UINT32_C(4));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition", UINT32_C(4294967295)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(4)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(3)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-first", UINT32_C(5))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-last", UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-everywhere", UINT32_C(7))->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first", UINT32_C(4))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-mid", UINT32_C(5))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last", UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-everywhere", UINT32_C(7))->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-first", UINT32_C(4))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-mid", UINT32_C(5))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last", UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-everywhere", UINT32_C(7))->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-first", UINT32_C(5))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-last", UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-everywhere", UINT32_C(7))->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-first", UINT32_C(3), UINT32_C(1), UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-mid1", UINT32_C(3), UINT32_C(1), UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-mid2", UINT32_C(0), UINT32_C(3), UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-last", UINT32_C(10), UINT32_C(9), UINT32_C(4294967295))->to_ui32() == UINT32_C(20));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-everywhere", UINT32_C(2), UINT32_C(1), UINT32_C(5))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-first", UINT32_C(30))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-last", UINT32_C(30))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-everywhere", UINT32_C(12))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-first", UINT32_C(3))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-mid", UINT32_C(3))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-last", UINT32_C(3))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-everywhere", UINT32_C(3))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-first", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-last", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-everywhere", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-first", UINT32_C(3))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-mid", UINT32_C(3))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-last", UINT32_C(3))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-everywhere", UINT32_C(3))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-first", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-last", UINT32_C(2))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-everywhere", UINT32_C(12))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid1")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid2")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-last")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-everywhere")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-first", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-last", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-everywhere", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-first", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-last", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-everywhere", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-first")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-everywhere")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-first", UINT32_C(100))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-last", UINT32_C(100))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-everywhere", UINT32_C(100))->to_ui32() == UINT32_C(0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first", UINT32_C(0), UINT32_C(1)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-mid", UINT32_C(0), UINT32_C(2)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last", UINT32_C(0), UINT32_C(3)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-everywhere", UINT32_C(0), UINT32_C(4)));
}

BACKEND_TEST_CASE( "Testing wasm <nop_1_wasm>", "[nop_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "nop.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <nop_2_wasm>", "[nop_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "nop.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <nop_3_wasm>", "[nop_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "nop.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <nop_4_wasm>", "[nop_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "nop.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

