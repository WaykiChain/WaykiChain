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

BACKEND_TEST_CASE( "Testing wasm <address_0_wasm>", "[address_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "address.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "8u_good1", UINT32_C(0))->to_ui32() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good2", UINT32_C(0))->to_ui32() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good3", UINT32_C(0))->to_ui32() == UINT32_C(98));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good4", UINT32_C(0))->to_ui32() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good5", UINT32_C(0))->to_ui32() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good1", UINT32_C(0))->to_ui32() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good2", UINT32_C(0))->to_ui32() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good3", UINT32_C(0))->to_ui32() == UINT32_C(98));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good4", UINT32_C(0))->to_ui32() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good5", UINT32_C(0))->to_ui32() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good1", UINT32_C(0))->to_ui32() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good2", UINT32_C(0))->to_ui32() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good3", UINT32_C(0))->to_ui32() == UINT32_C(25442));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good4", UINT32_C(0))->to_ui32() == UINT32_C(25699));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good5", UINT32_C(0))->to_ui32() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good1", UINT32_C(0))->to_ui32() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good2", UINT32_C(0))->to_ui32() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good3", UINT32_C(0))->to_ui32() == UINT32_C(25442));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good4", UINT32_C(0))->to_ui32() == UINT32_C(25699));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good5", UINT32_C(0))->to_ui32() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good1", UINT32_C(0))->to_ui32() == UINT32_C(1684234849));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good2", UINT32_C(0))->to_ui32() == UINT32_C(1684234849));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good3", UINT32_C(0))->to_ui32() == UINT32_C(1701077858));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good4", UINT32_C(0))->to_ui32() == UINT32_C(1717920867));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good5", UINT32_C(0))->to_ui32() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good1", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good2", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good3", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good4", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good5", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good1", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good2", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good3", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good4", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good5", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good1", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good2", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good3", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good4", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good5", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good1", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good2", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good3", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good4", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good5", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good1", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good2", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good3", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good4", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good5", UINT32_C(65507))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good1", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good2", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good3", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good4", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good5", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good1", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good2", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good3", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good4", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good5", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good1", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good2", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good3", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good4", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good5", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good1", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good2", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good3", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good4", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good5", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good1", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good2", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good3", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32_good4", UINT32_C(65508))->to_ui32() == UINT32_C(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_good5", UINT32_C(65508)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", UINT32_C(1)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <address_2_wasm>", "[address_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "address.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "8u_good1", UINT32_C(0))->to_ui64() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good2", UINT32_C(0))->to_ui64() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good3", UINT32_C(0))->to_ui64() == UINT32_C(98));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good4", UINT32_C(0))->to_ui64() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good1", UINT32_C(0))->to_ui64() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good2", UINT32_C(0))->to_ui64() == UINT32_C(97));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good3", UINT32_C(0))->to_ui64() == UINT32_C(98));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good4", UINT32_C(0))->to_ui64() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good1", UINT32_C(0))->to_ui64() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good2", UINT32_C(0))->to_ui64() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good3", UINT32_C(0))->to_ui64() == UINT32_C(25442));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good4", UINT32_C(0))->to_ui64() == UINT32_C(25699));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good1", UINT32_C(0))->to_ui64() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good2", UINT32_C(0))->to_ui64() == UINT32_C(25185));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good3", UINT32_C(0))->to_ui64() == UINT32_C(25442));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good4", UINT32_C(0))->to_ui64() == UINT32_C(25699));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good1", UINT32_C(0))->to_ui64() == UINT32_C(1684234849));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good2", UINT32_C(0))->to_ui64() == UINT32_C(1684234849));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good3", UINT32_C(0))->to_ui64() == UINT32_C(1701077858));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good4", UINT32_C(0))->to_ui64() == UINT32_C(1717920867));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good1", UINT32_C(0))->to_ui64() == UINT32_C(1684234849));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good2", UINT32_C(0))->to_ui64() == UINT32_C(1684234849));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good3", UINT32_C(0))->to_ui64() == UINT32_C(1701077858));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good4", UINT32_C(0))->to_ui64() == UINT32_C(1717920867));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good1", UINT32_C(0))->to_ui64() == UINT32_C(7523094288207667809));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good2", UINT32_C(0))->to_ui64() == UINT32_C(7523094288207667809));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good3", UINT32_C(0))->to_ui64() == UINT32_C(7595434461045744482));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good4", UINT32_C(0))->to_ui64() == UINT32_C(7667774633883821155));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good5", UINT32_C(0))->to_ui64() == UINT32_C(122));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good1", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good2", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good3", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good4", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good5", UINT32_C(65503))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8u_good5", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "8s_good5", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16u_good5", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "16s_good5", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32u_good5", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "32s_good5", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good1", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good2", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good3", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "64_good4", UINT32_C(65504))->to_ui64() == UINT32_C(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_good5", UINT32_C(65504)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32u_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32s_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32u_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32s_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", UINT32_C(1)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <address_3_wasm>", "[address_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "address.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good1", UINT32_C(0))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good2", UINT32_C(0))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good3", UINT32_C(0))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good4", UINT32_C(0))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good5", UINT32_C(0))->to_f32()) == UINT32_C(2144337921));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good1", UINT32_C(65524))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good2", UINT32_C(65524))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good3", UINT32_C(65524))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good4", UINT32_C(65524))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good5", UINT32_C(65524))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good1", UINT32_C(65525))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good2", UINT32_C(65525))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good3", UINT32_C(65525))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "32_good4", UINT32_C(65525))->to_f32()) == UINT32_C(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_good5", UINT32_C(65525)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", UINT32_C(1)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <address_4_wasm>", "[address_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "address.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good1", UINT32_C(0))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good2", UINT32_C(0))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good3", UINT32_C(0))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good4", UINT32_C(0))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good5", UINT32_C(0))->to_f64()) == UINT64_C(9222246136947933185));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good1", UINT32_C(65510))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good2", UINT32_C(65510))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good3", UINT32_C(65510))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good4", UINT32_C(65510))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good5", UINT32_C(65510))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good1", UINT32_C(65511))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good2", UINT32_C(65511))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good3", UINT32_C(65511))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "64_good4", UINT32_C(65511))->to_f64()) == UINT64_C(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_good5", UINT32_C(65511)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", UINT32_C(1)), std::exception);
}

