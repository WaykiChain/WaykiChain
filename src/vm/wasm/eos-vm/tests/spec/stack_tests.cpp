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

BACKEND_TEST_CASE( "Testing wasm <stack_0_wasm>", "[stack_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "stack.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "fac-expr", UINT64_C(25))->to_ui64() == UINT32_C(7034535277573963776));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-stack", UINT64_C(25))->to_ui64() == UINT32_C(7034535277573963776));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-mixed", UINT64_C(25))->to_ui64() == UINT32_C(7034535277573963776));
}

BACKEND_TEST_CASE( "Testing wasm <stack_1_wasm>", "[stack_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "stack.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

