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
using backend_t = backend<std::nullptr_t>;

TEST_CASE( "Testing wasm <br_0_wasm>", "[br_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( br_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "type-i32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-i64"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f64"));
   CHECK(bkend.call_with_return(nullptr, "env", "type-i32-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "type-i64-value")->to_ui64() == UINT32_C(2));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "type-f32-value")->to_f32()) == UINT32_C(1077936128));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "type-f64-value")->to_f64()) == UINT64_C(4616189618054758400));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-mid"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-last"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-value")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first")->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid")->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last")->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value")->to_ui32() == UINT32_C(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-value")->to_ui32() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-value-cond")->to_ui32() == UINT32_C(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-value")->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-value-index")->to_ui32() == UINT32_C(11));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value")->to_ui64() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-cond")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(1), UINT32_C(6))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(0), UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(0), UINT32_C(6))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(1), UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(0), UINT32_C(6))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(1), UINT32_C(6))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-second", UINT32_C(0), UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-second", UINT32_C(1), UINT32_C(6))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-cond")->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-first")->to_ui32() == UINT32_C(12));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-mid")->to_ui32() == UINT32_C(13));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-last")->to_ui32() == UINT32_C(14));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-func")->to_ui32() == UINT32_C(20));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first")->to_ui32() == UINT32_C(21));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")->to_ui32() == UINT32_C(22));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-last")->to_ui32() == UINT32_C(23));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-value")->to_ui32() == UINT32_C(17));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-value")->to_ui32() == UINT32_C(1));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "as-load-address")->to_f32()) == UINT32_C(1071225242));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loadN-address")->to_ui64() == UINT32_C(30));
   CHECK(bkend.call_with_return(nullptr, "env", "as-store-address")->to_ui32() == UINT32_C(30));
   CHECK(bkend.call_with_return(nullptr, "env", "as-store-value")->to_ui32() == UINT32_C(31));
   CHECK(bkend.call_with_return(nullptr, "env", "as-storeN-address")->to_ui32() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "as-storeN-value")->to_ui32() == UINT32_C(33));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "as-unary-operand")->to_f32()) == UINT32_C(1079613850));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-left")->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-right")->to_ui64() == UINT32_C(45));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand")->to_ui32() == UINT32_C(44));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-left")->to_ui32() == UINT32_C(43));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-right")->to_ui32() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "as-convert-operand")->to_ui32() == UINT32_C(41));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-size")->to_ui32() == UINT32_C(40));
   CHECK(bkend.call_with_return(nullptr, "env", "nested-block-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "nested-br-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "nested-br_if-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "nested-br_table-value")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "nested-br_table-value-index")->to_ui32() == UINT32_C(9));
}

