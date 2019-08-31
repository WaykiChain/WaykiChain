#include <catch2/catch.hpp>
#include <eosio/vm/backend.hpp>
#include <wasm_config.hpp>

extern eosio::vm::wasm_allocator wa;

using backend_t = eosio::vm::backend<nullptr_t>;
using namespace eosio::vm;

TEST_CASE( "Testing wasm <address_0_wasm>", "[address_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( address_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good1", static_cast<uint32_t>(0))) == static_cast<uint32_t>(97));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good2", static_cast<uint32_t>(0))) == static_cast<uint32_t>(97));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good3", static_cast<uint32_t>(0))) == static_cast<uint32_t>(98));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good4", static_cast<uint32_t>(0))) == static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good5", static_cast<uint32_t>(0))) == static_cast<uint32_t>(122));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good1", static_cast<uint32_t>(0))) == static_cast<uint32_t>(97));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good2", static_cast<uint32_t>(0))) == static_cast<uint32_t>(97));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good3", static_cast<uint32_t>(0))) == static_cast<uint32_t>(98));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good4", static_cast<uint32_t>(0))) == static_cast<uint32_t>(99));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good5", static_cast<uint32_t>(0))) == static_cast<uint32_t>(122));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good1", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25185));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good2", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25185));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good3", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25442));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good4", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25699));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good5", static_cast<uint32_t>(0))) == static_cast<uint32_t>(122));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good1", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25185));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good2", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25185));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good3", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25442));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good4", static_cast<uint32_t>(0))) == static_cast<uint32_t>(25699));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good5", static_cast<uint32_t>(0))) == static_cast<uint32_t>(122));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good1", static_cast<uint32_t>(0))) == static_cast<uint32_t>(1684234849));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good2", static_cast<uint32_t>(0))) == static_cast<uint32_t>(1684234849));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good3", static_cast<uint32_t>(0))) == static_cast<uint32_t>(1701077858));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good4", static_cast<uint32_t>(0))) == static_cast<uint32_t>(1717920867));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good5", static_cast<uint32_t>(0))) == static_cast<uint32_t>(122));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good1", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good2", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good3", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good4", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good5", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good1", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good2", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good3", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good4", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good5", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good1", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good2", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good3", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good4", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good5", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good1", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good2", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good3", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good4", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good5", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good1", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good2", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good3", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good4", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good5", static_cast<uint32_t>(65507))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good1", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good2", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good3", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good4", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8u_good5", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good1", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good2", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good3", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good4", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "8s_good5", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good1", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good2", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good3", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good4", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16u_good5", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good1", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good2", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good3", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good4", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "16s_good5", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good1", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good2", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good3", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "32_good4", static_cast<uint32_t>(65508))) == static_cast<uint32_t>(0));

   if constexpr (should_align_memory_ops)
      CHECK_THROWS_AS(bkend(nullptr, "env", "32_good5", static_cast<uint32_t>(65508)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", static_cast<uint32_t>(1)), std::exception);
}

TEST_CASE( "Testing wasm <address_2_wasm>", "[address_2_wasm_tests]" ) {
   auto code = backend_t::read_wasm( address_2_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(97));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(97));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(98));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(99));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(97));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(97));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(98));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(99));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25185));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25185));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25442));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25699));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25185));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25185));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25442));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(25699));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1684234849));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1684234849));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1701077858));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1717920867));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1684234849));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1684234849));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1701077858));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(1717920867));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good1", static_cast<uint32_t>(0))) == static_cast<uint64_t>(7523094288207667809));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good2", static_cast<uint32_t>(0))) == static_cast<uint64_t>(7523094288207667809));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good3", static_cast<uint32_t>(0))) == static_cast<uint64_t>(7595434461045744482));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good4", static_cast<uint32_t>(0))) == static_cast<uint64_t>(7667774633883821155));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(122));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good1", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good2", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good3", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good4", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good5", static_cast<uint32_t>(65503))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8u_good5", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "8s_good5", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16u_good5", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "16s_good5", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32u_good5", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "32s_good5", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good1", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good2", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good3", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "64_good4", static_cast<uint32_t>(65504))) == static_cast<uint64_t>(0));

   if constexpr (should_align_memory_ops)
      CHECK_THROWS_AS(bkend(nullptr, "env", "64_good5", static_cast<uint32_t>(65504)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32u_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32s_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8u_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "8s_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16u_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "16s_bad", static_cast<uint32_t>(1)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32u_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32s_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", static_cast<uint32_t>(1)), std::exception);
}

TEST_CASE( "Testing wasm <address_3_wasm>", "[address_3_wasm_tests]" ) {
   auto code = backend_t::read_wasm( address_3_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good1", static_cast<uint32_t>(0))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good2", static_cast<uint32_t>(0))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good3", static_cast<uint32_t>(0))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good4", static_cast<uint32_t>(0))) == static_cast<float>(0));
   CHECK(to_fui32(*bkend.call_with_return(nullptr, "env", "32_good5", static_cast<uint32_t>(0))) == static_cast<uint32_t>(2144337921));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good1", static_cast<uint32_t>(65524))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good2", static_cast<uint32_t>(65524))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good3", static_cast<uint32_t>(65524))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good4", static_cast<uint32_t>(65524))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good5", static_cast<uint32_t>(65524))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good1", static_cast<uint32_t>(65525))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good2", static_cast<uint32_t>(65525))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good3", static_cast<uint32_t>(65525))) == static_cast<float>(0));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good4", static_cast<uint32_t>(65525))) == static_cast<float>(0));

   if constexpr (should_align_memory_ops)
      CHECK_THROWS_AS(bkend(nullptr, "env", "32_good5", static_cast<uint32_t>(65525)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "32_bad", static_cast<uint32_t>(1)), std::exception);
}

TEST_CASE( "Testing wasm <address_4_wasm>", "[address_4_wasm_tests]" ) {
   auto code = backend_t::read_wasm( address_4_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good1", static_cast<uint32_t>(0))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good2", static_cast<uint32_t>(0))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good3", static_cast<uint32_t>(0))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good4", static_cast<uint32_t>(0))) == static_cast<double>(0));
   CHECK(to_fui64(*bkend.call_with_return(nullptr, "env", "64_good5", static_cast<uint32_t>(0))) == static_cast<uint64_t>(0x7ffc000000000001));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good1", static_cast<uint32_t>(65510))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good2", static_cast<uint32_t>(65510))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good3", static_cast<uint32_t>(65510))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good4", static_cast<uint32_t>(65510))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good5", static_cast<uint32_t>(65510))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good1", static_cast<uint32_t>(65511))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good2", static_cast<uint32_t>(65511))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good3", static_cast<uint32_t>(65511))) == static_cast<double>(0));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good4", static_cast<uint32_t>(65511))) == static_cast<double>(0));

   if constexpr (should_align_memory_ops)
      CHECK_THROWS_AS(bkend(nullptr, "env", "64_good5", static_cast<uint32_t>(65511)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", static_cast<uint32_t>(0)), std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "64_bad", static_cast<uint32_t>(1)), std::exception);
}
