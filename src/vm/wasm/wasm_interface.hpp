#pragma once

#include <vector>
#include <map>
#include "wasm_host_methods.hpp"
#include <eosio/vm/backend.hpp>
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
        // std::unique_ptr<backend_t>& get_instantiated_backend(const vector <uint8_t>& code);
        // static std::map<code_version, std::unique_ptr<backend_t>> get_backend_cache(){
        //     static std::map<code_version, std::unique_ptr<backend_t>> _backend_cache;
        //     return _backend_cache;
        // }

    public:
        void Initialize( vmType type );
        void Execute( vector <uint8_t> code, CWasmContextInterface *pWasmContext );
        void validate( vector <uint8_t> code );
        void exit() {};

    };
}
