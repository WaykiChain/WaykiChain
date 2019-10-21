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

BACKEND_TEST_CASE( "Testing wasm <loop_0_wasm>", "[loop_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "empty"));
   CHECK(bkend.call_with_return(nullptr, "env", "singular")->to_ui32() == UINT32_C(7));
   CHECK(bkend.call_with_return(nullptr, "env", "multi")->to_ui32() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "nested")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "deep")->to_ui32() == UINT32_C(150));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last")->to_ui32() == UINT32_C(2));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-then")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-else")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-first")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call_indirect-last")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-call-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.set-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-local.tee-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-global.set-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-operand")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-unary-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-operand")->to_ui32() == UINT32_C(12));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "break-bare")->to_ui32() == UINT32_C(19));
   CHECK(bkend.call_with_return(nullptr, "env", "break-value")->to_ui32() == UINT32_C(18));
   CHECK(bkend.call_with_return(nullptr, "env", "break-repeated")->to_ui32() == UINT32_C(18));
   CHECK(bkend.call_with_return(nullptr, "env", "break-inner")->to_ui32() == UINT32_C(31));
   CHECK(bkend.call_with_return(nullptr, "env", "effects")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "while", UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "while", UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "while", UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "while", UINT64_C(3))->to_ui64() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "while", UINT64_C(5))->to_ui64() == UINT32_C(120));
   CHECK(bkend.call_with_return(nullptr, "env", "while", UINT64_C(20))->to_ui64() == UINT32_C(2432902008176640000));
   CHECK(bkend.call_with_return(nullptr, "env", "for", UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "for", UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "for", UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "for", UINT64_C(3))->to_ui64() == UINT32_C(6));
   CHECK(bkend.call_with_return(nullptr, "env", "for", UINT64_C(5))->to_ui64() == UINT32_C(120));
   CHECK(bkend.call_with_return(nullptr, "env", "for", UINT64_C(20))->to_ui64() == UINT32_C(2432902008176640000));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(1088421888)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1088421888)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1073741824)))->to_f32()) == UINT32_C(1073741824));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1077936128)))->to_f32()) == UINT32_C(1082130432));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1082130432)))->to_f32()) == UINT32_C(1086324736));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1120403456)))->to_f32()) == UINT32_C(1159684096));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1120534528)))->to_f32()) == UINT32_C(1159892992));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1092616192)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1073741824)))->to_f32()) == UINT32_C(1077936128));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1077936128)))->to_f32()) == UINT32_C(1082130432));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1088421888)), bit_cast<float>(UINT32_C(1082130432)))->to_f32()) == UINT32_C(1092940751));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1088421888)), bit_cast<float>(UINT32_C(1120403456)))->to_f32()) == UINT32_C(1166601314));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "nesting", bit_cast<float>(UINT32_C(1088421888)), bit_cast<float>(UINT32_C(1120534528)))->to_f32()) == UINT32_C(1159892992));
}

BACKEND_TEST_CASE( "Testing wasm <loop_1_wasm>", "[loop_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_10_wasm>", "[loop_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_11_wasm>", "[loop_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_12_wasm>", "[loop_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_2_wasm>", "[loop_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_3_wasm>", "[loop_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_4_wasm>", "[loop_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_5_wasm>", "[loop_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_6_wasm>", "[loop_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.6.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_7_wasm>", "[loop_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.7.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_8_wasm>", "[loop_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.8.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <loop_9_wasm>", "[loop_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "loop.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

