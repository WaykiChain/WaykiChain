#include <algorithm>
#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <eosio/vm/backend.hpp>
#include <iostream>
#include <iterator>
#include <utils.hpp>
#include <vector>
#include <wasm_config.hpp>

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;
using backend_t = backend<std::nullptr_t>;

TEST_CASE("Testing wasm <call_indirect_0_wasm>", "[call_indirect_0_wasm_tests]") {
   auto      code = backend_t::read_wasm(call_indirect_0_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-i32")) == static_cast<uint32_t>(306));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-i64")) == static_cast<uint64_t>(356));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32")) == static_cast<float>(3890.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64")) == static_cast<double>(3940.0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-index")) == static_cast<uint64_t>(100));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-first-i32")) == static_cast<uint32_t>(32));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-first-i64")) == static_cast<uint64_t>(64));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-first-f32")) == static_cast<float>(1.32f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-first-f64")) == static_cast<double>(1.64));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-second-i32")) == static_cast<uint32_t>(32));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-second-i64")) == static_cast<uint64_t>(64));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-second-f32")) == static_cast<float>(32.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-second-f64")) == static_cast<double>(64.1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch", static_cast<uint32_t>(5),
                                        static_cast<uint64_t>(2))) == static_cast<uint64_t>(2));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch", static_cast<uint32_t>(5),
                                        static_cast<uint64_t>(5))) == static_cast<uint64_t>(5));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch", static_cast<uint32_t>(12),
                                        static_cast<uint64_t>(5))) == static_cast<uint64_t>(120));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch", static_cast<uint32_t>(13),
                                        static_cast<uint64_t>(5))) == static_cast<uint64_t>(8));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch", static_cast<uint32_t>(20),
                                        static_cast<uint64_t>(2))) == static_cast<uint64_t>(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", static_cast<uint32_t>(0), static_cast<uint64_t>(2)),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", static_cast<uint32_t>(15), static_cast<uint64_t>(2)),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", static_cast<uint32_t>(29), static_cast<uint64_t>(2)),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", static_cast<uint32_t>(4294967295), static_cast<uint64_t>(2)),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch", static_cast<uint32_t>(1213432423), static_cast<uint64_t>(2)),
                   std::exception);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", static_cast<uint32_t>(5))) ==
         static_cast<uint64_t>(9));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", static_cast<uint32_t>(12))) ==
         static_cast<uint64_t>(362880));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", static_cast<uint32_t>(13))) ==
         static_cast<uint64_t>(55));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", static_cast<uint32_t>(20))) ==
         static_cast<uint64_t>(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i64", static_cast<uint32_t>(11)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i64", static_cast<uint32_t>(22)), std::exception);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", static_cast<uint32_t>(4))) ==
         static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", static_cast<uint32_t>(23))) ==
         static_cast<uint32_t>(362880));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", static_cast<uint32_t>(26))) ==
         static_cast<uint32_t>(55));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", static_cast<uint32_t>(19))) ==
         static_cast<uint32_t>(9));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i32", static_cast<uint32_t>(9)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-i32", static_cast<uint32_t>(21)), std::exception);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", static_cast<uint32_t>(6))) ==
         static_cast<float>(9.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", static_cast<uint32_t>(24))) ==
         static_cast<float>(362880.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", static_cast<uint32_t>(27))) ==
         static_cast<float>(55.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", static_cast<uint32_t>(21))) ==
         static_cast<float>(9.0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f32", static_cast<uint32_t>(8)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f32", static_cast<uint32_t>(19)), std::exception);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", static_cast<uint32_t>(7))) ==
         static_cast<double>(9.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", static_cast<uint32_t>(25))) ==
         static_cast<double>(362880.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", static_cast<uint32_t>(28))) ==
         static_cast<double>(55.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", static_cast<uint32_t>(22))) ==
         static_cast<double>(9.0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f64", static_cast<uint32_t>(10)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "dispatch-structural-f64", static_cast<uint32_t>(18)), std::exception);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-i64", static_cast<uint64_t>(0))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-i64", static_cast<uint64_t>(1))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-i64", static_cast<uint64_t>(5))) ==
         static_cast<uint64_t>(120));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-i64", static_cast<uint64_t>(25))) ==
         static_cast<uint64_t>(7034535277573963776));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fac-i32", static_cast<uint32_t>(0))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fac-i32", static_cast<uint32_t>(1))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fac-i32", static_cast<uint32_t>(5))) ==
         static_cast<uint32_t>(120));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fac-i32", static_cast<uint32_t>(10))) ==
         static_cast<uint32_t>(3628800));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", static_cast<float>(0))) == static_cast<float>(1.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", static_cast<float>(1.0f))) ==
         static_cast<float>(1.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", static_cast<float>(5.0f))) ==
         static_cast<float>(120.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", static_cast<float>(10.0f))) ==
         static_cast<float>(3628800.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", static_cast<double>(0))) ==
         static_cast<double>(1.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", static_cast<double>(1.0))) ==
         static_cast<double>(1.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", static_cast<double>(5.0))) ==
         static_cast<double>(120.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", static_cast<double>(10.0))) ==
         static_cast<double>(3628800.0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib-i64", static_cast<uint64_t>(0))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib-i64", static_cast<uint64_t>(1))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib-i64", static_cast<uint64_t>(2))) ==
         static_cast<uint64_t>(2));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib-i64", static_cast<uint64_t>(5))) ==
         static_cast<uint64_t>(8));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib-i64", static_cast<uint64_t>(20))) ==
         static_cast<uint64_t>(10946));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fib-i32", static_cast<uint32_t>(0))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fib-i32", static_cast<uint32_t>(1))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fib-i32", static_cast<uint32_t>(2))) ==
         static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fib-i32", static_cast<uint32_t>(5))) ==
         static_cast<uint32_t>(8));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "fib-i32", static_cast<uint32_t>(20))) ==
         static_cast<uint32_t>(10946));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", static_cast<float>(0))) == static_cast<float>(1.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", static_cast<float>(1.0f))) ==
         static_cast<float>(1.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", static_cast<float>(2.0f))) ==
         static_cast<float>(2.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", static_cast<float>(5.0f))) ==
         static_cast<float>(8.0f));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", static_cast<float>(20.0f))) ==
         static_cast<float>(10946.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", static_cast<double>(0))) ==
         static_cast<double>(1.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", static_cast<double>(1.0))) ==
         static_cast<double>(1.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", static_cast<double>(2.0))) ==
         static_cast<double>(2.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", static_cast<double>(5.0))) ==
         static_cast<double>(8.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", static_cast<double>(20.0))) ==
         static_cast<double>(10946.0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint32_t>(0))) ==
         static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint32_t>(1))) ==
         static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint32_t>(100))) ==
         static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint32_t>(77))) ==
         static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint32_t>(0))) == static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint32_t>(1))) == static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint32_t>(200))) ==
         static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint32_t>(77))) ==
         static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-condition")) == static_cast<uint32_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == static_cast<uint64_t>(356));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == static_cast<uint32_t>(2));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == static_cast<float>(3890.0f));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == static_cast<uint32_t>(2));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == static_cast<uint32_t>(1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == static_cast<float>(1.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-local.set-value")) == static_cast<double>(1.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-local.tee-value")) == static_cast<double>(1.0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-global.set-value")) == static_cast<double>(1.0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == static_cast<uint32_t>(1));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == static_cast<float>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == static_cast<uint32_t>(11));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == static_cast<uint32_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "as-convert-operand")) == static_cast<uint64_t>(1));
}
