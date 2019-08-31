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

TEST_CASE("Testing wasm <call_0_wasm>", "[call_0_wasm_tests]") {
   auto code = backend_t::read_wasm(call_0_wasm);
   wa.reset();
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-i32")) == static_cast<uint32_t>(306));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-i64")) == static_cast<uint64_t>(356));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32")) == static_cast<float>(3890.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64")) == static_cast<double>(3940.0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-first-i32")) == static_cast<uint32_t>(32));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-first-i64")) == static_cast<uint64_t>(64));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-first-f32")) == static_cast<float>(1.32f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-first-f64")) == static_cast<double>(1.64));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "type-second-i32")) == static_cast<uint32_t>(32));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "type-second-i64")) == static_cast<uint64_t>(64));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-second-f32")) == static_cast<float>(32.0f));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-second-f64")) == static_cast<double>(64.1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac", static_cast<uint64_t>(0))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac", static_cast<uint64_t>(1))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac", static_cast<uint64_t>(5))) ==
         static_cast<uint64_t>(120));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac", static_cast<uint64_t>(25))) ==
         static_cast<uint64_t>(7034535277573963776));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-acc", static_cast<uint64_t>(0),
                                        static_cast<uint64_t>(1))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-acc", static_cast<uint64_t>(1),
                                        static_cast<uint64_t>(1))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-acc", static_cast<uint64_t>(5),
                                        static_cast<uint64_t>(1))) == static_cast<uint64_t>(120));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fac-acc", static_cast<uint64_t>(25),
                                        static_cast<uint64_t>(1))) == static_cast<uint64_t>(7034535277573963776));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib", static_cast<uint64_t>(0))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib", static_cast<uint64_t>(1))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib", static_cast<uint64_t>(2))) == static_cast<uint64_t>(2));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib", static_cast<uint64_t>(5))) == static_cast<uint64_t>(8));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "fib", static_cast<uint64_t>(20))) ==
         static_cast<uint64_t>(10946));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint64_t>(0))) ==
         static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint64_t>(1))) ==
         static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint64_t>(100))) ==
         static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "even", static_cast<uint64_t>(77))) ==
         static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint64_t>(0))) == static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint64_t>(1))) == static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint64_t>(200))) ==
         static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "odd", static_cast<uint64_t>(77))) ==
         static_cast<uint32_t>(44));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-if-condition")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == static_cast<uint32_t>(2));
   CHECK_THROWS_AS(bkend(nullptr, "env", "as-call_indirect-last"), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == static_cast<uint32_t>(306));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-local.set-value")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-local.tee-value")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-global.set-value")) == static_cast<uint32_t>(306));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == static_cast<uint32_t>(1));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == static_cast<float>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == static_cast<uint32_t>(11));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == static_cast<uint32_t>(9));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == static_cast<uint32_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "as-convert-operand")) == static_cast<uint64_t>(1));
}
