#pragma once

#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"

using namespace std;
using namespace wasm;

namespace wasm {
    //class CWasmContext;

    void wasmio_native_setcode( wasm_context & );
    void wasmio_bank_native_transfer( wasm_context & );

    inline void sub_balance(CAccount& from, const wasm::asset& quantity, CAccountDBCache &database){
        //auto &database    = db.accountCache;

        string symbol     = quantity.sym.code().to_string();
        uint8_t precision = quantity.sym.precision();
        WASM_ASSERT(precision == 0,
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, the precision of system coin %s must be %d",
                    symbol,
                    0) 

        WASM_ASSERT(from.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount),
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, operate account failed ,name=%s",
                    from.nickid.ToString().c_str())  

        WASM_ASSERT(database.SetAccount(from.regid, from), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")     
    }

    inline void add_balance(CAccount& to, const wasm::asset& quantity, CAccountDBCache &database){
        //auto &database    = db.accountCache;

        string symbol     = quantity.sym.code().to_string();
        uint8_t precision = quantity.sym.precision();
        WASM_ASSERT(precision == 0,
                    account_operation_exception,
                    "wasmnativecontract.add_balance, the precision of system coin %s must be %d",
                    symbol,
                    0) 

        WASM_ASSERT(to.OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount),
                    account_operation_exception,
                    "wasmnativecontract.add_balance, operate account failed ,name=%s",
                    to.nickid.ToString().c_str()) 

        WASM_ASSERT(database.SetAccount(to.regid, to), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")          
    }

};