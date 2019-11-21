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

BACKEND_TEST_CASE( "Testing wasm <traps_0_wasm>", "[traps_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "traps.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.div_s", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.div_u", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.div_s", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.div_u", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.div_s", UINT32_C(2147483648), UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.div_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <traps_1_wasm>", "[traps_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "traps.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.rem_s", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.rem_u", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.rem_s", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.rem_u", UINT64_C(1), UINT64_C(0)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <traps_2_wasm>", "[traps_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "traps.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.trunc_f32_s", bit_cast<float>(UINT32_C(2143289344))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.trunc_f32_u", bit_cast<float>(UINT32_C(2143289344))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.trunc_f64_s", bit_cast<double>(UINT64_C(9221120237041090560))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.trunc_f64_u", bit_cast<double>(UINT64_C(9221120237041090560))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.trunc_f32_s", bit_cast<float>(UINT32_C(2143289344))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.trunc_f32_u", bit_cast<float>(UINT32_C(2143289344))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.trunc_f64_s", bit_cast<double>(UINT64_C(9221120237041090560))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.trunc_f64_u", bit_cast<double>(UINT64_C(9221120237041090560))), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <traps_3_wasm>", "[traps_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "traps.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.load16_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.load16_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.load8_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i32.load8_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load32_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load32_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load16_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load16_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load8_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.i64.load8_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.f32.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "no_dce.f64.load", UINT32_C(65536)), std::exception);
}

