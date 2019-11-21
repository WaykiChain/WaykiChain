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

BACKEND_TEST_CASE( "Testing wasm <i64_0_wasm>", "[i64_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(9223372036854775807), UINT64_C(1))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT64_C(1073741823), UINT64_C(1))->to_ui64() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(9223372036854775807), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(9223372036854775808), UINT64_C(1))->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT64_C(1073741823), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(1152921504606846976), UINT64_C(4096))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(9223372036854775807), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775809));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(81985529216486895), UINT64_C(18364758544493064720))->to_ui64() == UINT32_C(2465395958572223728));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(1));
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_s", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_s", UINT64_C(0), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(0), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(9223372036854775808), UINT64_C(2))->to_ui64() == UINT32_C(13835058055282163712));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(9223372036854775809), UINT64_C(1000))->to_ui64() == UINT32_C(18437520701672696841));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(5), UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(18446744073709551611), UINT64_C(2))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(5), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(18446744073709551611), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(7), UINT64_C(3))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(18446744073709551609), UINT64_C(3))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(7), UINT64_C(18446744073709551613))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(18446744073709551609), UINT64_C(18446744073709551613))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(11), UINT64_C(5))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT64_C(17), UINT64_C(7))->to_ui64() == UINT32_C(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_u", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_u", UINT64_C(0), UINT64_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(9223372036854775808), UINT64_C(2))->to_ui64() == UINT32_C(4611686018427387904));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(10371807465568210928), UINT64_C(4294967297))->to_ui64() == UINT32_C(2414874607));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(9223372036854775809), UINT64_C(1000))->to_ui64() == UINT32_C(9223372036854775));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(5), UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(18446744073709551611), UINT64_C(2))->to_ui64() == UINT32_C(9223372036854775805));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(5), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(18446744073709551611), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(7), UINT64_C(3))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(11), UINT64_C(5))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT64_C(17), UINT64_C(7))->to_ui64() == UINT32_C(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_s", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_s", UINT64_C(0), UINT64_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(9223372036854775807), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(0), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(9223372036854775808), UINT64_C(2))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(9223372036854775809), UINT64_C(1000))->to_ui64() == UINT32_C(18446744073709550809));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(5), UINT64_C(2))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(18446744073709551611), UINT64_C(2))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(5), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(18446744073709551611), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(7), UINT64_C(3))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(18446744073709551609), UINT64_C(3))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(7), UINT64_C(18446744073709551613))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(18446744073709551609), UINT64_C(18446744073709551613))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(11), UINT64_C(5))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT64_C(17), UINT64_C(7))->to_ui64() == UINT32_C(3));
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_u", UINT64_C(1), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_u", UINT64_C(0), UINT64_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(9223372036854775808), UINT64_C(2))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(10371807465568210928), UINT64_C(4294967297))->to_ui64() == UINT32_C(2147483649));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(9223372036854775809), UINT64_C(1000))->to_ui64() == UINT32_C(809));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(5), UINT64_C(2))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(18446744073709551611), UINT64_C(2))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(5), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(18446744073709551611), UINT64_C(18446744073709551614))->to_ui64() == UINT32_C(18446744073709551611));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(7), UINT64_C(3))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(11), UINT64_C(5))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT64_C(17), UINT64_C(7))->to_ui64() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(0), UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(9223372036854775807), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(4042326015), UINT64_C(4294963440))->to_ui64() == UINT32_C(4042322160));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(0), UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(4042326015), UINT64_C(4294963440))->to_ui64() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(0), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(0), UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(18446744073709551615), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(4042326015), UINT64_C(4294963440))->to_ui64() == UINT32_C(252645135));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(9223372036854775807), UINT64_C(1))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui64() == UINT32_C(18446744073709551614));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(9223372036854775808), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(4611686018427387904), UINT64_C(1))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(63))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(64))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(65))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT64_C(1), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(9223372036854775807), UINT64_C(1))->to_ui64() == UINT32_C(4611686018427387903));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(9223372036854775808), UINT64_C(1))->to_ui64() == UINT32_C(13835058055282163712));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(4611686018427387904), UINT64_C(1))->to_ui64() == UINT32_C(2305843009213693952));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(64))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(65))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(1), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(9223372036854775808), UINT64_C(63))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(18446744073709551615), UINT64_C(64))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(18446744073709551615), UINT64_C(65))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(18446744073709551615), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(9223372036854775807), UINT64_C(1))->to_ui64() == UINT32_C(4611686018427387903));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(9223372036854775808), UINT64_C(1))->to_ui64() == UINT32_C(4611686018427387904));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(4611686018427387904), UINT64_C(1))->to_ui64() == UINT32_C(2305843009213693952));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(64))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(65))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(1), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(9223372036854775808), UINT64_C(63))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(18446744073709551615), UINT64_C(64))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(18446744073709551615), UINT64_C(65))->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(18446744073709551615), UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(1), UINT64_C(64))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(12379718583323101902), UINT64_C(1))->to_ui64() == UINT32_C(6312693092936652189));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(18302628889324683264), UINT64_C(4))->to_ui64() == UINT32_C(16140901123551657999));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(12379570969274382345), UINT64_C(53))->to_ui64() == UINT32_C(87109505680009935));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(12380715672649826460), UINT64_C(63))->to_ui64() == UINT32_C(6190357836324913230));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(12379570969274382345), UINT64_C(245))->to_ui64() == UINT32_C(87109505680009935));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(12379676934707509257), UINT64_C(18446744073709551597))->to_ui64() == UINT32_C(14916262237559758314));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(12380715672649826460), UINT64_C(9223372036854775871))->to_ui64() == UINT32_C(6190357836324913230));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(1), UINT64_C(63))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT64_C(9223372036854775808), UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(1), UINT64_C(1))->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(1), UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(1), UINT64_C(64))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(12379718583323101902), UINT64_C(1))->to_ui64() == UINT32_C(6189859291661550951));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(18302628889324683264), UINT64_C(4))->to_ui64() == UINT32_C(1143914305582792704));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(12379570969274382345), UINT64_C(53))->to_ui64() == UINT32_C(7534987797011123550));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(12380715672649826460), UINT64_C(63))->to_ui64() == UINT32_C(6314687271590101305));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(12379570969274382345), UINT64_C(245))->to_ui64() == UINT32_C(7534987797011123550));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(12379676934707509257), UINT64_C(18446744073709551597))->to_ui64() == UINT32_C(10711665151168044651));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(12380715672649826460), UINT64_C(9223372036854775871))->to_ui64() == UINT32_C(6314687271590101305));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(1), UINT64_C(63))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT64_C(9223372036854775808), UINT64_C(63))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(0))->to_ui64() == UINT32_C(64));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(32768))->to_ui64() == UINT32_C(48));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(255))->to_ui64() == UINT32_C(56));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(1))->to_ui64() == UINT32_C(63));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(2))->to_ui64() == UINT32_C(62));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT64_C(0))->to_ui64() == UINT32_C(64));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT64_C(32768))->to_ui64() == UINT32_C(15));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT64_C(65536))->to_ui64() == UINT32_C(16));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(63));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(64));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(0))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(32768))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(9223512776490647552))->to_ui64() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(9223372036854775807))->to_ui64() == UINT32_C(63));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(12297829381041378645))->to_ui64() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(11068046444512062122))->to_ui64() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT64_C(16045690984833335023))->to_ui64() == UINT32_C(48));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(0), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(1), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(18446744073709551615), UINT64_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(18446744073709551615), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(1), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(0), UINT64_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(9223372036854775808), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(0), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(9223372036854775808), UINT64_C(18446744073709551615))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(18446744073709551615), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(9223372036854775808), UINT64_C(9223372036854775807))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT64_C(9223372036854775807), UINT64_C(9223372036854775808))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <i64_1_wasm>", "[i64_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_10_wasm>", "[i64_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_11_wasm>", "[i64_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_12_wasm>", "[i64_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_13_wasm>", "[i64_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.13.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_14_wasm>", "[i64_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.14.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_15_wasm>", "[i64_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.15.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_16_wasm>", "[i64_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.16.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_17_wasm>", "[i64_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.17.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_18_wasm>", "[i64_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.18.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_19_wasm>", "[i64_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.19.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_2_wasm>", "[i64_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_20_wasm>", "[i64_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.20.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_21_wasm>", "[i64_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.21.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_22_wasm>", "[i64_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.22.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_23_wasm>", "[i64_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.23.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_24_wasm>", "[i64_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.24.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_25_wasm>", "[i64_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.25.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_26_wasm>", "[i64_26_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.26.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_27_wasm>", "[i64_27_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.27.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_28_wasm>", "[i64_28_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.28.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_29_wasm>", "[i64_29_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.29.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_3_wasm>", "[i64_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_4_wasm>", "[i64_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_5_wasm>", "[i64_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_6_wasm>", "[i64_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.6.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_7_wasm>", "[i64_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.7.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_8_wasm>", "[i64_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.8.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i64_9_wasm>", "[i64_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i64.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

