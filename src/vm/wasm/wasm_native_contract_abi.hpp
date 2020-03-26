#pragma once
#include "wasm/abi_def.hpp"

namespace wasm {

//fixme:should be have a mutex protect
#define REGISTER_NATIVE_CONTRACT_ABIS()                              \
        if (get_native_contract_abis().size() == 0) {                \
            register_native_contract_abis(wasm::wasmio, wasmio_contract_abi());            \
            register_native_contract_abis(wasm::wasmio_bank, wasmio_bank_contract_abi());  \
        }

    static inline std::vector<char> wasmio_contract_abi() {

        abi_def abi;

        if (abi.version.size() == 0) {
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

        auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
        return abi_bytes;

    }

    static inline std::vector<char> wasmio_bank_contract_abi() {

        abi_def abi;

        if (abi.version.size() == 0) {
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

        auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
        return abi_bytes;

    }

    inline  map<uint64_t, std::vector<char>>&  get_native_contract_abis(){
        static map<uint64_t, std::vector<char>> native_contract_abis;
        return native_contract_abis;

    }

    inline void register_native_contract_abis(uint64_t contract, const std::vector<char>& abi){
        get_native_contract_abis()[contract] = abi;
    }

    inline bool get_native_contract_abi(uint64_t contract, std::vector<char>& abi){
        //fixme:should be have a mutex protect
        // if (get_native_contract_abis().size() == 0) {
        //     register_native_contract_abis(wasm::wasmio, wasmio_contract_abi());
        //     register_native_contract_abis(wasm::wasmio_bank, wasmio_bank_contract_abi());
        // }

        REGISTER_NATIVE_CONTRACT_ABIS();

        auto ret = get_native_contract_abis().find(contract);
        if (ret != get_native_contract_abis().end()) {
            abi = ret->second;
            return true;
        }

        return false;
    }

    inline bool is_native_contract(uint64_t contract){
        //fixme:should be have a mutex protect
        // if (get_native_contract_abis().size() == 0) {
        //     register_native_contract_abis(wasm::wasmio, wasmio_contract_abi());
        //     register_native_contract_abis(wasm::wasmio_bank, wasmio_bank_contract_abi());
        // }

        REGISTER_NATIVE_CONTRACT_ABIS();

        auto ret = get_native_contract_abis().find(contract);
        if (ret != get_native_contract_abis().end()) {
            return true;
        }
        return false;

    }


}