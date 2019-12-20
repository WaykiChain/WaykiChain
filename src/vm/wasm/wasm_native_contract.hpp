#pragma once

#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/modules/dex_contract.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

    void wasmio_native_setcode( wasm_context & );
    void wasmio_bank_native_transfer( wasm_context & );

    inline bool is_native_contract(uint64_t contract){
        if(contract == wasmio ||
           contract == wasmio_bank)
            return true;
        return false;
    }

    inline void sub_balance(CAccount& owner, const wasm::asset& quantity, CAccountDBCache &database){

        string symbol     = quantity.sym.code().to_string();
        uint8_t precision = quantity.sym.precision();
        WASM_ASSERT(precision == 8,
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, The precision of system coin %s must be %d",
                    symbol, 8)

        WASM_ASSERT(owner.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount),
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, Operate account %s failed",
                    owner.nickid.ToString().c_str())

        WASM_ASSERT(database.SetAccount(owner.regid, owner), account_operation_exception,
                    "%s","wasmnativecontract.Setcode, Save account error")
    }

    inline void add_balance(CAccount& owner, const wasm::asset& quantity, CAccountDBCache &database){

        string symbol     = quantity.sym.code().to_string();
        uint8_t precision = quantity.sym.precision();
        WASM_ASSERT(precision == 8,
                    account_operation_exception,
                    "wasmnativecontract.add_balance, The precision of system coin %s must be %d",
                    symbol, 8)

        WASM_ASSERT(owner.OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount),
                    account_operation_exception,
                    "wasmnativecontract.add_balance, Operate account %s failed",
                    owner.nickid.ToString().c_str())

        WASM_ASSERT(database.SetAccount(owner.regid, owner), account_operation_exception,
                    "%s","wasmnativecontract.Setcode, Save account error")
    }

};