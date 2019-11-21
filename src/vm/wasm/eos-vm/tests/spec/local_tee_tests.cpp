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

BACKEND_TEST_CASE( "Testing wasm <local_tee_0_wasm>", "[local_tee_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "type-local-i32")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "type-local-i64")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "type-local-f32")->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "type-local-f64")->to_f64()) == UINT64_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "type-param-i32", UINT32_C(2))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "type-param-i64", UINT64_C(3))->to_ui64() == UINT32_C(11));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "type-param-f32", bit_cast<float>(UINT32_C(1082969293)))->to_f32()) == UINT32_C(1093769626));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "type-param-f64", bit_cast<double>(UINT64_C(4617878467915022336)))->to_f64()) == UINT64_C(4623057607486498406));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-first", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-mid", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-block-last", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid", UINT32_C(0))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last", UINT32_C(0))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value", UINT32_C(0))->to_ui32() == UINT32_C(9));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond", UINT32_C(0)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-value", UINT32_C(0))->to_ui32() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-value-cond", UINT32_C(0))->to_ui32() == UINT32_C(6));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index", UINT32_C(0)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-value", UINT32_C(0))->to_ui32() == UINT32_C(10));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-value-index", UINT32_C(0))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value", UINT32_C(0))->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-cond", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(1))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(0))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(0), UINT32_C(1))->to_ui32() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-second", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-cond", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-first", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-mid", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-last", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-last", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-index", UINT32_C(0))->to_ui32() == UINT32_C(4294967295));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-local.set-value"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-global.set-value"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-address", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loadN-address", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-address", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-value", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-storeN-address", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-storeN-value", UINT32_C(0)));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "as-unary-operand", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(4286640610));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-left", UINT32_C(0))->to_ui32() == UINT32_C(13));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-right", UINT32_C(0))->to_ui32() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-left", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-right", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-convert-operand", UINT64_C(0))->to_ui32() == UINT32_C(41));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-size", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-mixed", UINT64_C(1), bit_cast<float>(UINT32_C(1074580685)), bit_cast<double>(UINT64_C(4614613358185178726)), UINT32_C(4), UINT32_C(5)));
   CHECK(bkend.call_with_return(nullptr, "env", "write", UINT64_C(1), bit_cast<float>(UINT32_C(1073741824)), bit_cast<double>(UINT64_C(4614613358185178726)), UINT32_C(4), UINT32_C(5))->to_ui64() == UINT32_C(56));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "result", UINT64_C(18446744073709551615), bit_cast<float>(UINT32_C(3221225472)), bit_cast<double>(UINT64_C(13837985395039954534)), UINT32_C(4294967292), UINT32_C(4294967291))->to_f64()) == UINT64_C(4630094481904264806));
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_1_wasm>", "[local_tee_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_10_wasm>", "[local_tee_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_11_wasm>", "[local_tee_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_12_wasm>", "[local_tee_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_13_wasm>", "[local_tee_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.13.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_14_wasm>", "[local_tee_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.14.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_15_wasm>", "[local_tee_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.15.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_16_wasm>", "[local_tee_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.16.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_17_wasm>", "[local_tee_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.17.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_18_wasm>", "[local_tee_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.18.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_19_wasm>", "[local_tee_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.19.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_2_wasm>", "[local_tee_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_20_wasm>", "[local_tee_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.20.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_21_wasm>", "[local_tee_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.21.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_22_wasm>", "[local_tee_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.22.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_23_wasm>", "[local_tee_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.23.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_24_wasm>", "[local_tee_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.24.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_25_wasm>", "[local_tee_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.25.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_26_wasm>", "[local_tee_26_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.26.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_27_wasm>", "[local_tee_27_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.27.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_28_wasm>", "[local_tee_28_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.28.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_29_wasm>", "[local_tee_29_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.29.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_3_wasm>", "[local_tee_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_30_wasm>", "[local_tee_30_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.30.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_31_wasm>", "[local_tee_31_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.31.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_32_wasm>", "[local_tee_32_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.32.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_33_wasm>", "[local_tee_33_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.33.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_34_wasm>", "[local_tee_34_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.34.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_35_wasm>", "[local_tee_35_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.35.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_36_wasm>", "[local_tee_36_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.36.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_37_wasm>", "[local_tee_37_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.37.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_38_wasm>", "[local_tee_38_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.38.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_39_wasm>", "[local_tee_39_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.39.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_4_wasm>", "[local_tee_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_40_wasm>", "[local_tee_40_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.40.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_41_wasm>", "[local_tee_41_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.41.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_5_wasm>", "[local_tee_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_6_wasm>", "[local_tee_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.6.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_7_wasm>", "[local_tee_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.7.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_8_wasm>", "[local_tee_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.8.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <local_tee_9_wasm>", "[local_tee_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "local_tee.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

