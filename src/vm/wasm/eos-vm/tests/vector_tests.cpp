#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <fstream>
#include <string>

#include <catch2/catch.hpp>

#include <eosio/vm/vector.hpp>

using namespace eosio;
using namespace eosio::vm;

TEST_CASE( "unmanaged_vector tests", "[unmanaged_vector_tests]") {
   {
      unmanaged_vector<char> uv0(10);
      for (int i = 0; i < uv0.size(); i++)
         uv0[i] = 'a';
      CHECK_THROWS_AS([&](){ uv0.at(10) = 'a'; }(), std::exception);
      uv0.resize(11);
      uv0[10] = 'a';
      CHECK_THROWS_AS([&](){ uv0.at(11) = 'a'; }(), std::exception);
      uv0.resize(8);
      CHECK_THROWS_AS([&](){ uv0.at(8) = 'a'; }(), std::exception);
      size_t uv0_size = uv0.size();
      unmanaged_vector<char> uv1(std::move(uv0));
      CHECK( uv0_size == uv1.size() );
      for (int i=0; i < uv1.size(); i++)
         uv1[i] = 'b';
      uv1.emplace_back('b');
      for (int i=0; i < 8; i++)
         uv1.emplace_back('c');
      uv1.emplace_back('c');
      for (int i=0; i < 16; i++)
         uv1.emplace_back('d');
   }

   {
      unmanaged_vector<char> uv0(10);
      for (int i=0; i < uv0.size(); i++)
         uv0[i] = 'b';
      uv0.push_back('b');
      uv0.push_back('b');
      for (int i=0; i < 8; i++)
         uv0.push_back('c');
      uv0.push_back('c');
      for (int i=0; i < 16; i++)
         uv0.push_back('d');
   }
}

TEST_CASE( "managed_vector tests", "[managed_vector_tests]") {
   bounded_allocator ba(20);
   managed_vector<char, bounded_allocator> v0(ba, 10);
   for (int i = 0; i < v0.size(); i++)
      v0[i] = 'a';
   CHECK_THROWS_AS([&](){ v0[10] = 'a'; }(), std::exception);
   v0.resize(10);
   v0[9] = 'a';
   CHECK_THROWS_AS([&](){ v0[10] = 'a'; }(), std::exception);
   v0.resize(8);
   CHECK_THROWS_AS([&](){ v0[8] = 'a'; }(), std::exception);
   size_t v0_size = v0.size();
   managed_vector<char, bounded_allocator> v1(std::move(v0));
   CHECK( v0_size == v1.size() );
   for (int i=0; i < v1.size(); i++)
      v1[i] = 'b';
   v1.emplace_back('b');
   for (int i=0; i < 7; i++)
      v1.emplace_back('c');
   CHECK_THROWS_AS([&]() {v1.emplace_back('c');}(), std::exception);
}
