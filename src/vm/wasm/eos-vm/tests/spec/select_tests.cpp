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

BACKEND_TEST_CASE( "Testing wasm <select_0_wasm>", "[select_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "select_i32", UINT32_C(1), UINT32_C(2), UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "select_i64", UINT64_C(2), UINT64_C(1), UINT32_C(1))->to_ui64() == UINT32_C(2));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1073741824)), UINT32_C(1))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(4611686018427387904)), UINT32_C(1))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bkend.call_with_return(nullptr, "env", "select_i32", UINT32_C(1), UINT32_C(2), UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "select_i32", UINT32_C(2), UINT32_C(1), UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "select_i64", UINT64_C(2), UINT64_C(1), UINT32_C(4294967295))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "select_i64", UINT64_C(2), UINT64_C(1), UINT32_C(4042322160))->to_ui64() == UINT32_C(2));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(1065353216)), UINT32_C(1))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(2139226884)), bit_cast<float>(UINT32_C(1065353216)), UINT32_C(1))->to_f32()) == UINT32_C(2139226884));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(1065353216)), UINT32_C(0))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(2139226884)), bit_cast<float>(UINT32_C(1065353216)), UINT32_C(0))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2143289344)), UINT32_C(1))->to_f32()) == UINT32_C(1073741824));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2139226884)), UINT32_C(1))->to_f32()) == UINT32_C(1073741824));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2143289344)), UINT32_C(0))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "select_f32", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2139226884)), UINT32_C(0))->to_f32()) == UINT32_C(2139226884));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(4607182418800017408)), UINT32_C(1))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(9218868437227537156)), bit_cast<double>(UINT64_C(4607182418800017408)), UINT32_C(1))->to_f64()) == UINT64_C(9218868437227537156));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(4607182418800017408)), UINT32_C(0))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(9218868437227537156)), bit_cast<double>(UINT64_C(4607182418800017408)), UINT32_C(0))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9221120237041090560)), UINT32_C(1))->to_f64()) == UINT64_C(4611686018427387904));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9218868437227537156)), UINT32_C(1))->to_f64()) == UINT64_C(4611686018427387904));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9221120237041090560)), UINT32_C(0))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "select_f64", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9218868437227537156)), UINT32_C(0))->to_f64()) == UINT64_C(9218868437227537156));
   CHECK_THROWS_AS(bkend(nullptr, "env", "select_trap_l", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "select_trap_l", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "select_trap_r", UINT32_C(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "select_trap_r", UINT32_C(0)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first", UINT32_C(1))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last", UINT32_C(1))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-first", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-mid", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-loop-last", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition", UINT32_C(1)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-first", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-first", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first", UINT32_C(0))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first", UINT32_C(1))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK_THROWS_AS(bkend(nullptr, "env", "as-call_indirect-last", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "as-call_indirect-last", UINT32_C(1)), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first", UINT32_C(1)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last", UINT32_C(1)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-value", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-value", UINT32_C(1))->to_ui32() == UINT32_C(3));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-value", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-value", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand", UINT32_C(0)));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand", UINT32_C(1)));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-value", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-value", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-value", UINT32_C(0))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-value", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-operand", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-operand", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-operand", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-operand", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-operand", UINT32_C(0))->to_ui32() == UINT32_C(4));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-operand", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-left", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-left", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-right", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-right", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-convert-operand", UINT32_C(0))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-convert-operand", UINT32_C(1))->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <select_1_wasm>", "[select_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_10_wasm>", "[select_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_11_wasm>", "[select_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_12_wasm>", "[select_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_13_wasm>", "[select_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.13.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_14_wasm>", "[select_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.14.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_15_wasm>", "[select_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.15.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_16_wasm>", "[select_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.16.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_2_wasm>", "[select_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_3_wasm>", "[select_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_4_wasm>", "[select_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_5_wasm>", "[select_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_6_wasm>", "[select_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.6.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_7_wasm>", "[select_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.7.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_8_wasm>", "[select_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.8.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <select_9_wasm>", "[select_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "select.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

