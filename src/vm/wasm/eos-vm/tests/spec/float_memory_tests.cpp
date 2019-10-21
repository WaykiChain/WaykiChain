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

BACKEND_TEST_CASE( "Testing wasm <float_memory_0_wasm>", "[float_memory_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_memory.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2141192192));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(0));
bkend(nullptr, "env", "f32.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2141192192));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(0));
bkend(nullptr, "env", "i32.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2141192192));
}

BACKEND_TEST_CASE( "Testing wasm <float_memory_1_wasm>", "[float_memory_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_memory.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9219994337134247936));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(0));
bkend(nullptr, "env", "f64.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9219994337134247936));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(0));
bkend(nullptr, "env", "i64.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9219994337134247936));
}

BACKEND_TEST_CASE( "Testing wasm <float_memory_2_wasm>", "[float_memory_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_memory.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2141192192));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(0));
bkend(nullptr, "env", "f32.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2141192192));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(0));
bkend(nullptr, "env", "i32.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2141192192));
}

BACKEND_TEST_CASE( "Testing wasm <float_memory_3_wasm>", "[float_memory_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_memory.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9219994337134247936));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(0));
bkend(nullptr, "env", "f64.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9219994337134247936));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(0));
bkend(nullptr, "env", "i64.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9219994337134247936));
}

BACKEND_TEST_CASE( "Testing wasm <float_memory_4_wasm>", "[float_memory_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_memory.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2144337921));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2144337921));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(0));
bkend(nullptr, "env", "f32.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2144337921));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2144337921));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(0));
bkend(nullptr, "env", "i32.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i32.load")->to_ui32() == UINT32_C(2144337921));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load")->to_f32()) == UINT32_C(2144337921));
}

BACKEND_TEST_CASE( "Testing wasm <float_memory_5_wasm>", "[float_memory_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_memory.5.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9222246136947933185));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9222246136947933185));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(0));
bkend(nullptr, "env", "f64.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9222246136947933185));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9222246136947933185));
bkend(nullptr, "env", "reset");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(0));
bkend(nullptr, "env", "i64.store");
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load")->to_ui64() == UINT32_C(9222246136947933185));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load")->to_f64()) == UINT64_C(9222246136947933185));
}

