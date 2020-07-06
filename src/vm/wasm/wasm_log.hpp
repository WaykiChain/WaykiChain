#pragma once

#include "logging.h"

// #include <cstdint>
// #include <string>

// using namespace std;

// #define WASM_FUNCTION_PRINT_LENGTH 64

// #define WASM_TRACE( ... ) {                  \
//    string msg = tfm::format( __VA_ARGS__ );  \
//    std::ostringstream o;                     \
//    o << __FILE__ << ":" << __LINE__          \
//              << ":[" << __FUNCTION__ << "]"; \
//    while (o.str().size() < WASM_FUNCTION_PRINT_LENGTH) \
//         o << " ";                             \
//     std::cout << o.str() << msg << std::endl;}

#define WASM_TRACE( ... ) LogPrint(BCLog::WASM, __VA_ARGS__);
