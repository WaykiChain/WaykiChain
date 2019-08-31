#include <eosio/wasm_backend/parser.hpp>
#include <fstream>
#include <iostream>
#include <chrono>

using namespace eosio;
using namespace eosio::wasm_backend;
using namespace std::chrono;


std::vector<uint8_t> read_wasm( const std::string& fname ) {
   std::ifstream wasm_file(fname, std::ios::binary);
   FC_ASSERT( wasm_file.is_open(), "wasm file cannot be found" );
   wasm_file.seekg(0, std::ios::end);
   std::vector<uint8_t> wasm; 
   int len = wasm_file.tellg();
   FC_ASSERT( len >= 0, "wasm file length is -1" );
   wasm.resize(len);
   wasm_file.seekg(0, std::ios::beg);
   wasm_file.read((char*)wasm.data(), wasm.size());
   wasm_file.close();
   return wasm;
}
//extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size ) {
//   binary_parser bp;
//   module mod;
//   memory_manager::set_memory_limits( 128*1024, 64*1024 );
//   wasm_code test;
//   test.resize( size );
//   memcpy( (char*)test.data(), data, size );
//   bp.parse_module( test, mod );
//}

int main( int argc, const char* argv[] ) {
   high_resolution_clock::time_point start;
   try {
      memory_manager::set_memory_limits( 128*1024, 64*1024 );
      binary_parser bp;
      module mod;
      wasm_code test = read_wasm(argv[1]);
      start = high_resolution_clock::now();
      bp.parse_module( test, mod );
   } catch (fc::exception& e) {
      std::cout << "FAIL : " << e.what() <<"\n";
   }
   auto stop  = high_resolution_clock::now();
   auto duration = duration_cast<microseconds>(stop - start);
   std::cout << "TIME : " << duration.count() << "us\n";

#if 0
   catch ( wasm_parse_exception& ex ) {
   } catch ( wasm_interpreter_exception& ex ) {
   } catch ( wasm_section_length_exception& ex ) {
   } catch ( wasm_bad_alloc& ex ) {
   } catch ( wasm_double_free& ex ) {
   } catch ( wasm_vector_oob_exception& ex ) {
   } catch ( wasm_memory_exception& ex ) {
   } catch ( wasm_unsupported_import_exception& ex ) {
   } catch ( wasm_illegal_opcode_exception& ex ) {
   } catch ( guarded_ptr_exception& ex ) {
   }
#endif
}
