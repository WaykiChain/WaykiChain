#pragma once
#include "wasm/abi_def.hpp"
#include "wasm/modules/dex_contract.hpp"

namespace wasm {

    static inline abi_def wasmio_contract_abi() {

        abi_def abi;

        if( abi.version.size() == 0 ) {
            abi.version = "wasm::abi/1.0";
        }

        abi.structs.emplace_back(struct_def{
                "setcode", "", {
                        {"account", "name"},
                        {"code", "bytes"},
                        {"abi", "bytes"},
                        {"memo", "string"}
                }
        });

        abi.actions.push_back(action_def{"setcode", "setcode", ""});
        return abi;

    }

    static inline abi_def wasmio_bank_contract_abi() {

        abi_def abi;

        if( abi.version.size() == 0 ) {
            abi.version = "wasm::abi/1.0";
        }

        abi.structs.emplace_back(struct_def{
                "transfer", "", {
                        {"from", "name"},
                        {"to", "name"},
                        {"quantity", "asset"},
                        {"memo", "string"}
                }
        });

        abi.actions.push_back(action_def{"transfer", "transfer", ""});
        return abi;

    }

    static inline bool get_native_contract_abi(uint64_t contract, std::vector<char>& abi){

        if(wasm::wasmio == contract ) {
            wasm::abi_def wasm_abi = wasmio_contract_abi();
            abi = wasm::pack<wasm::abi_def>(wasm_abi);
            return true;
        } else if (wasm::wasmio_bank == contract){
            wasm::abi_def wasm_abi = wasmio_bank_contract_abi();
            abi = wasm::pack<wasm::abi_def>(wasm_abi);
            return true;
        }

        return false;

    }


}