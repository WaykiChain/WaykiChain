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

BACKEND_TEST_CASE( "Testing wasm <memory_redundancy_0_wasm>", "[memory_redundancy_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_redundancy.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "test_store_to_load")->to_ui32() == UINT32_C(128));
bkend(nullptr, "env", "zero_everything");
   CHECK(bkend.call_with_return(nullptr, "env", "test_redundant_load")->to_ui32() == UINT32_C(128));
bkend(nullptr, "env", "zero_everything");
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "test_dead_store")->to_f32()) == UINT32_C(35));
bkend(nullptr, "env", "zero_everything");
   CHECK(bkend.call_with_return(nullptr, "env", "malloc_aliasing")->to_ui32() == UINT32_C(43));
}

