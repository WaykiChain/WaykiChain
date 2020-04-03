#pragma once

#include "entities/receipt.h"
#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"

#include "wasm/modules/wasm_router.hpp"
#include "wasm/modules/wasm_native_bank_module.hpp"
#include "wasm/modules/wasm_native_module.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

   inline std::vector<std::shared_ptr<wasm::native_module>>& get_wasm_native_modules(){
        static std::vector<std::shared_ptr<wasm::native_module>> modules;
        return modules;
    }

    inline action_router& get_wasm_act_route(){
        static action_router router;
        return router;
    }

    inline abi_router& get_wasm_abi_route(){
        static abi_router router;
        return router;
    }

    inline bool get_native_contract_abi(uint64_t contract, std::vector<char>& abi){
        auto& router  = get_wasm_abi_route();
        auto* handler = router.route(contract);
        if(handler == nullptr) return false;
        abi = (*handler)();
        return true; 
    }

    inline bool is_native_contract(uint64_t contract){
        auto& router = get_wasm_abi_route();
        if(router.route(contract) == nullptr) return false;
        return true;

    }

};