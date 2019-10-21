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

BACKEND_TEST_CASE( "Testing wasm <left-to-right_0_wasm>", "[left-to-right_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "left-to-right.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32_add")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_add")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_sub")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_sub")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_mul")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_mul")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_div_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_div_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_div_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_div_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_rem_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_rem_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_rem_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_rem_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_and")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_and")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_or")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_or")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_xor")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_xor")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_shl")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_shl")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_shr_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_shr_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_shr_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_shr_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_eq")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_eq")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_ne")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_ne")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_lt_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_lt_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_le_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_le_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_lt_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_lt_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_le_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_le_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_gt_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_gt_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_ge_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_ge_s")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_gt_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_gt_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_ge_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_ge_u")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store8")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store8")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_store16")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store16")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_store32")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_call")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_call")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_call_indirect")->to_ui32() == UINT32_C(66052));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_call_indirect")->to_ui32() == UINT32_C(66052));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_select")->to_ui32() == UINT32_C(66053));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_select")->to_ui32() == UINT32_C(66053));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_add")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_add")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_sub")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_sub")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_mul")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_mul")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_div")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_div")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_copysign")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_copysign")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_eq")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_eq")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_ne")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_ne")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_lt")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_lt")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_le")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_le")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_gt")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_gt")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_ge")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_ge")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_min")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_min")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_max")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_max")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_store")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_store")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_call")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_call")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_call_indirect")->to_ui32() == UINT32_C(66052));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_call_indirect")->to_ui32() == UINT32_C(66052));
   CHECK(bkend.call_with_return(nullptr, "env", "f32_select")->to_ui32() == UINT32_C(66053));
   CHECK(bkend.call_with_return(nullptr, "env", "f64_select")->to_ui32() == UINT32_C(66053));
   CHECK(bkend.call_with_return(nullptr, "env", "br_if")->to_ui32() == UINT32_C(258));
   CHECK(bkend.call_with_return(nullptr, "env", "br_table")->to_ui32() == UINT32_C(258));
}

