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

BACKEND_TEST_CASE( "Testing wasm <int_exprs_0_wasm>", "[int_exprs_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_cmp_s_offset", UINT32_C(2147483647), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_cmp_u_offset", UINT32_C(4294967295), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_cmp_s_offset", UINT64_C(9223372036854775807), UINT64_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_cmp_u_offset", UINT64_C(18446744073709551615), UINT64_C(0))->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_1_wasm>", "[int_exprs_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_wrap_extend_s", UINT64_C(4538991236898928))->to_ui64() == UINT32_C(1079009392));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_wrap_extend_s", UINT64_C(45230338458316960))->to_ui64() == UINT32_C(18446744072918986912));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_10_wasm>", "[int_exprs_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.10.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_rem_s_2", UINT32_C(4294967285))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_rem_s_2", UINT64_C(18446744073709551605))->to_ui64() == UINT32_C(18446744073709551615));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_11_wasm>", "[int_exprs_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.11.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.div_s_0", UINT32_C(71)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.div_u_0", UINT32_C(71)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.div_s_0", UINT64_C(71)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.div_u_0", UINT64_C(71)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_12_wasm>", "[int_exprs_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.12.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_s_3", UINT32_C(71))->to_ui32() == UINT32_C(23));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_s_3", UINT32_C(1610612736))->to_ui32() == UINT32_C(536870912));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_u_3", UINT32_C(71))->to_ui32() == UINT32_C(23));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_u_3", UINT32_C(3221225472))->to_ui32() == UINT32_C(1073741824));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_s_3", UINT64_C(71))->to_ui64() == UINT32_C(23));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_s_3", UINT64_C(3458764513820540928))->to_ui64() == UINT32_C(1152921504606846976));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_u_3", UINT64_C(71))->to_ui64() == UINT32_C(23));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_u_3", UINT64_C(13835058055282163712))->to_ui64() == UINT32_C(4611686018427387904));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_13_wasm>", "[int_exprs_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.13.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_s_5", UINT32_C(71))->to_ui32() == UINT32_C(14));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_s_5", UINT32_C(1342177280))->to_ui32() == UINT32_C(268435456));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_u_5", UINT32_C(71))->to_ui32() == UINT32_C(14));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_u_5", UINT32_C(2684354560))->to_ui32() == UINT32_C(536870912));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_s_5", UINT64_C(71))->to_ui64() == UINT32_C(14));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_s_5", UINT64_C(5764607523034234880))->to_ui64() == UINT32_C(1152921504606846976));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_u_5", UINT64_C(71))->to_ui64() == UINT32_C(14));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_u_5", UINT64_C(11529215046068469760))->to_ui64() == UINT32_C(2305843009213693952));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_14_wasm>", "[int_exprs_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.14.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_s_7", UINT32_C(71))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_s_7", UINT32_C(1879048192))->to_ui32() == UINT32_C(268435456));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_u_7", UINT32_C(71))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.div_u_7", UINT32_C(3758096384))->to_ui32() == UINT32_C(536870912));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_s_7", UINT64_C(71))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_s_7", UINT64_C(8070450532247928832))->to_ui64() == UINT32_C(1152921504606846976));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_u_7", UINT64_C(71))->to_ui64() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.div_u_7", UINT64_C(16140901064495857664))->to_ui64() == UINT32_C(2305843009213693952));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_15_wasm>", "[int_exprs_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.15.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_s_3", UINT32_C(71))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_s_3", UINT32_C(1610612736))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_u_3", UINT32_C(71))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_u_3", UINT32_C(3221225472))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_s_3", UINT64_C(71))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_s_3", UINT64_C(3458764513820540928))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_u_3", UINT64_C(71))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_u_3", UINT64_C(13835058055282163712))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_16_wasm>", "[int_exprs_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.16.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_s_5", UINT32_C(71))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_s_5", UINT32_C(1342177280))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_u_5", UINT32_C(71))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_u_5", UINT32_C(2684354560))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_s_5", UINT64_C(71))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_s_5", UINT64_C(5764607523034234880))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_u_5", UINT64_C(71))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_u_5", UINT64_C(11529215046068469760))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_17_wasm>", "[int_exprs_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.17.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_s_7", UINT32_C(71))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_s_7", UINT32_C(1879048192))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_u_7", UINT32_C(71))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.rem_u_7", UINT32_C(3758096384))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_s_7", UINT64_C(71))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_s_7", UINT64_C(8070450532247928832))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_u_7", UINT64_C(71))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.rem_u_7", UINT64_C(16140901064495857664))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_18_wasm>", "[int_exprs_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.18.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.no_fold_div_neg1", UINT32_C(2147483648)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.no_fold_div_neg1", UINT64_C(9223372036854775808)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_2_wasm>", "[int_exprs_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_wrap_extend_u", UINT64_C(4538991236898928))->to_ui64() == UINT32_C(1079009392));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_3_wasm>", "[int_exprs_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_shl_shr_s", UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_shl_shr_u", UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_shl_shr_s", UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_shl_shr_u", UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_4_wasm>", "[int_exprs_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_shr_s_shl", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_shr_u_shl", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_shr_s_shl", UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_shr_u_shl", UINT64_C(1))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_5_wasm>", "[int_exprs_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.5.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_div_s_mul", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_div_u_mul", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_div_s_mul", UINT64_C(1))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_div_u_mul", UINT64_C(1))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_6_wasm>", "[int_exprs_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.6.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.no_fold_div_s_self", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.no_fold_div_u_self", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.no_fold_div_s_self", UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.no_fold_div_u_self", UINT64_C(0)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_7_wasm>", "[int_exprs_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.7.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.no_fold_rem_s_self", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.no_fold_rem_u_self", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.no_fold_rem_s_self", UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.no_fold_rem_u_self", UINT64_C(0)), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_8_wasm>", "[int_exprs_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.8.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_mul_div_s", UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_mul_div_u", UINT32_C(2147483648))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_mul_div_s", UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_mul_div_u", UINT64_C(9223372036854775808))->to_ui64() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <int_exprs_9_wasm>", "[int_exprs_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "int_exprs.9.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_div_s_2", UINT32_C(4294967285))->to_ui32() == UINT32_C(4294967291));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_div_s_2", UINT64_C(18446744073709551605))->to_ui64() == UINT32_C(18446744073709551611));
}

