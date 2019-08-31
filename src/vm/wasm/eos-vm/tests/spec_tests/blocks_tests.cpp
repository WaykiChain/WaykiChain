#include <catch2/catch.hpp>
#include <eosio/vm/backend.hpp>
#include <wasm_config.hpp>

extern eosio::vm::wasm_allocator wa;

using backend_t = eosio::vm::backend<nullptr_t>;
using namespace eosio::vm;

TEST_CASE( "Testing wasm <blocks_0_wasm>", "[blocks_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( blocks_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "empty"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "singular")) == static_cast<uint32_t>(7));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "multi")) == static_cast<uint32_t>(8));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "deep")) == static_cast<uint32_t>(150));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-loop-first")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-loop-mid")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-loop-last")) == static_cast<uint32_t>(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-then")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-else")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == static_cast<uint32_t>(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == static_cast<uint32_t>(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-local.set-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-local.tee-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-global.set-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-binary-operand")) == static_cast<uint32_t>(12));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-operand")) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "break-bare")) == static_cast<uint32_t>(19));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "break-value")) == static_cast<uint32_t>(18));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "break-repeated")) == static_cast<uint32_t>(18));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "break-inner")) == static_cast<uint32_t>(15));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "effects")) == static_cast<uint32_t>(1));
}

