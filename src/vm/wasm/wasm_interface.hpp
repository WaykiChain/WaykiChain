#pragma once

#include <vector>
#include <map>
#include "wasm_context_interface.hpp"

namespace wasm {

    enum class vmType {
        eosvm,
        wabt
    };

    class CWasmInterface {

    public:

        CWasmInterface();
        //CWasmInterface(vmType type);
        ~CWasmInterface();

    public:
        void initialize( vmType type );
        void execute( vector <uint8_t> code, CWasmContextInterface *pWasmContext );
        void validate( vector <uint8_t> code );
        void exit() {};

    };
}
