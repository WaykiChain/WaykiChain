#pragma once
#include<map>

namespace wasm {

  //only asset owner can invoke this op
  inline void mint_burn_balance(wasm_context &context, bool isMintOperate, bool is_not_tx_payer = true) {

      auto &database                = context.database.accountCache;
      context.control_trx.run_cost += context.trx.GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;

      auto transfer_data = wasm::unpack<std::tuple<uint64_t, wasm::asset>>(context.trx.data);
      auto target        = std::get<0>(transfer_data);
      auto quantity      = std::get<1>(transfer_data);

      CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
      CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");

      string symbol      = quantity.symbol.code().to_string();
      CAsset asset;
      CHAIN_ASSERT( context.assetCache.GetAsset(symbol, asset),
                      wasm_chain::native_contract_assert_exception,
                      "asset (%s) not found from d/b",
                      symbol );

      CAccount assetOwner;
      CHAIN_ASSERT( database.GetAccount(asset.owner_uid, assetOwner),
                      wasm_chain::native_contract_assert_exception,
                      "asset owner account '%s' does not exist",
                      asset.regid.to_string())

      context.require_auth(assetOwner.regid.GetIntValue());

      CAccount targetAccount;
      CHAIN_ASSERT( database.GetAccount(CRegID(target), targetAccount),
                      wasm_chain::native_contract_assert_exception,
                      "target account '%s' does not exist",
                      wasm::regid(target).to_string())

      if (isMintOperate) { //mint operate
        CHAIN_ASSERT( targetAccount.OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount, ReceiptCode::WASM_MINT_COINS, context.control_trx.receipts),
                      account_access_exception,
                      "Account %s balance overminted",
                      targetAccount.regid.ToString())
      } else {            //burn operate
        CHAIN_ASSERT( targetAccount.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount, ReceiptCode::WASM_BURN_COINS, context.control_trx.receipts),
                    account_access_exception,
                    "Account %s balance overburn",
                    targetAccount.regid.ToString())
      }

      if (targetAccount.keyid != context.control_trx.txAccount.keyid) //skip saving for Tx payer account
        CHAIN_ASSERT( database.SetAccount(targetAccount.keyid, targetAccount), account_access_exception, "Save targetAccount error")

      context.require_recipient(target);
  }

  inline void transfer_balance(CAccount& toAccount, const wasm::asset& quantity, wasm_context &context) {

      string symbol     = quantity.symbol.code().to_string();
      uint8_t precision = quantity.symbol.precision();
      CHAIN_ASSERT( precision == 8,
                    account_access_exception,
                    "The precision of system coin %s must be %d",
                    symbol, 8)

      CAccount *pToAccount = toAccount.IsEmpty() ? nullptr : &toAccount;

      CHAIN_ASSERT( context.control_trx.txAccount.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount,
                    ReceiptCode::WASM_TRANSFER_ACTUAL_COINS, context.control_trx.receipts, pToAccount),
                    account_access_exception,
                    "Account %s balance overdrawn",
                    context.control_trx.txAccount.regid.ToString())

      //txAccount state will be saved from within CBaseTx::ExecuteFullTx, hence skipped

      CHAIN_ASSERT( context.database.accountCache.SetAccount(toAccount.keyid, toAccount), account_access_exception, "Save toAccount error")
  }

}