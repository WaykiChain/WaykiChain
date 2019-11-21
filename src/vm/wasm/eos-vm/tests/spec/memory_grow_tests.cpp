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

BACKEND_TEST_CASE( "Testing wasm <memory_grow_0_wasm>", "[memory_grow_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_grow.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "size")->to_ui32() == UINT32_C(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "store_at_zero"), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load_at_zero"), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "store_at_page_size"), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load_at_page_size"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "size")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "load_at_zero")->to_ui32() == UINT32_C(0));
   CHECK(!bkend.call_with_return(nullptr, "env", "store_at_zero"));
   CHECK(bkend.call_with_return(nullptr, "env", "load_at_zero")->to_ui32() == UINT32_C(2));
   //CHECK_THROWS_AS(bkend(nullptr, "env", "store_at_page_size"), std::exception);
   //CHECK_THROWS_AS(bkend(nullptr, "env", "load_at_page_size"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(4))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "size")->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "load_at_zero")->to_ui32() == UINT32_C(2));
   CHECK(!bkend.call_with_return(nullptr, "env", "store_at_zero"));
   CHECK(bkend.call_with_return(nullptr, "env", "load_at_zero")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "load_at_page_size")->to_ui32() == UINT32_C(0));
   CHECK(!bkend.call_with_return(nullptr, "env", "store_at_page_size"));
   CHECK(bkend.call_with_return(nullptr, "env", "load_at_page_size")->to_ui32() == UINT32_C(3));
}

BACKEND_TEST_CASE( "Testing wasm <memory_grow_1_wasm>", "[memory_grow_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_grow.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(2))->to_ui32() == UINT32_C(1));
   //CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(800))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(65536))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(64736))->to_ui32() == UINT32_C(4294967295));
   //CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(803));
}

BACKEND_TEST_CASE( "Testing wasm <memory_grow_2_wasm>", "[memory_grow_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_grow.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(6))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(65536))->to_ui32() == UINT32_C(4294967295));
}

BACKEND_TEST_CASE( "Testing wasm <memory_grow_3_wasm>", "[memory_grow_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_grow.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "check-memory-zero", UINT32_C(0), UINT32_C(65535))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "check-memory-zero", UINT32_C(65536), UINT32_C(131071))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "check-memory-zero", UINT32_C(131072), UINT32_C(196607))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "check-memory-zero", UINT32_C(196608), UINT32_C(262143))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "check-memory-zero", UINT32_C(262144), UINT32_C(327679))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "grow", UINT32_C(1))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "check-memory-zero", UINT32_C(327680), UINT32_C(393215))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <memory_grow_4_wasm>", "[memory_grow_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_grow.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-value-cond")->to_ui32() == UINT32_C(6));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-value-index")->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-cond")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-second", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-cond")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-first")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-mid")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-last")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-last")->to_ui32() == UINT32_C(4294967295));
   //CHECK_THROWS_AS(bkend(nullptr, "env", "as-call_indirect-index"), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-local.set-value"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-global.set-value"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-address")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loadN-address")->to_ui32() == UINT32_C(0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-address"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-value"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-storeN-address"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-storeN-value"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-operand")->to_ui32() == UINT32_C(31));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-left")->to_ui32() == UINT32_C(11));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-right")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-left")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-right")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-size")->to_ui32() == UINT32_C(1));
}

