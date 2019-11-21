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

BACKEND_TEST_CASE( "Testing wasm <start_3_wasm>", "[start_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "start.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "get")->to_ui32() == UINT32_C(68));
bkend(nullptr, "env", "inc");
   CHECK(bkend.call_with_return(nullptr, "env", "get")->to_ui32() == UINT32_C(69));
bkend(nullptr, "env", "inc");
   CHECK(bkend.call_with_return(nullptr, "env", "get")->to_ui32() == UINT32_C(70));
}

BACKEND_TEST_CASE( "Testing wasm <start_4_wasm>", "[start_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "start.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "get")->to_ui32() == UINT32_C(68));
bkend(nullptr, "env", "inc");
   CHECK(bkend.call_with_return(nullptr, "env", "get")->to_ui32() == UINT32_C(69));
bkend(nullptr, "env", "inc");
   CHECK(bkend.call_with_return(nullptr, "env", "get")->to_ui32() == UINT32_C(70));
}

/*
TEST_CASE( "Testing wasm <start_5_wasm>", "[start_5_wasm_tests]" ) {
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "start.5.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <start_6_wasm>", "[start_6_wasm_tests]" ) {
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "start.6.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <start_7_wasm>", "[start_7_wasm_tests]" ) {
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "start.7.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}
*/
