#pragma once
#include "wasm/abi_def.hpp"

namespace wasm {

    static inline abi_def wasmio_contract_abi() {

        abi_def wasmio_abi;

        if( wasmio_abi.version.size() == 0 ) {
            wasmio_abi.version = "wasm::abi/1.0";
        }

        wasmio_abi.structs.emplace_back(struct_def{
                "setcode", "", {
                        {"account", "name"},
                        {"code", "bytes"},
                        {"abi", "bytes"},
                        {"memo", "string"}
                }
        });
        wasmio_abi.structs.emplace_back(struct_def{
                "transfer", "", {
                        {"from", "name"},
                        {"to", "name"},
                        {"quantity", "asset"},
                        {"memo", "string"}
                }
        });


        wasmio_abi.actions.push_back(action_def{"setcode", "setcode", ""});
        wasmio_abi.actions.push_back(action_def{"transfer", "transfer", ""});

        return wasmio_abi;

    }

    static inline bool get_native_contract_abi(uint64_t contract, std::vector<char>& abi){

        if(wasm::wasmio == contract ) {
            wasm::abi_def wasmio_abi = wasmio_contract_abi();
            abi = wasm::pack<wasm::abi_def>(wasmio_abi);
            return true;
        } else if(wasm::wasmio_bank == contract){
            wasm::abi_def wasmio_bank_abi = wasmio_contract_abi();
            abi = wasm::pack<wasm::abi_def>(wasmio_bank_abi);
            return true;
        }

        return false;

    }


}