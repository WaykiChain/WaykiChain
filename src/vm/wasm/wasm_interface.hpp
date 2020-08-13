#pragma once

#include <vector>
#include <map>
#include "wasm/wasm_context_interface.hpp"
#include "wasm/wasm_runtime.hpp"

void wasm_code_cache_free();

namespace wasm {

    enum class vm_type {
        eos_vm,
        eos_vm_jit
    };

    class wasm_interface {

    public:

        wasm_interface();
        ~wasm_interface();

    public:
        void initialize(vm_type vm);
        int64_t execute(const vector <uint8_t>& code, const uint256 &hash, wasm_context_interface *pWasmContext);
        void validate(const vector <uint8_t>& code);
        void exit();

    };
}
