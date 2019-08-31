#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/test/framework.hpp>

#include <eosio/wasm_backend/memory_manager.hpp>
#include <eosio/wasm_backend/vector.hpp>

using namespace eosio;
using namespace eosio::wasm_backend;

BOOST_AUTO_TEST_SUITE(memory_tests)
/*
BOOST_AUTO_TEST_CASE(allocator_tests) { 
   try {
      {
         bounded_allocator nalloc(5);
         uint32_t* base = nalloc.alloc<uint32_t>(0);
         BOOST_CHECK_EQUAL( base, nalloc.alloc<uint32_t>(0) );
         uint32_t* i1 = nalloc.alloc<uint32_t>();
         BOOST_CHECK_EQUAL( i1, base );
         nalloc.alloc<uint8_t>();
         BOOST_CHECK_THROW( nalloc.alloc<uint8_t>(), wasm_bad_alloc );
      }
      {
         bounded_allocator nalloc(12);
         uint8_t* base = nalloc.alloc<uint8_t>(0);
         uint32_t* i1 = nalloc.alloc<uint32_t>();
         *i1 = 0xFEFEFEFE;
         uint32_t* i2 = nalloc.alloc<uint32_t>();
         *i2 = 0x7C7C7C7C;
         uint16_t* i3 = nalloc.alloc<uint16_t>();
         *i3 = 0xBBBB;
         uint8_t* i4 = nalloc.alloc<uint8_t>();
         *i4 = 0xAA;
         BOOST_CHECK_EQUAL( *i1, 0xFEFEFEFE );
         BOOST_CHECK_EQUAL( *i2, 0x7C7C7C7C );
         BOOST_CHECK_EQUAL( *i3, 0xBBBB );
         BOOST_CHECK_EQUAL( *i4, 0xAA );
         nalloc.free();
         BOOST_CHECK_EQUAL(base, nalloc.alloc<uint8_t>(0));
      }
   } FC_LOG_AND_RETHROW() 
}

BOOST_AUTO_TEST_CASE(memory_manager_tests) { 
   try {
      {
         memory_manager::set_memory_limits( 0 );
         auto& nalloc = memory_manager::get_allocator<memory_manager::types::growable>();
         BOOST_CHECK_THROW( nalloc.alloc<uint16_t>(1), wasm_bad_alloc );
      }
   } FC_LOG_AND_RETHROW() 
}

BOOST_AUTO_TEST_CASE(wasm_allocator_tests) { 
   try {
      memory_manager::set_memory_limits( 1 );
      auto& walloc = memory_manager::get_allocator<memory_manager::types::wasm>();
      constexpr uint64_t max_pages = constants::max_pages;
      uint8_t* p = walloc.alloc<uint8_t>(max_pages);
      for (int i=0; i < constants::max_useable_memory; i++) {
         p[i] = 3;
      }
      for (int i=0; i < constants::max_useable_memory; i++) {
         BOOST_CHECK_EQUAL(p[i], 3);
      }

      BOOST_CHECK_THROW(walloc.alloc<uint8_t>(1), wasm_bad_alloc);

      walloc.reset();
      walloc.alloc<uint8_t>(max_pages);
      BOOST_CHECK_EQUAL(walloc.get_current_page(), constants::max_pages);
      BOOST_CHECK_THROW(walloc.alloc<uint8_t>(1), wasm_bad_alloc);

      walloc.reset();
      BOOST_CHECK_THROW(walloc.alloc<uint8_t>(max_pages+1), wasm_bad_alloc);

      walloc.reset();
      for (int i=0; i < max_pages; i++) {
         walloc.alloc<uint8_t>(1);
      }
      BOOST_CHECK_EQUAL(walloc.get_current_page(), constants::max_pages);
      BOOST_CHECK_THROW(walloc.alloc<uint64_t>(1), wasm_bad_alloc);

      walloc.reset();
      *(p+constants::page_size-1) = 3;
      BOOST_CHECK_THROW(*(p+constants::page_size) = 3, wasm_memory_exception);
      walloc.reset();
      walloc.alloc<uint8_t>(1);
      *(p+constants::page_size) = 3;
      *(p+(2*constants::page_size)-1) = 3;
      BOOST_CHECK_THROW(*(p+(2*constants::page_size)) = 3, wasm_memory_exception);
   } FC_LOG_AND_RETHROW() 
}

struct test_struct {
   uint32_t i;
   uint64_t l;
   float    f;
   double   d;
};

BOOST_AUTO_TEST_CASE(vector_tests) {
   try {
      {
         memory_manager::set_memory_limits( sizeof(test_struct)*3 );
         managed_vector<test_struct, memory_manager::types::growable> nvec(1);
         nvec.push_back( { 33, 33, 33.0f, 33.0 } );
         BOOST_CHECK( nvec[0].i == 33 && nvec[0].l == 33 && nvec[0].f == 33.0f && nvec[0].d == 33.0 );
         BOOST_CHECK_THROW( nvec.push_back( {0,0,0,0} ), wasm_vector_oob_exception );
         nvec.resize(3);
         nvec.push_back({22, 22, 22.0f, 22.0});
         nvec.push_back({11, 11, 11.0f, 11.0});
         BOOST_CHECK( nvec[0].i == 33 && nvec[0].l == 33 && nvec[0].f == 33.0f && nvec[0].d == 33.0 );
         BOOST_CHECK( nvec[1].i == 22 && nvec[1].l == 22 && nvec[1].f == 22.0f && nvec[1].d == 22.0 );
         BOOST_CHECK( nvec[2].i == 11 && nvec[2].l == 11 && nvec[2].f == 11.0f && nvec[2].d == 11.0 );
         nvec.resize(2);
         BOOST_CHECK_THROW( nvec[2], wasm_vector_oob_exception );

         memory_manager::set_memory_limits( sizeof(test_struct)*3 );
         //BOOST_CHECK_THROW( nvec.resize(4), wasm_bad_alloc );
      }
   } FC_LOG_AND_RETHROW()
}
*/
BOOST_AUTO_TEST_SUITE_END()

