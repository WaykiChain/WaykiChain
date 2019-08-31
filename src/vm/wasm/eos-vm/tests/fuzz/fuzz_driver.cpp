#include <eosio/wasm_backend/backend.hpp>

using namespace eosio;
using namespace eosio::wasm_backend;

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size ) {
   wasm_allocator wa;
   wasm_code wc; 
   wc.resize(size);
   memcpy((uint8_t*)wc.data(), data, size);
   backend<std::nullptr_t> bkend( wc );
   bkend.execute_all(nullptr);
}
