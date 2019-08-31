#include <catch2/catch.hpp>
#include <eosio/vm/backend.hpp>
#include <wasm_config.hpp>

extern eosio::vm::wasm_allocator wa;

using backend_t = eosio::vm::backend<nullptr_t>;
using namespace eosio::vm;

TEST_CASE( "Testing wasm <br_0_wasm>", "[br_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( br_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "type-i32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-i64"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f64"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-i32-value")) == static_cast<uint32_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-i64-value")) == static_cast<uint64_t>(2));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32-value")) == static_cast<float>(3.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64-value")) == static_cast<double>(4.0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-mid"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-last"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-block-value")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-loop-first")) == static_cast<uint32_t>(3));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-loop-mid")) == static_cast<uint32_t>(4));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-loop-last")) == static_cast<uint32_t>(5));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == static_cast<uint32_t>(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-value")) == static_cast<uint32_t>(8));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-value-cond")) == static_cast<uint32_t>(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-value")) == static_cast<uint32_t>(10));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-value-index")) == static_cast<uint32_t>(11));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "as-return-value")) == static_cast<uint64_t>(7));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-cond")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-then", static_cast<uint32_t>(1), static_cast<uint32_t>(6))) == static_cast<uint32_t>(3));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-then", static_cast<uint32_t>(0), static_cast<uint32_t>(6))) == static_cast<uint32_t>(6));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-else", static_cast<uint32_t>(0), static_cast<uint32_t>(6))) == static_cast<uint32_t>(4));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-else", static_cast<uint32_t>(1), static_cast<uint32_t>(6))) == static_cast<uint32_t>(6));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-first", static_cast<uint32_t>(0), static_cast<uint32_t>(6))) == static_cast<uint32_t>(5));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-first", static_cast<uint32_t>(1), static_cast<uint32_t>(6))) == static_cast<uint32_t>(5));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-second", static_cast<uint32_t>(0), static_cast<uint32_t>(6))) == static_cast<uint32_t>(6));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-second", static_cast<uint32_t>(1), static_cast<uint32_t>(6))) == static_cast<uint32_t>(6));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-cond")) == static_cast<uint32_t>(7));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call-first")) == static_cast<uint32_t>(12));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call-mid")) == static_cast<uint32_t>(13));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call-last")) == static_cast<uint32_t>(14));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-func")) == static_cast<uint32_t>(20));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == static_cast<uint32_t>(21));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == static_cast<uint32_t>(22));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == static_cast<uint32_t>(23));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-local.set-value")) == static_cast<uint32_t>(17));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-local.tee-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-global.set-value")) == static_cast<uint32_t>(1));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-load-address")) == static_cast<float>(1.7f));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "as-loadN-address")) == static_cast<uint64_t>(30));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-store-address")) == static_cast<uint32_t>(30));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-store-value")) == static_cast<uint32_t>(31));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-storeN-address")) == static_cast<uint32_t>(32));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-storeN-value")) == static_cast<uint32_t>(33));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == static_cast<float>(3.4f));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == static_cast<uint32_t>(3));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == static_cast<uint64_t>(45));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == static_cast<uint32_t>(43));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == static_cast<uint32_t>(42));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-convert-operand")) == static_cast<uint32_t>(41));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-size")) == static_cast<uint32_t>(40));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested-block-value")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested-br-value")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index")) == static_cast<uint32_t>(9));
}

