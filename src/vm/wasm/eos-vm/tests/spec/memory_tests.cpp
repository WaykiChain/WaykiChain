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

BACKEND_TEST_CASE( "Testing wasm <memory_0_wasm>", "[memory_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <memory_1_wasm>", "[memory_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <memory_10_wasm>", "[memory_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_11_wasm>", "[memory_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_12_wasm>", "[memory_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_13_wasm>", "[memory_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.13.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_14_wasm>", "[memory_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.14.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_15_wasm>", "[memory_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.15.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_16_wasm>", "[memory_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.16.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_17_wasm>", "[memory_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.17.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_18_wasm>", "[memory_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.18.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_19_wasm>", "[memory_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.19.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_2_wasm>", "[memory_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <memory_20_wasm>", "[memory_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.20.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_21_wasm>", "[memory_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.21.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_22_wasm>", "[memory_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.22.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_23_wasm>", "[memory_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.23.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_24_wasm>", "[memory_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.24.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_25_wasm>", "[memory_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.25.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "data")->to_ui32() == UINT32_C(1));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "cast")->to_f64()) == UINT64_C(4631107791820423168));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_s", UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_u", UINT32_C(4294967295))->to_ui32() == UINT32_C(255));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(4294967295))->to_ui32() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(4294967295))->to_ui32() == UINT32_C(65535));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_s", UINT32_C(100))->to_ui32() == UINT32_C(100));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_u", UINT32_C(200))->to_ui32() == UINT32_C(200));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(20000))->to_ui32() == UINT32_C(20000));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(40000))->to_ui32() == UINT32_C(40000));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_s", UINT32_C(4275856707))->to_ui32() == UINT32_C(67));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_s", UINT32_C(878104047))->to_ui32() == UINT32_C(4294967279));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_u", UINT32_C(4275856707))->to_ui32() == UINT32_C(67));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load8_u", UINT32_C(878104047))->to_ui32() == UINT32_C(239));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(4275856707))->to_ui32() == UINT32_C(25923));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_s", UINT32_C(878104047))->to_ui32() == UINT32_C(4294954479));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(4275856707))->to_ui32() == UINT32_C(25923));
   CHECK(bkend.call_with_return(nullptr, "env", "i32_load16_u", UINT32_C(878104047))->to_ui32() == UINT32_C(52719));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_s", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_u", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(255));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(65535));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(18446744073709551615));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(18446744073709551615))->to_ui64() == UINT32_C(4294967295));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_s", UINT64_C(100))->to_ui64() == UINT32_C(100));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_u", UINT64_C(200))->to_ui64() == UINT32_C(200));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(20000))->to_ui64() == UINT32_C(20000));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(40000))->to_ui64() == UINT32_C(40000));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(20000))->to_ui64() == UINT32_C(20000));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(40000))->to_ui64() == UINT32_C(40000));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_s", UINT64_C(18364758543954109763))->to_ui64() == UINT32_C(67));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_s", UINT64_C(3771275841602506223))->to_ui64() == UINT32_C(18446744073709551599));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_u", UINT64_C(18364758543954109763))->to_ui64() == UINT32_C(67));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load8_u", UINT64_C(3771275841602506223))->to_ui64() == UINT32_C(239));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(18364758543954109763))->to_ui64() == UINT32_C(25923));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_s", UINT64_C(3771275841602506223))->to_ui64() == UINT32_C(18446744073709538799));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(18364758543954109763))->to_ui64() == UINT32_C(25923));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load16_u", UINT64_C(3771275841602506223))->to_ui64() == UINT32_C(52719));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(18364758543954109763))->to_ui64() == UINT32_C(1446274371));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_s", UINT64_C(3771275841602506223))->to_ui64() == UINT32_C(18446744071976963567));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(18364758543954109763))->to_ui64() == UINT32_C(1446274371));
   CHECK(bkend.call_with_return(nullptr, "env", "i64_load32_u", UINT64_C(3771275841602506223))->to_ui64() == UINT32_C(2562379247));
}

BACKEND_TEST_CASE( "Testing wasm <memory_3_wasm>", "[memory_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <memory_4_wasm>", "[memory_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_5_wasm>", "[memory_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <memory_6_wasm>", "[memory_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.6.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "memsize")->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <memory_7_wasm>", "[memory_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.7.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "memsize")->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <memory_8_wasm>", "[memory_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.8.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "memsize")->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <memory_9_wasm>", "[memory_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "memory.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

