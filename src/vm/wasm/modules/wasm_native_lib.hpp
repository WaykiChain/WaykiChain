#pragma once

#include "entities/receipt.h"
#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"

#include "wasm/wasm_router.hpp"
#include "wasm/modules/wasm_handler.hpp"
#include "wasm/modules/wasm_native_bank_module.hpp"
#include "wasm/modules/wasm_native_module.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

    using transfer_data_t = std::tuple <uint64_t, uint64_t, wasm::asset, string >;
    using set_code_data_t = std::tuple<uint64_t, string, string, string>;

    inline void sub_balance(CAccount& owner, const wasm::asset& quantity, CAccountDBCache &database, ReceiptList &receipts) {

        string symbol     = quantity.symbol.code().to_string();
        uint8_t precision = quantity.symbol.precision();
        CHAIN_ASSERT( precision == 8,
                      account_access_exception,
                      "The precision of system coin %s must be %d",
                      symbol, 8)

        CHAIN_ASSERT( owner.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount,
                                           ReceiptCode::WASM_TRANSFER_ACTUAL_COINS, receipts),
                      account_access_exception,
                      "Account %s overdrawn balance",
                      owner.regid.ToString())

        CHAIN_ASSERT( database.SetAccount(owner.regid, owner), account_access_exception,
                      "Save account error")
    }

    inline void add_balance(CAccount& owner, const wasm::asset& quantity, CAccountDBCache &database, ReceiptList &receipts){

        string symbol     = quantity.symbol.code().to_string();
        uint8_t precision = quantity.symbol.precision();
        CHAIN_ASSERT( precision == 8,
                      account_access_exception,
                      "The precision of system coin %s must be %d",
                      symbol, 8)

        CHAIN_ASSERT( owner.OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount,
                                            ReceiptCode::WASM_TRANSFER_ACTUAL_COINS, receipts),
                      account_access_exception,
                      "Operate account %s failed",
                      owner.regid.ToString().c_str())

        CHAIN_ASSERT( database.SetAccount(owner.regid, owner), account_access_exception,
                      "Save account error")
    }

};