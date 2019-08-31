#include <catch2/catch.hpp>
#include <eosio/vm/backend.hpp>
#include <wasm_config.hpp>

extern eosio::vm::wasm_allocator wa;

using backend_t = eosio::vm::backend<nullptr_t>;
using namespace eosio::vm;

TEST_CASE( "Testing wasm <break_drop_0_wasm>", "[break_drop_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( break_drop_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "br"));
   CHECK(!bkend.call_with_return(nullptr, "env", "br_if"));
   CHECK(!bkend.call_with_return(nullptr, "env", "br_table"));
}

