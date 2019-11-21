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

BACKEND_TEST_CASE( "Testing wasm <int_literals_0_wasm>", "[int_literals_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_literals.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.test")->to_ui32() == UINT32_C(195940365));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.umax")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.smax")->to_ui32() == UINT32_C(2147483647));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.neg_smax")->to_ui32() == UINT32_C(2147483649));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.smin")->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.alt_smin")->to_ui32() == UINT32_C(2147483648));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.inc_smin")->to_ui32() == UINT32_C(2147483649));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.neg_zero")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.not_octal")->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.unsigned_decimal")->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.plus_sign")->to_ui32() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.test")->to_ui64() == UINT32_C(913028331277281902));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.umax")->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.smax")->to_ui64() == UINT32_C(9223372036854775807));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.neg_smax")->to_ui64() == UINT32_C(9223372036854775809));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.smin")->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.alt_smin")->to_ui64() == UINT32_C(9223372036854775808));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.inc_smin")->to_ui64() == UINT32_C(9223372036854775809));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.neg_zero")->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.not_octal")->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.unsigned_decimal")->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.plus_sign")->to_ui64() == UINT32_C(42));
   CHECK(bkend.call_with_return(nullptr, "env", "i32-dec-sep1")->to_ui32() == UINT32_C(1000000));
   CHECK(bkend.call_with_return(nullptr, "env", "i32-dec-sep2")->to_ui32() == UINT32_C(1000));
   CHECK(bkend.call_with_return(nullptr, "env", "i32-hex-sep1")->to_ui32() == UINT32_C(168755353));
   CHECK(bkend.call_with_return(nullptr, "env", "i32-hex-sep2")->to_ui32() == UINT32_C(109071));
   CHECK(bkend.call_with_return(nullptr, "env", "i64-dec-sep1")->to_ui64() == UINT32_C(1000000));
   CHECK(bkend.call_with_return(nullptr, "env", "i64-dec-sep2")->to_ui64() == UINT32_C(1000));
   CHECK(bkend.call_with_return(nullptr, "env", "i64-hex-sep1")->to_ui64() == UINT32_C(3078696982321561));
   CHECK(bkend.call_with_return(nullptr, "env", "i64-hex-sep2")->to_ui64() == UINT32_C(109071));
}

