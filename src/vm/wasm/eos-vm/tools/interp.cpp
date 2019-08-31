#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>
#include <eosio/vm/watchdog.hpp>

#include <iostream>

using namespace eosio;
using namespace eosio::vm;

/**
 * Simple implementation of an interpreter using eos-vm.
 */
int main(int argc, char** argv) {
   // Thread specific `allocator` used for wasm linear memory.
   wasm_allocator wa;
   // Specific the backend with no "host" for host functions.
   using backend_t = eosio::vm::backend<nullptr_t>;

   if (argc < 2) {
      std::cerr << "Error, no wasm file provided\n";
      return -1;
   }

   watchdog<std::chrono::nanoseconds> wd;
   wd.set_duration(std::chrono::seconds(3));
   try {
      // Read the wasm into memory.
      auto code = backend_t::read_wasm( argv[1] );

      // Instaniate a new backend using the wasm provided.
      backend_t bkend( code );
      wd.set_callback([&](){
		      bkend.get_context().exit();
		      });

      // Point the backend to the allocator you want it to use.
      bkend.set_wasm_allocator( &wa );

      // Execute any exported functions provided by the wasm.
      bkend.execute_all(&wd);

   } catch ( ... ) {
      std::cerr << "eos-vm interpreter error\n";
   }
   return 0;
}
