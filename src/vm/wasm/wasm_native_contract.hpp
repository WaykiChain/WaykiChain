#pragma once

#include "wasm/wasm_context.hpp"

using namespace std;
using namespace wasm;

namespace wasm {
    //class CWasmContext;

    void wasm_native_setcode( CWasmContext & );
    void wasm_native_transfer( CWasmContext & );

};