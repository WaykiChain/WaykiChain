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

BACKEND_TEST_CASE( "Testing wasm <memory_trap_0_wasm>", "[memory_trap_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_trap.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "store", UINT32_C(4294967292), UINT32_C(42)));
   CHECK(bkend.call_with_return(nullptr, "env", "load", UINT32_C(4294967292))->to_ui32() == UINT32_C(42));
   CHECK_THROWS_AS(bkend(nullptr, "env", "store", UINT32_C(4294967293), UINT32_C(13)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "store", UINT32_C(4294967294), UINT32_C(13)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "store", UINT32_C(4294967295), UINT32_C(13)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "store", UINT32_C(0), UINT32_C(13)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load", UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "store", UINT32_C(2147483648), UINT32_C(13)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "load", UINT32_C(2147483648)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "memory.grow", UINT32_C(65537))->to_ui32() == UINT32_C(4294967295));
}

BACKEND_TEST_CASE( "Testing wasm <memory_trap_1_wasm>", "[memory_trap_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory_trap.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(65536), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(65535), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(65534), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(65533), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(4294967295), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(4294967294), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(4294967293), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store", UINT32_C(4294967292), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65536), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65535), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65534), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65533), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65532), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65531), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65530), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(65529), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967295), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967294), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967293), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967292), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967291), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967290), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967289), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store", UINT32_C(4294967288), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(65536), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(65535), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(65534), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(65533), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(4294967295), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(4294967294), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(4294967293), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.store", UINT32_C(4294967292), bit_cast<float>(UINT32_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65536), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65535), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65534), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65533), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65532), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65531), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65530), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(65529), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967295), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967294), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967293), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967292), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967291), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967290), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967289), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.store", UINT32_C(4294967288), bit_cast<double>(UINT64_C(0))), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store8", UINT32_C(65536), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store8", UINT32_C(4294967295), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store16", UINT32_C(65536), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store16", UINT32_C(65535), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store16", UINT32_C(4294967295), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.store16", UINT32_C(4294967294), UINT32_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store8", UINT32_C(65536), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store8", UINT32_C(4294967295), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store16", UINT32_C(65536), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store16", UINT32_C(65535), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store16", UINT32_C(4294967295), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store16", UINT32_C(4294967294), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(65536), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(65535), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(65534), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(65533), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(4294967295), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(4294967294), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(4294967293), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.store32", UINT32_C(4294967292), UINT64_C(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(65534)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(65533)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load", UINT32_C(4294967292)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65534)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65533)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65532)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65531)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65530)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(65529)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967292)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967291)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967290)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967289)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load", UINT32_C(4294967288)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(65534)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(65533)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f32.load", UINT32_C(4294967292)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65534)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65533)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65532)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65531)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65530)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(65529)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967292)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967291)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967290)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967289)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "f64.load", UINT32_C(4294967288)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load8_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load8_s", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load8_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load8_u", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_s", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_s", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_s", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_u", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_u", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.load16_u", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load8_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load8_s", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load8_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load8_u", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_s", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_s", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_s", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_u", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_u", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load16_u", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(65534)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(65533)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_s", UINT32_C(4294967292)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(65536)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(65535)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(65534)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(65533)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(4294967295)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(4294967294)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(4294967293)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.load32_u", UINT32_C(4294967292)), std::exception);
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load", UINT32_C(65528))->to_ui64() == UINT32_C(7523094288207667809));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.load", UINT32_C(0))->to_ui64() == UINT32_C(7523094288207667809));
}

