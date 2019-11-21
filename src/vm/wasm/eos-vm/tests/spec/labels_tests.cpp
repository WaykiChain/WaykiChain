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

BACKEND_TEST_CASE( "Testing wasm <labels_0_wasm>", "[labels_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "labels.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "block")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "loop1")->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "loop2")->to_ui32() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "loop3")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "loop4", UINT32_C(8))->to_ui32() == UINT32_C(16));
   CHECK(bkend.call_with_return(nullptr, "env", "loop5")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "loop6")->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "if")->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "if2")->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "switch", UINT32_C(0))->to_ui32() == UINT32_C(50));
   CHECK(bkend.call_with_return(nullptr, "env", "switch", UINT32_C(1))->to_ui32() == UINT32_C(20));
   CHECK(bkend.call_with_return(nullptr, "env", "switch", UINT32_C(2))->to_ui32() == UINT32_C(20));
   CHECK(bkend.call_with_return(nullptr, "env", "switch", UINT32_C(3))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "switch", UINT32_C(4))->to_ui32() == UINT32_C(50));
   CHECK(bkend.call_with_return(nullptr, "env", "switch", UINT32_C(5))->to_ui32() == UINT32_C(50));
   CHECK(bkend.call_with_return(nullptr, "env", "return", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "return", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "return", UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "br_if0")->to_ui32() == UINT32_C(29));
   CHECK(bkend.call_with_return(nullptr, "env", "br_if1")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "br_if2")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "br_if3")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "br")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shadowing")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "redefinition")->to_ui32() == UINT32_C(5));
}

BACKEND_TEST_CASE( "Testing wasm <labels_1_wasm>", "[labels_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "labels.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <labels_2_wasm>", "[labels_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "labels.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <labels_3_wasm>", "[labels_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "labels.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

