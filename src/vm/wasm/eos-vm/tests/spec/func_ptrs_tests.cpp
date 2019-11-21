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

BACKEND_TEST_CASE( "Testing wasm <func_ptrs_0_wasm>", "[func_ptrs_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "func_ptrs.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "one")->to_ui32() == UINT32_C(13));
   CHECK(bkend.call_with_return(nullptr, "env", "two", UINT32_C(13))->to_ui32() == UINT32_C(14));
   CHECK(bkend.call_with_return(nullptr, "env", "three", UINT32_C(13))->to_ui32() == UINT32_C(11));
#if 0
bkend(nullptr, "env", "four", UINT32_C(83));
#endif
}

BACKEND_TEST_CASE( "Testing wasm <func_ptrs_8_wasm>", "[func_ptrs_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "func_ptrs.8.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(2))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(3))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(4))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(5))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(6))->to_ui32() == UINT32_C(3));
   CHECK_THROWS_AS(bkend(nullptr, "env", "callt", UINT32_C(7)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "callt", UINT32_C(100)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "callt", UINT32_C(4294967295)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(2))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(3))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(4))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(5))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "callu", UINT32_C(6))->to_ui32() == UINT32_C(3));
   CHECK_THROWS_AS(bkend(nullptr, "env", "callu", UINT32_C(7)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "callu", UINT32_C(100)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "callu", UINT32_C(4294967295)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <func_ptrs_9_wasm>", "[func_ptrs_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "func_ptrs.9.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "callt", UINT32_C(1))->to_ui32() == UINT32_C(2));
}

