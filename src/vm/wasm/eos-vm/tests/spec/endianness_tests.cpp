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

BACKEND_TEST_CASE( "Testing wasm <endianness_0_wasm>", "[endianness_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "endianness.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(4294963054))->to_ui32() == UINT32_C(4294963054));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(42))->to_ui32() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(12816))->to_ui32() == UINT32_C(12816));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(4294967295))->to_ui32() == UINT32_C(65535));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(4294963054))->to_ui32() == UINT32_C(61294));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(42))->to_ui32() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(51966))->to_ui32() == UINT32_C(51966));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load", UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load", UINT32_C(4252543054))->to_ui32() == UINT32_C(4252543054));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load", UINT32_C(42424242))->to_ui32() == UINT32_C(42424242));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load", UINT32_C(2880249322))->to_ui32() == UINT32_C(2880249322));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(18446744073709547374))->to_ui64() == UINT32_C(18446744073709547374));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(42))->to_ui64() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(12816))->to_ui64() == UINT32_C(12816));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(65535));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(18446744073709547374))->to_ui64() == UINT32_C(61294));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(42))->to_ui64() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(51966))->to_ui64() == UINT32_C(51966));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(18446744073667127374))->to_ui64() == UINT32_C(18446744073667127374));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(42424242))->to_ui64() == UINT32_C(42424242));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(305419896))->to_ui64() == UINT32_C(305419896));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(18446744073667127374))->to_ui64() == UINT32_C(4252543054));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(42424242))->to_ui64() == UINT32_C(42424242));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(2880249322))->to_ui64() == UINT32_C(2880249322));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load", UINT64_C(18446744073667127374))->to_ui64() == UINT32_C(18446744073667127374));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load", UINT64_C(2880249322))->to_ui64() == UINT32_C(2880249322));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load", UINT64_C(12370766947463011818))->to_ui64() == UINT32_C(12370766947463011818));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_load", bit_cast<float>(UINT32_C(3212836864)))->to_f32()) == UINT32_C(3212836864));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_load", bit_cast<float>(UINT32_C(1011494326)))->to_f32()) == UINT32_C(1011494326));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_load", bit_cast<float>(UINT32_C(1166316389)))->to_f32()) == UINT32_C(1166316389));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_load", bit_cast<float>(UINT32_C(2139095039)))->to_f32()) == UINT32_C(2139095039));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_load", bit_cast<double>(UINT64_C(13830554455654793216)))->to_f64()) == UINT64_C(13830554455654793216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_load", bit_cast<double>(UINT64_C(4653144502447687399)))->to_f64()) == UINT64_C(4653144502447687399));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_load", bit_cast<double>(UINT64_C(4691032041816096430)))->to_f64()) == UINT64_C(4691032041816096430));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_load", bit_cast<double>(UINT64_C(9218868437227405311)))->to_f64()) == UINT64_C(9218868437227405311));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store16", UINT32_C(4294967295))->to_ui32() == UINT32_C(65535));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store16", UINT32_C(4294963054))->to_ui32() == UINT32_C(61294));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store16", UINT32_C(42))->to_ui32() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store16", UINT32_C(51966))->to_ui32() == UINT32_C(51966));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store", UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store", UINT32_C(4294963054))->to_ui32() == UINT32_C(4294963054));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store", UINT32_C(42424242))->to_ui32() == UINT32_C(42424242));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store", UINT32_C(3735931646))->to_ui32() == UINT32_C(3735931646));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store16", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(65535));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store16", UINT64_C(18446744073709547374))->to_ui64() == UINT32_C(61294));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store16", UINT64_C(42))->to_ui64() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store16", UINT64_C(51966))->to_ui64() == UINT32_C(51966));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store32", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store32", UINT64_C(18446744073709547374))->to_ui64() == UINT32_C(4294963054));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store32", UINT64_C(42424242))->to_ui64() == UINT32_C(42424242));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store32", UINT64_C(3735931646))->to_ui64() == UINT32_C(3735931646));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store", UINT64_C(18446744073667127374))->to_ui64() == UINT32_C(18446744073667127374));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store", UINT64_C(2880249322))->to_ui64() == UINT32_C(2880249322));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store", UINT64_C(12370766947463011818))->to_ui64() == UINT32_C(12370766947463011818));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_store", bit_cast<float>(UINT32_C(3212836864)))->to_f32()) == UINT32_C(3212836864));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_store", bit_cast<float>(UINT32_C(1011494326)))->to_f32()) == UINT32_C(1011494326));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_store", bit_cast<float>(UINT32_C(1166316389)))->to_f32()) == UINT32_C(1166316389));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_store", bit_cast<float>(UINT32_C(2139095039)))->to_f32()) == UINT32_C(2139095039));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_store", bit_cast<double>(UINT64_C(13830554455654793216)))->to_f64()) == UINT64_C(13830554455654793216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_store", bit_cast<double>(UINT64_C(4653144502447687399)))->to_f64()) == UINT64_C(4653144502447687399));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_store", bit_cast<double>(UINT64_C(4691032041816096430)))->to_f64()) == UINT64_C(4691032041816096430));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_store", bit_cast<double>(UINT64_C(9218868437227405311)))->to_f64()) == UINT64_C(9218868437227405311));
}

