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

TEST_CASE("Testing wasm <elem_0_wasm>", "[elem_0_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_0_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_10_wasm>", "[elem_10_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_10_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

/* Only allowing imports of functions currently
TEST_CASE("Testing wasm <elem_11_wasm>", "[elem_11_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_11_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}
*/

TEST_CASE("Testing wasm <elem_12_wasm>", "[elem_12_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_12_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

/* Only allowing imports of functions currently
TEST_CASE("Testing wasm <elem_13_wasm>", "[elem_13_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_13_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_14_wasm>", "[elem_14_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_14_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_15_wasm>", "[elem_15_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_15_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_16_wasm>", "[elem_16_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_16_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_17_wasm>", "[elem_17_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_17_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_1_wasm>", "[elem_1_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_1_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_2_wasm>", "[elem_2_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_2_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}
*/

TEST_CASE("Testing wasm <elem_36_wasm>", "[elem_36_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_36_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-overwritten")) == static_cast<uint32_t>(66));
}

/* Only allowing imports of functions currently
TEST_CASE("Testing wasm <elem_37_wasm>", "[elem_37_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_37_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-overwritten-element")) == static_cast<uint32_t>(66));
}
*/

TEST_CASE("Testing wasm <elem_38_wasm>", "[elem_38_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_38_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK_THROWS_AS(bkend(nullptr, "env", "call-7"), std::exception);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-8")) == static_cast<uint32_t>(65));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-9")) == static_cast<uint32_t>(66));
}

/* Only allowing imports of functions currently
TEST_CASE("Testing wasm <elem_39_wasm>", "[elem_39_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_39_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-7")) == static_cast<uint32_t>(67));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-8")) == static_cast<uint32_t>(68));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-9")) == static_cast<uint32_t>(66));
}

TEST_CASE("Testing wasm <elem_3_wasm>", "[elem_3_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_3_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_40_wasm>", "[elem_40_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_40_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-7")) == static_cast<uint32_t>(67));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-8")) == static_cast<uint32_t>(69));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-9")) == static_cast<uint32_t>(70));
}

TEST_CASE("Testing wasm <elem_4_wasm>", "[elem_4_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_4_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_5_wasm>", "[elem_5_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_5_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

TEST_CASE("Testing wasm <elem_6_wasm>", "[elem_6_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_6_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}
*/

TEST_CASE("Testing wasm <elem_7_wasm>", "[elem_7_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_7_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-7")) == static_cast<uint32_t>(65));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "call-9")) == static_cast<uint32_t>(66));
}

TEST_CASE("Testing wasm <elem_8_wasm>", "[elem_8_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_8_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}

/*
TEST_CASE("Testing wasm <elem_9_wasm>", "[elem_9_wasm_tests]") {
   auto      code = backend_t::read_wasm(elem_9_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);
}
*/
