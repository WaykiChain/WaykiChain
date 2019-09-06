#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <iostream>
#include <fstream>
#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;

TEST_CASE( "Allocate until failure", "[alloc_fail]" ) {
   growable_allocator ga(1); // allocate 1 page
   int* base = ga.alloc<int>(0);
   int* p1, *p2;
   size_t amt1, amt2;

   auto equals = [&](auto* p1, auto* p2, size_t off) { 
      return (uintptr_t)p1 - (uintptr_t)p2-off; 
   };

   auto alloc = [&](size_t amt) { return (int*)ga.alloc<uint8_t>(amt); };
   auto is_aligned = [](auto* p) { return ((uintptr_t)p) % 16 == 0; };

   p1 = alloc(15);

   // base should == the first allocated memory
   REQUIRE(base == p1);
   
   // should be 16 byte aligned
   REQUIRE(is_aligned(p1));

   p2 = alloc(1);
   REQUIRE(is_aligned(p2));
   REQUIRE(equals(p1, p2, 16));

   p1 = alloc(1);
   REQUIRE(is_aligned(p1));
   REQUIRE(equals(p2, p1, 16));
}
