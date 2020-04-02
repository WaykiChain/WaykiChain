#pragma once
#include<map>

namespace wasm {
  
  inline void sub_balance(CAccount& owner, const wasm::asset& quantity, CAccountDBCache &database, ReceiptList &receipts, bool is_not_tx_payer = true) {

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

      if(is_not_tx_payer){
          CHAIN_ASSERT( database.SetAccount(owner.regid, owner), account_access_exception,
                        "Save account error")
      }
  }

  inline void add_balance(CAccount& owner, const wasm::asset& quantity, CAccountDBCache &database, ReceiptList &receipts, bool is_not_tx_payer = true){

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

      if(is_not_tx_payer){
        CHAIN_ASSERT( database.SetAccount(owner.regid, owner), account_access_exception,
                      "Save account error")
      }
  }	

}