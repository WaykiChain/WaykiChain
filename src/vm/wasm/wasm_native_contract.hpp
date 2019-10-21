#pragma once

#include "wasm/wasm_context.hpp"

using namespace std;
using namespace wasm;

namespace wasm {
    //class CWasmContext;

    void wasm_native_setcode( wasm_context & );
    void wasm_native_transfer( wasm_context & );

};