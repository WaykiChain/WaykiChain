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

#define ONLY_FUNCTION_IMPORTS 0

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;
using backend_t = backend<std::nullptr_t>;

TEST_CASE( "Testing wasm <elem_0_wasm>", "[elem_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

#if ONLY_FUNCTION_IMPORTS
TEST_CASE( "Testing wasm <elem_10_wasm>", "[elem_10_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_10_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_11_wasm>", "[elem_11_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_11_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}
#endif

TEST_CASE( "Testing wasm <elem_12_wasm>", "[elem_12_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_12_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

#if ONLY_FUNCTION_IMPORTS
TEST_CASE( "Testing wasm <elem_10_wasm>", "[elem_10_wasm_tests]" ) {
TEST_CASE( "Testing wasm <elem_13_wasm>", "[elem_13_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_13_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_14_wasm>", "[elem_14_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_14_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_15_wasm>", "[elem_15_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_15_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_16_wasm>", "[elem_16_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_16_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_17_wasm>", "[elem_17_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_17_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_1_wasm>", "[elem_1_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_1_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_2_wasm>", "[elem_2_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_2_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}
#endif

TEST_CASE( "Testing wasm <elem_36_wasm>", "[elem_36_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_36_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "call-overwritten")->to_ui32() == UINT32_C(66));
}

#if ONLY_FUNCTION_IMPORTS
TEST_CASE( "Testing wasm <elem_37_wasm>", "[elem_37_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_37_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "call-overwritten-element")->to_ui32() == UINT32_C(66));
}

TEST_CASE( "Testing wasm <elem_38_wasm>", "[elem_38_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_38_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "call-7"), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "call-8")->to_ui32() == UINT32_C(65));
   CHECK(bkend.call_with_return(nullptr, "env", "call-9")->to_ui32() == UINT32_C(66));
}

TEST_CASE( "Testing wasm <elem_39_wasm>", "[elem_39_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_39_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "call-7")->to_ui32() == UINT32_C(67));
   CHECK(bkend.call_with_return(nullptr, "env", "call-8")->to_ui32() == UINT32_C(68));
   CHECK(bkend.call_with_return(nullptr, "env", "call-9")->to_ui32() == UINT32_C(66));
}

TEST_CASE( "Testing wasm <elem_3_wasm>", "[elem_3_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_3_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_40_wasm>", "[elem_40_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_40_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "call-7")->to_ui32() == UINT32_C(67));
   CHECK(bkend.call_with_return(nullptr, "env", "call-8")->to_ui32() == UINT32_C(69));
   CHECK(bkend.call_with_return(nullptr, "env", "call-9")->to_ui32() == UINT32_C(70));
}

TEST_CASE( "Testing wasm <elem_4_wasm>", "[elem_4_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_4_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_5_wasm>", "[elem_5_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_5_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <elem_6_wasm>", "[elem_6_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_6_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}
#endif

TEST_CASE( "Testing wasm <elem_7_wasm>", "[elem_7_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_7_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "call-7")->to_ui32() == UINT32_C(65));
   CHECK(bkend.call_with_return(nullptr, "env", "call-9")->to_ui32() == UINT32_C(66));
}

TEST_CASE( "Testing wasm <elem_8_wasm>", "[elem_8_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_8_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

#if ONLY_FUNCTION_IMPORTS
TEST_CASE( "Testing wasm <elem_9_wasm>", "[elem_9_wasm_tests]" ) {
   auto code = backend_t::read_wasm( elem_9_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}
#endif

