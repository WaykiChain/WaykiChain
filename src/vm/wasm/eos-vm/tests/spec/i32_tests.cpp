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

BACKEND_TEST_CASE( "Testing wasm <i32_0_wasm>", "[i32_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(2147483647), UINT32_C(1))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "add", UINT32_C(1073741823), UINT32_C(1))->to_ui32() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(2147483647), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(2147483648), UINT32_C(1))->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "sub", UINT32_C(1073741823), UINT32_C(4294967295))->to_ui32() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(268435456), UINT32_C(4096))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(2147483647), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483649));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(19088743), UINT32_C(1985229328))->to_ui32() == UINT32_C(898528368));
   CHECK(bkend.call_with_return(nullptr, "env", "mul", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_s", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_s", UINT32_C(0), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_s", UINT32_C(2147483648), UINT32_C(4294967295)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(0), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(2147483648), UINT32_C(2))->to_ui32() == UINT32_C(3221225472));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(2147483649), UINT32_C(1000))->to_ui32() == UINT32_C(4292819813));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(5), UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(4294967291), UINT32_C(2))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(5), UINT32_C(4294967294))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(4294967291), UINT32_C(4294967294))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(7), UINT32_C(3))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(4294967289), UINT32_C(3))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(7), UINT32_C(4294967293))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(4294967289), UINT32_C(4294967293))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(11), UINT32_C(5))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_s", UINT32_C(17), UINT32_C(7))->to_ui32() == UINT32_C(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_u", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "div_u", UINT32_C(0), UINT32_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(2147483648), UINT32_C(2))->to_ui32() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(2414874608), UINT32_C(65537))->to_ui32() == UINT32_C(36847));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(2147483649), UINT32_C(1000))->to_ui32() == UINT32_C(2147483));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(5), UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(4294967291), UINT32_C(2))->to_ui32() == UINT32_C(2147483645));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(5), UINT32_C(4294967294))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(4294967291), UINT32_C(4294967294))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(7), UINT32_C(3))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(11), UINT32_C(5))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "div_u", UINT32_C(17), UINT32_C(7))->to_ui32() == UINT32_C(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_s", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_s", UINT32_C(0), UINT32_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(2147483647), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(0), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(2147483648), UINT32_C(2))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(2147483649), UINT32_C(1000))->to_ui32() == UINT32_C(4294966649));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(5), UINT32_C(2))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(4294967291), UINT32_C(2))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(5), UINT32_C(4294967294))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(4294967291), UINT32_C(4294967294))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(7), UINT32_C(3))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(4294967289), UINT32_C(3))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(7), UINT32_C(4294967293))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(4294967289), UINT32_C(4294967293))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(11), UINT32_C(5))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_s", UINT32_C(17), UINT32_C(7))->to_ui32() == UINT32_C(3));
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_u", UINT32_C(1), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "rem_u", UINT32_C(0), UINT32_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(2147483648), UINT32_C(2))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(2414874608), UINT32_C(65537))->to_ui32() == UINT32_C(32769));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(2147483649), UINT32_C(1000))->to_ui32() == UINT32_C(649));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(5), UINT32_C(2))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(4294967291), UINT32_C(2))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(5), UINT32_C(4294967294))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(4294967291), UINT32_C(4294967294))->to_ui32() == UINT32_C(4294967291));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(7), UINT32_C(3))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(11), UINT32_C(5))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rem_u", UINT32_C(17), UINT32_C(7))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(2147483647), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(4042326015), UINT32_C(4294963440))->to_ui32() == UINT32_C(4042322160));
   CHECK(bkend.call_with_return(nullptr, "env", "and", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(4042326015), UINT32_C(4294963440))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "or", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(4294967295), UINT32_C(2147483647))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(4042326015), UINT32_C(4294963440))->to_ui32() == UINT32_C(252645135));
   CHECK(bkend.call_with_return(nullptr, "env", "xor", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(2147483647), UINT32_C(1))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(4294967294));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(2147483648), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1073741824), UINT32_C(1))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(31))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(32))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(33))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(4294967295))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "shl", UINT32_C(1), UINT32_C(2147483647))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(2147483647), UINT32_C(1))->to_ui32() == UINT32_C(1073741823));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(2147483648), UINT32_C(1))->to_ui32() == UINT32_C(3221225472));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1073741824), UINT32_C(1))->to_ui32() == UINT32_C(536870912));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(32))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(33))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(1), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(2147483648), UINT32_C(31))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(4294967295), UINT32_C(32))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(4294967295), UINT32_C(33))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(4294967295), UINT32_C(2147483647))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_s", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(2147483647), UINT32_C(1))->to_ui32() == UINT32_C(1073741823));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(2147483648), UINT32_C(1))->to_ui32() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1073741824), UINT32_C(1))->to_ui32() == UINT32_C(536870912));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(32))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(33))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(1), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(2147483648), UINT32_C(31))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(4294967295), UINT32_C(32))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(4294967295), UINT32_C(33))->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(4294967295), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "shr_u", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(1), UINT32_C(32))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(2882377846), UINT32_C(1))->to_ui32() == UINT32_C(1469788397));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(4261469184), UINT32_C(4))->to_ui32() == UINT32_C(3758997519));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(2965492451), UINT32_C(5))->to_ui32() == UINT32_C(406477942));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(32768), UINT32_C(37))->to_ui32() == UINT32_C(1048576));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(2965492451), UINT32_C(65285))->to_ui32() == UINT32_C(406477942));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(1989852383), UINT32_C(4294967277))->to_ui32() == UINT32_C(1469837011));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(1989852383), UINT32_C(2147483661))->to_ui32() == UINT32_C(1469837011));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(1), UINT32_C(31))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "rotl", UINT32_C(2147483648), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(1), UINT32_C(32))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(4278242304), UINT32_C(1))->to_ui32() == UINT32_C(2139121152));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(524288), UINT32_C(4))->to_ui32() == UINT32_C(32768));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(2965492451), UINT32_C(5))->to_ui32() == UINT32_C(495324823));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(32768), UINT32_C(37))->to_ui32() == UINT32_C(1024));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(2965492451), UINT32_C(65285))->to_ui32() == UINT32_C(495324823));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(1989852383), UINT32_C(4294967277))->to_ui32() == UINT32_C(3875255509));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(1989852383), UINT32_C(2147483661))->to_ui32() == UINT32_C(3875255509));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(1), UINT32_C(31))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "rotr", UINT32_C(2147483648), UINT32_C(31))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(0))->to_ui32() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(32768))->to_ui32() == UINT32_C(16));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(255))->to_ui32() == UINT32_C(24));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(1))->to_ui32() == UINT32_C(31));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(2))->to_ui32() == UINT32_C(30));
   CHECK(bkend.call_with_return(nullptr, "env", "clz", UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT32_C(0))->to_ui32() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT32_C(32768))->to_ui32() == UINT32_C(15));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT32_C(65536))->to_ui32() == UINT32_C(16));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT32_C(2147483648))->to_ui32() == UINT32_C(31));
   CHECK(bkend.call_with_return(nullptr, "env", "ctz", UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(4294967295))->to_ui32() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(32768))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(2147516416))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(2147483647))->to_ui32() == UINT32_C(31));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(2863311530))->to_ui32() == UINT32_C(16));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(1431655765))->to_ui32() == UINT32_C(16));
   CHECK(bkend.call_with_return(nullptr, "env", "popcnt", UINT32_C(3735928559))->to_ui32() == UINT32_C(24));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eqz", UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "eq", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ne", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_s", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "lt_u", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_s", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "le_u", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_s", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "gt_u", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_s", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(1), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(4294967295), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(2147483648), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(2147483647), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(4294967295), UINT32_C(4294967295))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(2147483648), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(0), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(2147483648), UINT32_C(4294967295))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(4294967295), UINT32_C(2147483648))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(2147483648), UINT32_C(2147483647))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "ge_u", UINT32_C(2147483647), UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <i32_1_wasm>", "[i32_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_10_wasm>", "[i32_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_11_wasm>", "[i32_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_12_wasm>", "[i32_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_13_wasm>", "[i32_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.13.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_14_wasm>", "[i32_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.14.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_15_wasm>", "[i32_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.15.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_16_wasm>", "[i32_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.16.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_17_wasm>", "[i32_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.17.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_18_wasm>", "[i32_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.18.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_19_wasm>", "[i32_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.19.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_2_wasm>", "[i32_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_20_wasm>", "[i32_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.20.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_21_wasm>", "[i32_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.21.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_22_wasm>", "[i32_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.22.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_23_wasm>", "[i32_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.23.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_24_wasm>", "[i32_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.24.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_25_wasm>", "[i32_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.25.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_26_wasm>", "[i32_26_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.26.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_27_wasm>", "[i32_27_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.27.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_28_wasm>", "[i32_28_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.28.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_29_wasm>", "[i32_29_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.29.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_3_wasm>", "[i32_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_30_wasm>", "[i32_30_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.30.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_31_wasm>", "[i32_31_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.31.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_32_wasm>", "[i32_32_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.32.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_33_wasm>", "[i32_33_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.33.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_34_wasm>", "[i32_34_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.34.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_35_wasm>", "[i32_35_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.35.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_36_wasm>", "[i32_36_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.36.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_37_wasm>", "[i32_37_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.37.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_38_wasm>", "[i32_38_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.38.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_39_wasm>", "[i32_39_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.39.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_4_wasm>", "[i32_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_40_wasm>", "[i32_40_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.40.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_41_wasm>", "[i32_41_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.41.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_42_wasm>", "[i32_42_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.42.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_43_wasm>", "[i32_43_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.43.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_44_wasm>", "[i32_44_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.44.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_45_wasm>", "[i32_45_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.45.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_46_wasm>", "[i32_46_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.46.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_47_wasm>", "[i32_47_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.47.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_48_wasm>", "[i32_48_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.48.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_49_wasm>", "[i32_49_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.49.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_5_wasm>", "[i32_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_50_wasm>", "[i32_50_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.50.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_51_wasm>", "[i32_51_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.51.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_52_wasm>", "[i32_52_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.52.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_53_wasm>", "[i32_53_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.53.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_54_wasm>", "[i32_54_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.54.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_55_wasm>", "[i32_55_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.55.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_56_wasm>", "[i32_56_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.56.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_57_wasm>", "[i32_57_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.57.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_58_wasm>", "[i32_58_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.58.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_59_wasm>", "[i32_59_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.59.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_6_wasm>", "[i32_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.6.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_60_wasm>", "[i32_60_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.60.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_61_wasm>", "[i32_61_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.61.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_62_wasm>", "[i32_62_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.62.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_63_wasm>", "[i32_63_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.63.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_64_wasm>", "[i32_64_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.64.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_65_wasm>", "[i32_65_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.65.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_66_wasm>", "[i32_66_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.66.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_67_wasm>", "[i32_67_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.67.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_68_wasm>", "[i32_68_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.68.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_69_wasm>", "[i32_69_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.69.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_7_wasm>", "[i32_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.7.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_70_wasm>", "[i32_70_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.70.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_71_wasm>", "[i32_71_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.71.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_72_wasm>", "[i32_72_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.72.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_73_wasm>", "[i32_73_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.73.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_74_wasm>", "[i32_74_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.74.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_75_wasm>", "[i32_75_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.75.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_76_wasm>", "[i32_76_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.76.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_77_wasm>", "[i32_77_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.77.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_78_wasm>", "[i32_78_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.78.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_79_wasm>", "[i32_79_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.79.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_8_wasm>", "[i32_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.8.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_80_wasm>", "[i32_80_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.80.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_81_wasm>", "[i32_81_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.81.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_82_wasm>", "[i32_82_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.82.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_83_wasm>", "[i32_83_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.83.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <i32_9_wasm>", "[i32_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "i32.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

