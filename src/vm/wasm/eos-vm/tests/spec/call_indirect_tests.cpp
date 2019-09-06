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
using backend_t = backend<std::nullptr_t>;

TEST_CASE( "Testing wasm <call_indirect_0_wasm>", "[call_indirect_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( call_indirect_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);
/*
   CHECK(bkend.call_with_return(nullptr, "env", "type-i32")->to_ui32() == UINT32_C(306));
   CHECK(bkend.call_with_return(nullptr, "env", "type-i64")->to_ui64() == UINT32_C(356));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "type-f32")->to_f32()) == UINT32_C(1165172736));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "type-f64")->to_f64()) == UINT64_C(4660882566700597248));
   CHECK(bkend.call_with_return(nullptr, "env", "type-index")->to_ui64() == UINT32_C(100));
   CHECK(bkend.call_with_return(nullptr, "env", "type-first-i32")->to_ui32() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "type-first-i64")->to_ui64() == UINT32_C(64));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "type-first-f32")->to_f32()) == UINT32_C(1068037571));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "type-first-f64")->to_f64()) == UINT64_C(4610064722561534525));
   CHECK(bkend.call_with_return(nullptr, "env", "type-second-i32")->to_ui32() == UINT32_C(32));
   CHECK(bkend.call_with_return(nullptr, "env", "type-second-i64")->to_ui64() == UINT32_C(64));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "type-second-f32")->to_f32()) == UINT32_C(1107296256));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "type-second-f64")->to_f64()) == UINT64_C(4634211053438658150));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch", UINT32_C(5), UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch", UINT32_C(5), UINT64_C(5))->to_ui64() == UINT32_C(5));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch", UINT32_C(12), UINT64_C(5))->to_ui64() == UINT32_C(120));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch", UINT32_C(13), UINT64_C(5))->to_ui64() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch", UINT32_C(20), UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", UINT32_C(0), UINT64_C(2)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", UINT32_C(15), UINT64_C(2)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", UINT32_C(29), UINT64_C(2)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", UINT32_C(4294967295), UINT64_C(2)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", UINT32_C(1213432423), UINT64_C(2)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", UINT32_C(5))->to_ui64() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", UINT32_C(12))->to_ui64() == UINT32_C(362880));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", UINT32_C(13))->to_ui64() == UINT32_C(55));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", UINT32_C(20))->to_ui64() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i64", UINT32_C(11)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i64", UINT32_C(22)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", UINT32_C(4))->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", UINT32_C(23))->to_ui32() == UINT32_C(362880));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", UINT32_C(26))->to_ui32() == UINT32_C(55));
   CHECK(bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", UINT32_C(19))->to_ui32() == UINT32_C(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i32", UINT32_C(9)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i32", UINT32_C(21)), std::exception);
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", UINT32_C(6))->to_f32()) == UINT32_C(1091567616));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", UINT32_C(24))->to_f32()) == UINT32_C(1219571712));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", UINT32_C(27))->to_f32()) == UINT32_C(1113325568));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", UINT32_C(21))->to_f32()) == UINT32_C(1091567616));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f32", UINT32_C(8)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f32", UINT32_C(19)), std::exception);
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", UINT32_C(7))->to_f64()) == UINT64_C(4621256167635550208));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", UINT32_C(25))->to_f64()) == UINT64_C(4689977843394805760));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", UINT32_C(28))->to_f64()) == UINT64_C(4632937379169042432));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", UINT32_C(22))->to_f64()) == UINT64_C(4621256167635550208));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f64", UINT32_C(10)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f64", UINT32_C(18)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i64", UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i64", UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i64", UINT64_C(5))->to_ui64() == UINT32_C(120));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i64", UINT64_C(25))->to_ui64() == UINT32_C(7034535277573963776));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i32", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i32", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i32", UINT32_C(5))->to_ui32() == UINT32_C(120));
   CHECK(bkend.call_with_return(nullptr, "env", "fac-i32", UINT32_C(10))->to_ui32() == UINT32_C(3628800));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fac-f32", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fac-f32", bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fac-f32", bit_cast<float>(UINT32_C(1084227584)))->to_f32()) == UINT32_C(1123024896));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fac-f32", bit_cast<float>(UINT32_C(1092616192)))->to_f32()) == UINT32_C(1247640576));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fac-f64", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fac-f64", bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fac-f64", bit_cast<double>(UINT64_C(4617315517961601024)))->to_f64()) == UINT64_C(4638144666238189568));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fac-f64", bit_cast<double>(UINT64_C(4621819117588971520)))->to_f64()) == UINT64_C(4705047200009289728));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i64", UINT64_C(0))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i64", UINT64_C(1))->to_ui64() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i64", UINT64_C(2))->to_ui64() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i64", UINT64_C(5))->to_ui64() == UINT32_C(8));
*/
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i64", UINT64_C(20))->to_ui64() == UINT32_C(10946));
   return;
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i32", UINT32_C(0))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i32", UINT32_C(1))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i32", UINT32_C(2))->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i32", UINT32_C(5))->to_ui32() == UINT32_C(8));
   CHECK(bkend.call_with_return(nullptr, "env", "fib-i32", UINT32_C(20))->to_ui32() == UINT32_C(10946));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fib-f32", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fib-f32", bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fib-f32", bit_cast<float>(UINT32_C(1073741824)))->to_f32()) == UINT32_C(1073741824));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fib-f32", bit_cast<float>(UINT32_C(1084227584)))->to_f32()) == UINT32_C(1090519040));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "fib-f32", bit_cast<float>(UINT32_C(1101004800)))->to_f32()) == UINT32_C(1177225216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fib-f64", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fib-f64", bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fib-f64", bit_cast<double>(UINT64_C(4611686018427387904)))->to_f64()) == UINT64_C(4611686018427387904));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fib-f64", bit_cast<double>(UINT64_C(4617315517961601024)))->to_f64()) == UINT64_C(4620693217682128896));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "fib-f64", bit_cast<double>(UINT64_C(4626322717216342016)))->to_f64()) == UINT64_C(4667243241467281408));
   CHECK(bkend.call_with_return(nullptr, "env", "even", UINT32_C(0))->to_ui32() == UINT32_C(44));
   CHECK(bkend.call_with_return(nullptr, "env", "even", UINT32_C(1))->to_ui32() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "even", UINT32_C(100))->to_ui32() == UINT32_C(44));
   CHECK(bkend.call_with_return(nullptr, "env", "even", UINT32_C(77))->to_ui32() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "odd", UINT32_C(0))->to_ui32() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "odd", UINT32_C(1))->to_ui32() == UINT32_C(44));
   CHECK(bkend.call_with_return(nullptr, "env", "odd", UINT32_C(200))->to_ui32() == UINT32_C(99));
   CHECK(bkend.call_with_return(nullptr, "env", "odd", UINT32_C(77))->to_ui32() == UINT32_C(44));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-first")->to_ui32() == UINT32_C(306));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-mid")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-select-last")->to_ui32() == UINT32_C(2));
   CHECK(bkend.call_with_return(nullptr, "env", "as-if-condition")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-first")->to_ui64() == UINT32_C(356));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_if-last")->to_ui32() == UINT32_C(2));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "as-br_table-first")->to_f32()) == UINT32_C(1165172736));
   CHECK(bkend.call_with_return(nullptr, "env", "as-br_table-last")->to_ui32() == UINT32_C(2));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(bkend.call_with_return(nullptr, "env", "as-memory.grow-value")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-return-value")->to_ui32() == UINT32_C(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "as-br-value")->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "as-local.set-value")->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "as-local.tee-value")->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "as-global.set-value")->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bkend.call_with_return(nullptr, "env", "as-load-operand")->to_ui32() == UINT32_C(1));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "as-unary-operand")->to_f32()) == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-left")->to_ui32() == UINT32_C(11));
   CHECK(bkend.call_with_return(nullptr, "env", "as-binary-right")->to_ui32() == UINT32_C(9));
   CHECK(bkend.call_with_return(nullptr, "env", "as-test-operand")->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-left")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-compare-right")->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "as-convert-operand")->to_ui64() == UINT32_C(1));
}

