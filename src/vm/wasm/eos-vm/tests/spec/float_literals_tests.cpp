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

BACKEND_TEST_CASE( "Testing wasm <float_literals_0_wasm>", "[float_literals_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_literals.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.nan")->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.positive_nan")->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.negative_nan")->to_ui32() == UINT32_C(4290772992));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.plain_nan")->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.informally_known_as_plain_snan")->to_ui32() == UINT32_C(2141192192));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.all_ones_nan")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.misc_nan")->to_ui32() == UINT32_C(2139169605));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.misc_positive_nan")->to_ui32() == UINT32_C(2142257232));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.misc_negative_nan")->to_ui32() == UINT32_C(4289379550));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.infinity")->to_ui32() == UINT32_C(2139095040));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.positive_infinity")->to_ui32() == UINT32_C(2139095040));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.negative_infinity")->to_ui32() == UINT32_C(4286578688));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.zero")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.positive_zero")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.negative_zero")->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.misc")->to_ui32() == UINT32_C(1086918619));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.min_positive")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.min_normal")->to_ui32() == UINT32_C(8388608));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.max_subnormal")->to_ui32() == UINT32_C(8388607));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.max_finite")->to_ui32() == UINT32_C(2139095039));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.trailing_dot")->to_ui32() == UINT32_C(1149239296));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.zero")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.positive_zero")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.negative_zero")->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.misc")->to_ui32() == UINT32_C(1086918619));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.min_positive")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.min_normal")->to_ui32() == UINT32_C(8388608));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.max_subnormal")->to_ui32() == UINT32_C(8388607));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.max_finite")->to_ui32() == UINT32_C(2139095039));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.trailing_dot")->to_ui32() == UINT32_C(1343554297));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_dec.root_beer_float")->to_ui32() == UINT32_C(1065353217));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.nan")->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.positive_nan")->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.negative_nan")->to_ui64() == UINT32_C(18444492273895866368));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.plain_nan")->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.informally_known_as_plain_snan")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.all_ones_nan")->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.misc_nan")->to_ui64() == UINT32_C(9218888453225749180));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.misc_positive_nan")->to_ui64() == UINT32_C(9219717281780008969));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.misc_negative_nan")->to_ui64() == UINT32_C(18442992325002076997));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.infinity")->to_ui64() == UINT32_C(9218868437227405312));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.positive_infinity")->to_ui64() == UINT32_C(9218868437227405312));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.negative_infinity")->to_ui64() == UINT32_C(18442240474082181120));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.zero")->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.positive_zero")->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.negative_zero")->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.misc")->to_ui64() == UINT32_C(4618760256179416344));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.min_positive")->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.min_normal")->to_ui64() == UINT32_C(4503599627370496));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.max_subnormal")->to_ui64() == UINT32_C(4503599627370495));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.max_finite")->to_ui64() == UINT32_C(9218868437227405311));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.trailing_dot")->to_ui64() == UINT32_C(5057542381537067008));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.zero")->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.positive_zero")->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.negative_zero")->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.misc")->to_ui64() == UINT32_C(4618760256179416344));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.min_positive")->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.min_normal")->to_ui64() == UINT32_C(4503599627370496));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.max_subnormal")->to_ui64() == UINT32_C(4503599627370495));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.max_finite")->to_ui64() == UINT32_C(9218868437227405311));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.trailing_dot")->to_ui64() == UINT32_C(6103021453049119613));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_dec.root_beer_float")->to_ui64() == UINT32_C(4607182419335945764));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-dec-sep1")->to_f32()) == UINT32_C(1232348160));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-dec-sep2")->to_f32()) == UINT32_C(1148846080));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-dec-sep3")->to_f32()) == UINT32_C(1148897552));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-dec-sep4")->to_f32()) == UINT32_C(1482758550));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-dec-sep5")->to_f32()) == UINT32_C(1847438964));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-hex-sep1")->to_f32()) == UINT32_C(1294004234));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-hex-sep2")->to_f32()) == UINT32_C(1205143424));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-hex-sep3")->to_f32()) == UINT32_C(1193345009));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-hex-sep4")->to_f32()) == UINT32_C(1240465408));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32-hex-sep5")->to_f32()) == UINT32_C(1437319208));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-dec-sep1")->to_f64()) == UINT64_C(4696837146684686336));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-dec-sep2")->to_f64()) == UINT64_C(4652007308841189376));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-dec-sep3")->to_f64()) == UINT64_C(4652034942576659200));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-dec-sep4")->to_f64()) == UINT64_C(2796837019126844485));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-dec-sep5")->to_f64()) == UINT64_C(5027061507362119324));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-hex-sep1")->to_f64()) == UINT64_C(4838519794133185330));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-hex-sep2")->to_f64()) == UINT64_C(4682231715257647104));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-hex-sep3")->to_f64()) == UINT64_C(4675897489574114112));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-hex-sep4")->to_f64()) == UINT64_C(4701195061021376512));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64-hex-sep5")->to_f64()) == UINT64_C(4806880140420149248));
}

BACKEND_TEST_CASE( "Testing wasm <float_literals_1_wasm>", "[float_literals_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_literals.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "4294967249")->to_f64()) == UINT64_C(4751297606777307136));
}

