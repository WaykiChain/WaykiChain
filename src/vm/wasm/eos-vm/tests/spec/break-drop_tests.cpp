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

TEST_CASE( "Testing wasm <break-drop_0_wasm>", "[break-drop_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( break_drop_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "br"));
   CHECK(!bkend.call_with_return(nullptr, "env", "br_if"));
   CHECK(!bkend.call_with_return(nullptr, "env", "br_table"));
}

