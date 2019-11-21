#include <eosio/vm/backend.hpp>

#include <catch2/catch.hpp>

using namespace eosio::vm;

#include "reentry.wasm.hpp"
#include "utils.hpp"

BACKEND_TEST_CASE("test reentry", "[reentry]") {

   struct test_runner {
      eosio::vm::backend<test_runner, TestType>& bkend;
      uint32_t test_func_0(uint32_t val) {
         return bkend.call_with_return(this, "env", "bar", val)->to_ui32() + 50;
      }
      uint32_t test_func_1(uint32_t val) {
         return bkend.call_with_return(this, "env", "testbar", val)->to_ui32() + 50;
      }
      void eosio_assert(uint32_t, uint32_t) {}
      void* memset(void*, int, int) { return 0; }
   };

   using rhf_t     = eosio::vm::registered_host_functions<test_runner>;
   using backend_t = eosio::vm::backend<test_runner, TestType>;
   wasm_allocator wa;
   backend_t bkend(reentry_wasm);
   bkend.set_wasm_allocator(&wa);
   bkend.initialize();
   test_runner tr = {bkend};
   
   rhf_t::template add<test_runner, &test_runner::test_func_0, wasm_allocator>("env", "testfunc0");
   rhf_t::template add<test_runner, &test_runner::test_func_1, wasm_allocator>("env", "testfunc1");
   rhf_t::template add<test_runner, &test_runner::eosio_assert, wasm_allocator>("env", "eosio_assert");
   rhf_t::template add<test_runner, &test_runner::memset, wasm_allocator>("env", "memset");
   rhf_t::resolve(bkend.get_module());

   // level 0
   CHECK(bkend.call_with_return(&tr, "env", "foo", (uint32_t)10)->to_ui32() == 52);
   // level 1
   CHECK(bkend.call_with_return(&tr, "env", "testbar", (uint32_t)10)->to_ui32() == 160);
   CHECK(bkend.call_with_return(&tr, "env", "testbaz", (uint32_t)10)->to_ui32() == 513);
}
