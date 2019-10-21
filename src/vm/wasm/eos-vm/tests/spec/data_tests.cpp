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

BACKEND_TEST_CASE( "Testing wasm <data_0_wasm>", "[data_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_1_wasm>", "[data_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_11_wasm>", "[data_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.11.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_12_wasm>", "[data_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.12.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_14_wasm>", "[data_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.14.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_15_wasm>", "[data_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.15.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_16_wasm>", "[data_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.16.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_18_wasm>", "[data_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.18.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_25_wasm>", "[data_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.25.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_26_wasm>", "[data_26_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.26.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_27_wasm>", "[data_27_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.27.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_28_wasm>", "[data_28_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.28.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_29_wasm>", "[data_29_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.29.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_3_wasm>", "[data_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <data_31_wasm>", "[data_31_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.31.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_33_wasm>", "[data_33_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.33.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_34_wasm>", "[data_34_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.34.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_35_wasm>", "[data_35_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.35.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_37_wasm>", "[data_37_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.37.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   CHECK_THROWS_AS(bkend.initialize(nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_39_wasm>", "[data_39_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.39.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_40_wasm>", "[data_40_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.40.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_41_wasm>", "[data_41_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.41.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_42_wasm>", "[data_42_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.42.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_43_wasm>", "[data_43_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.43.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_44_wasm>", "[data_44_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.44.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <data_9_wasm>", "[data_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "data.9.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

