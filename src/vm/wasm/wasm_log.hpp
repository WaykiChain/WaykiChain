#pragma once

#include <cstdint>
#include <string>

using namespace std;

#define WASM_LOG_BUFFER_LENGTH 1024
#define WASM_FUNCTION_PRINT_LENGTH 64

#define WASM_TRACE( ... ) {                  \
   char buf[WASM_LOG_BUFFER_LENGTH];         \
   sprintf( buf,  __VA_ARGS__ );             \
   std::ostringstream o;                     \
   o << __FILE__ << ":" << __LINE__          \
             << ":[" << __FUNCTION__ << "]";  \
   while (o.str().size() < WASM_FUNCTION_PRINT_LENGTH) \
        o << " ";                             \
    std::cout << o.str() << buf << std::endl;} 
             

