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

TEST_CASE( "Testing wasm <block_0_wasm>", "[block_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( block_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "empty"));
   CHECK(bkend.call_with_return(nullptr, "env", "singular")->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "multi")->to_ui32() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "nested")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "deep")->to_ui32() == UINT32_C(150));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-last")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-operand")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-operand")->to_ui32() == UINT32_C(12));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "break-bare")->to_ui32() == UINT32_C(19));
   CHECK(bkend.call_with_return(nullptr, "env", "break-value")->to_ui32() == UINT32_C(18));
   CHECK(bkend.call_with_return(nullptr, "env", "break-repeated")->to_ui32() == UINT32_C(18));
   CHECK(bkend.call_with_return(nullptr, "env", "break-inner")->to_ui32() == UINT32_C(15));
   CHECK(bkend.call_with_return(nullptr, "env", "effects")->to_ui32() == UINT32_C(1));
}

