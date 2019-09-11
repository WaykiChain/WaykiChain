#pragma once

#include "wasm/wasmcontext.hpp"

using namespace std;
using namespace wasm;

namespace wasm {
    //class CWasmContext;

    void WasmNativeSetcode( CWasmContext & );
    void WasmNativeTransfer( CWasmContext & );

};