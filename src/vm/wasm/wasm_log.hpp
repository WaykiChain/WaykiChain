#pragma once

#include <cstdint>
#include <string>

using namespace std;

#define WASM_LOG_BUFFER_LENGTH 1024

#define WASM_TRACE( ... ) {                  \
   char buf[WASM_LOG_BUFFER_LENGTH];         \
   sprintf( buf,  __VA_ARGS__ );             \
   std::cout << __FILE__ << ":" << __LINE__  \
             << ":[" << __FUNCTION__ << "]    " << buf << std::endl;} 
             

             