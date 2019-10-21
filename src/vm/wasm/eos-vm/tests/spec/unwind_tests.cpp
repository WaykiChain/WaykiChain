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

BACKEND_TEST_CASE( "Testing wasm <unwind_0_wasm>", "[unwind_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "unwind.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "func-unwind-by-unreachable"), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "func-unwind-by-br"));
   CHECK(bkend.call_with_return(nullptr, "env", "func-unwind-by-br-value")->to_ui32() == UINT32_C(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "func-unwind-by-br_if"));
   CHECK(bkend.call_with_return(nullptr, "env", "func-unwind-by-br_if-value")->to_ui32() == UINT32_C(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "func-unwind-by-br_table"));
   CHECK(bkend.call_with_return(nullptr, "env", "func-unwind-by-br_table-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "func-unwind-by-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "block-unwind-by-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-br-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-br_if-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-br_table-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-unwind-by-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "block-nested-unwind-by-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_if-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_table-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "unary-after-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "unary-after-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "unary-after-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "unary-after-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "unary-after-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "binary-after-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "binary-after-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "binary-after-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "binary-after-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "binary-after-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "select-after-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "select-after-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "select-after-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "select-after-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "select-after-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "block-value-after-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "block-value-after-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-value-after-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-value-after-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "block-value-after-return")->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "loop-value-after-unreachable"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "loop-value-after-br")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "loop-value-after-br_if")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "loop-value-after-br_table")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "loop-value-after-return")->to_ui32() == UINT32_C(9));
}

