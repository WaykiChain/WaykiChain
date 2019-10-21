#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <fstream>
#include <string>

#include <catch2/catch.hpp>

#include <eosio/vm/backend.hpp>
#include "wasm_config.hpp"
#include "utils.hpp"

using namespace eosio;
using namespace eosio::vm;

void host_call() {}

#include "implementation_limits.hpp"

wasm_code implementation_limits_wasm_code{
   implementation_limits_wasm + 0,
   implementation_limits_wasm + sizeof(implementation_limits_wasm)};

BACKEND_TEST_CASE( "Test call depth", "[call_depth]") {
   wasm_allocator wa;
   using backend_t = eosio::vm::backend<nullptr_t, TestType>;
   using rhf_t     = eosio::vm::registered_host_functions<nullptr_t>;
   rhf_t::add<nullptr_t, &host_call, wasm_allocator>("env", "host.call");

   backend_t bkend(implementation_limits_wasm_code);
   bkend.set_wasm_allocator(&wa);
   bkend.initialize(nullptr);

   rhf_t::resolve(bkend.get_module());

   CHECK(!bkend.call_with_return(nullptr, "env", "call", (uint32_t)250));
   CHECK_THROWS_AS(bkend.call(nullptr, "env", "call", (uint32_t)251), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "call.indirect", (uint32_t)250));
   CHECK_THROWS_AS(bkend.call(nullptr, "env", "call.indirect", (uint32_t)251), std::exception);
   // The host call is added to the recursive function, so we have one fewer frames
   CHECK(!bkend.call_with_return(nullptr, "env", "call.host", (uint32_t)249));
   CHECK_THROWS_AS(bkend.call(nullptr, "env", "call.host", (uint32_t)250), std::exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "call.indirect.host", (uint32_t)249));
   CHECK_THROWS_AS(bkend.call(nullptr, "env", "call.indirect.host", (uint32_t)250), std::exception);
}
