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

TEST_CASE( "Testing wasm <align_0_wasm>", "[align_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_106_wasm>", "[align_106_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_106_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_align_switch", UINT32_C(0))->to_f32()) == UINT32_C(1092616192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_align_switch", UINT32_C(1))->to_f32()) == UINT32_C(1092616192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_align_switch", UINT32_C(2))->to_f32()) == UINT32_C(1092616192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_align_switch", UINT32_C(3))->to_f32()) == UINT32_C(1092616192));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_align_switch", UINT32_C(0))->to_f64()) == UINT64_C(4621819117588971520));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_align_switch", UINT32_C(1))->to_f64()) == UINT64_C(4621819117588971520));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_align_switch", UINT32_C(2))->to_f64()) == UINT64_C(4621819117588971520));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_align_switch", UINT32_C(3))->to_f64()) == UINT64_C(4621819117588971520));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_align_switch", UINT32_C(4))->to_f64()) == UINT64_C(4621819117588971520));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(2), UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(2), UINT32_C(1))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(2), UINT32_C(2))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(3), UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(3), UINT32_C(1))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(3), UINT32_C(2))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(4), UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(4), UINT32_C(1))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(4), UINT32_C(2))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_align_switch", UINT32_C(4), UINT32_C(4))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(0), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(0), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(1), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(1), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(2), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(2), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(2), UINT32_C(2))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(3), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(3), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(3), UINT32_C(2))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(4), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(4), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(4), UINT32_C(2))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(4), UINT32_C(4))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(5), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(5), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(5), UINT32_C(2))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(5), UINT32_C(4))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(6), UINT32_C(0))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(6), UINT32_C(1))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(6), UINT32_C(2))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(6), UINT32_C(4))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_align_switch", UINT32_C(6), UINT32_C(8))->to_ui64() == UINT32_C(10));
}

TEST_CASE( "Testing wasm <align_107_wasm>", "[align_107_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_107_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "store", UINT32_C(65532), UINT64_C(18446744073709551615)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "load", UINT32_C(65532))->to_ui32() == UINT32_C(0));
}

TEST_CASE( "Testing wasm <align_10_wasm>", "[align_10_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_10_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_11_wasm>", "[align_11_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_11_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_12_wasm>", "[align_12_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_12_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_13_wasm>", "[align_13_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_13_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_14_wasm>", "[align_14_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_14_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_15_wasm>", "[align_15_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_15_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_16_wasm>", "[align_16_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_16_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_17_wasm>", "[align_17_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_17_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_18_wasm>", "[align_18_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_18_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_19_wasm>", "[align_19_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_19_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_1_wasm>", "[align_1_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_1_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_20_wasm>", "[align_20_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_20_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_21_wasm>", "[align_21_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_21_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_22_wasm>", "[align_22_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_22_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_2_wasm>", "[align_2_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_2_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_3_wasm>", "[align_3_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_3_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_4_wasm>", "[align_4_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_4_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_5_wasm>", "[align_5_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_5_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_6_wasm>", "[align_6_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_6_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_7_wasm>", "[align_7_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_7_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_8_wasm>", "[align_8_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_8_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

TEST_CASE( "Testing wasm <align_9_wasm>", "[align_9_wasm_tests]" ) {
   auto code = backend_t::read_wasm( align_9_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

