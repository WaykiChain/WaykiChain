#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>
#include <eosio/vm/watchdog.hpp>

#include <chrono>
#include <iostream>

using namespace eosio;
using namespace eosio::vm;

int main(int argc, char** argv) {
   wasm_allocator wa;
   using backend_t = eosio::vm::backend<nullptr_t>;

   if (argc < 2) {
      std::cerr << "Error, no wasm file provided\n";
      return -1;
   }

   try {

      auto code = backend_t::read_wasm( argv[1] );
	
      auto t1 = std::chrono::high_resolution_clock::now();
      backend_t bkend( code );
      auto t2 = std::chrono::high_resolution_clock::now();
      std::cout << "Startup " << std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count() << "\n";

      bkend.set_wasm_allocator( &wa );

      auto t3 = std::chrono::high_resolution_clock::now();
      bkend.execute_all();
      auto t4 = std::chrono::high_resolution_clock::now();
      std::cout << "Execution " << std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count() << "\n";

   } catch ( ... ) {
      std::cerr << "eos-vm interpreter error\n";
   }
   return 0;
}
