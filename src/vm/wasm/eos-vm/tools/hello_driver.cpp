#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>
#include <eosio/vm/host_function.hpp>
#include <eosio/vm/watchdog.hpp>

#include <iostream>

using namespace eosio;
using namespace eosio::vm;

#include "hello.wasm.hpp"

// example of host function as a raw C style function
void eosio_assert(bool test, const char* msg) {
   if (!test) {
      std::cout << msg << std::endl;
      throw 0;
   }
}

void print_num(uint64_t n) { std::cout << "Number : " << n << "\n"; }

struct example_host_methods {
   // example of a host "method"
   void print_name(const char* nm) { std::cout << "Name : " << nm << " " << field << "\n"; }
   // example of another type of host function
   static void* memset(void* ptr, int x, size_t n) { return ::memset(ptr, x, n); }
   std::string  field = "";
};

/**
 * Simple implementation of an interpreter using eos-vm.
 */
int main(int argc, char** argv) {
   if (argc < 4) {
      std::cerr << "Please enter three numbers\n";
      return -1;
   }
   // Thread specific `allocator` used for wasm linear memory.
   wasm_allocator wa;
   // Specific the backend with example_host_methods for host functions.
   using backend_t = eosio::vm::backend<example_host_methods>;
   using rhf_t     = eosio::vm::registered_host_functions<example_host_methods>;

   // register print_num
   rhf_t::add<nullptr_t, &print_num, wasm_allocator>("env", "print_num");
   // register eosio_assert
   rhf_t::add<nullptr_t, &eosio_assert, wasm_allocator>("env", "eosio_assert");
   // register print_name
   rhf_t::add<example_host_methods, &example_host_methods::print_name, wasm_allocator>("env", "print_name");
   // finally register memset
   rhf_t::add<nullptr_t, &example_host_methods::memset, wasm_allocator>("env", "memset");

   watchdog wd{std::chrono::seconds(3)};
   try {
      // Instaniate a new backend using the wasm provided.
      backend_t bkend(hello_wasm);

      // Point the backend to the allocator you want it to use.
      bkend.set_wasm_allocator(&wa);
      bkend.initialize();
      // Resolve the host functions indices.
      rhf_t::resolve(bkend.get_module());

      // Instaniate a "host"
      example_host_methods ehm;
      ehm.field = "testing";
      // Execute apply.
      bkend(&ehm, "env", "apply", (uint64_t)std::atoi(argv[1]), (uint64_t)std::atoi(argv[2]),
            (uint64_t)std::atoi(argv[3]));

   } catch (...) { std::cerr << "eos-vm interpreter error\n"; }
   return 0;
}
