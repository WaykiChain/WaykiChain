#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <fstream>
#include <string>

#include <catch2/catch.hpp>

#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;

// host functions that are C-style functions
// wasm hex
/* Code used to generate test, compile with eosio-cpp v1.6.2 with minor manual edits to remove unneeded imports
 * extern "C" {
      struct state_t { float f; int i; };
      [[eosio::wasm_import]]
      void c_style_host_function_0();
      [[eosio::wasm_import]]
      void c_style_host_function_1(int);
      [[eosio::wasm_import]]
      void c_style_host_function_2(int, int);
      [[eosio::wasm_import]]
      void c_style_host_function_3(int, float);
      [[eosio::wasm_import]]
      void c_style_host_function_4(const state_t&);

      [[eosio::wasm_entry]]
      void apply(unsigned long long a, unsigned long long b, unsigned long long c) {
         if (a == 0)
            c_style_host_function_0(); 
         else if (a == 1)
            c_style_host_function_1((int)b); 
         else if (a == 2)
            c_style_host_function_2((int)b, (int)c); 
         else if (a == 3)
            c_style_host_function_3((int)b, *((int*)&c)); 
         else if (a == 4) {
            state_t s = {*((float*)&c), (int)b};
            c_style_host_function_4(s); 
         }
      }
   } */

#include "host_functions_tests_0.wasm.hpp"
// no return value and no input parameters
int c_style_host_function_state = 0;
struct state_t {
   float f = 0;
   int   i = 0;
};
void c_style_host_function_0() {
   c_style_host_function_state = 1; 
}
void c_style_host_function_1(int s) {
   c_style_host_function_state = s;
}
void c_style_host_function_2(int a, int b) {
   c_style_host_function_state = a+b;
}
void c_style_host_function_3(int a, float b) {
   c_style_host_function_state = a+b;
}
void c_style_host_function_4(const state_t& ss) {
   c_style_host_function_state = ss.i;
}

TEST_CASE( "Test C-style host function system", "[C-style_host_functions_tests]") { 
   wasm_allocator wa;
   using backend_t = eosio::vm::backend<nullptr_t>;
   using rhf_t     = eosio::vm::registered_host_functions<nullptr_t>;
   rhf_t::add<nullptr_t, &c_style_host_function_0, wasm_allocator>("env", "c_style_host_function_0");
   rhf_t::add<nullptr_t, &c_style_host_function_1, wasm_allocator>("env", "c_style_host_function_1");
   rhf_t::add<nullptr_t, &c_style_host_function_2, wasm_allocator>("env", "c_style_host_function_2");
   rhf_t::add<nullptr_t, &c_style_host_function_3, wasm_allocator>("env", "c_style_host_function_3");
   rhf_t::add<nullptr_t, &c_style_host_function_4, wasm_allocator>("env", "c_style_host_function_4");

   backend_t bkend(host_functions_test_0_wasm);
   bkend.set_wasm_allocator(&wa);
   bkend.initialize(nullptr);

   rhf_t::resolve(bkend.get_module());

   bkend.call(nullptr, "env", "apply", (uint64_t)0, (uint64_t)0, (uint64_t)0);
   CHECK(c_style_host_function_state == 1);

   bkend.call(nullptr, "env", "apply", (uint64_t)1, (uint64_t)2, (uint64_t)0);
   CHECK(c_style_host_function_state == 2);

   bkend.call(nullptr, "env", "apply", (uint64_t)2, (uint64_t)1, (uint64_t)2);
   CHECK(c_style_host_function_state == 3);

   float f = 2.4f;
   bkend.call(nullptr, "env", "apply", (uint64_t)3, (uint64_t)2, *(uint64_t*)&f);
   CHECK(c_style_host_function_state == 0x40199980);

   bkend.call(nullptr, "env", "apply", (uint64_t)4, (uint64_t)5, *(uint64_t*)&f);
   CHECK(c_style_host_function_state == 5);
}
