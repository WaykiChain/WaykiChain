#pragma once

#include "wasm/wasm_context.hpp"

using namespace std;
using namespace wasm;

namespace wasm {
    //class CWasmContext;

    void WasmNativeSetcode( CWasmContext & );
    void WasmNativeTransfer( CWasmContext & );

};