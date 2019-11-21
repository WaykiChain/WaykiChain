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

BACKEND_TEST_CASE( "Testing wasm <switch_0_wasm>", "[switch_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "switch.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(1))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(2))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(3))->to_ui32() == UINT32_C(4294967293));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(4))->to_ui32() == UINT32_C(100));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(5))->to_ui32() == UINT32_C(101));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(6))->to_ui32() == UINT32_C(102));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(7))->to_ui32() == UINT32_C(100));
   CHECK(bkend.call_with_return(nullptr, "env", "stmt", UINT32_C(4294967286))->to_ui32() == UINT32_C(102));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(1))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(2))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(3))->to_ui64() == UINT32_C(18446744073709551613));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(6))->to_ui64() == UINT32_C(101));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(7))->to_ui64() == UINT32_C(18446744073709551611));
   CHECK(bkend.call_with_return(nullptr, "env", "expr", UINT64_C(18446744073709551606))->to_ui64() == UINT32_C(100));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(0))->to_ui32() == UINT32_C(110));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(1))->to_ui32() == UINT32_C(12));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(2))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(3))->to_ui32() == UINT32_C(1116));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(4))->to_ui32() == UINT32_C(118));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(5))->to_ui32() == UINT32_C(20));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(6))->to_ui32() == UINT32_C(12));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(7))->to_ui32() == UINT32_C(1124));
   CHECK(bkend.call_with_return(nullptr, "env", "arg", UINT32_C(8))->to_ui32() == UINT32_C(126));
   CHECK(bkend.call_with_return(nullptr, "env", "corner")->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <switch_1_wasm>", "[switch_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "switch.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

