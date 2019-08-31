#include <catch2/catch.hpp>
#include <eosio/vm/backend.hpp>
#include <wasm_config.hpp>

extern eosio::vm::wasm_allocator wa;

using backend_t = eosio::vm::backend<nullptr_t>;
using namespace eosio::vm;

TEST_CASE( "Testing wasm <binary_0_wasm>", "[binary_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_0_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
}

TEST_CASE( "Testing wasm <binary_1_wasm>", "[binary_1_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_1_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
}

TEST_CASE( "Testing wasm <binary_2_wasm>", "[binary_2_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_2_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_32_wasm>", "[binary_32_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_32_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_33_wasm>", "[binary_33_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_33_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_34_wasm>", "[binary_34_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_34_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_35_wasm>", "[binary_35_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_35_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_36_wasm>", "[binary_36_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_36_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_37_wasm>", "[binary_37_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_37_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_38_wasm>", "[binary_38_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_38_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_39_wasm>", "[binary_39_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_39_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_3_wasm>", "[binary_3_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_3_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_40_wasm>", "[binary_40_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_40_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_41_wasm>", "[binary_41_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_41_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_42_wasm>", "[binary_42_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_42_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_43_wasm>", "[binary_43_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_43_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_87_wasm>", "[binary_87_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_87_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_92_wasm>", "[binary_92_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_92_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <binary_93_wasm>", "[binary_93_wasm_tests]" ) {
   auto code = backend_t::read_wasm( binary_93_wasm  );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

