#include "wasm/abi_def.h"


namespace wasm {

    abi_def wasmio_contract_abi(const abi_def &abi) {

        abi_def wasmio_abi(abi);
        wasmio_abi.structs.emplace_back(struct_def{
                "setcode", "", {
                        {"account", "name"},
                        {"code", "string"},
                        {"abi", "string"},
                        {"memo", "string"}
                }
        });

        wasmio_abi.actions.push_back(action_def{"setcode", "setcode", ""});

        return wasmio_abi;

    }

    abi_def wasmio_bank_contract_abi(const abi_def &abi) {

        abi_def wasmio_bank_abi(abi);
        wasmio_bank_abi.structs.emplace_back(struct_def{
                "transfer", "", {
                        {"from", "name"},
                        {"to", "name"},
                        {"quantity", "asset"},
                        {"memo", "string"}
                }
        });

        wasmio_bank_abi.actions.push_back(action_def{"transfer", "transfer", ""});

        return wasmio_bank_abi;

    }

}