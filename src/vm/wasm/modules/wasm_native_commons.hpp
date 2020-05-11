#pragma once
#include<map>

namespace wasm {

  inline void transfer_balance(CAccount& fromAccount, CAccount& toAccount, const wasm::asset& quantity, wasm_context &context) {

      string symbol     = quantity.symbol.code().to_string();
      uint8_t precision = quantity.symbol.precision();
      CHAIN_ASSERT( precision == 8,
                    wasm_chain::account_access_exception,
                    "The precision of system coin %s must be %d",
                    symbol, 8)

      CAccount *pToAccount = toAccount.IsEmpty() ? nullptr : &toAccount;

      CHAIN_ASSERT( fromAccount.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount,
                                              ReceiptType::WASM_TRANSFER_ACTUAL_COINS, context.control_trx.receipts, pToAccount),
                                              wasm_chain::account_access_exception,
                                              "Account %s balance overdrawn",
                                              fromAccount.regid.ToString())

      WASM_TRACE("finish transfer_balance, amount=%llu, symbol=%s, fromAccountRegId=%s",
                          quantity.amount, symbol, fromAccount.regid.ToString());

      // if ( fromAccount.keyid != context.control_trx.sp_tx_account->keyid )
      //   CHAIN_ASSERT( context.database.accountCache.SetAccount(fromAccount.keyid, fromAccount), account_access_exception, "Save fromAccount error")

      // if ( !toAccount.IsEmpty() && toAccount.keyid != context.control_trx.sp_tx_account->keyid )
      //   CHAIN_ASSERT( context.database.accountCache.SetAccount(toAccount.keyid, toAccount), account_access_exception, "Save toAccount error")
  }

  //only asset owner can invoke this op
  inline void mint_burn_balance(wasm_context &context, bool isMintOperate) {

      context.control_trx.run_cost += context.trx.GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;

      auto transfer_data = wasm::unpack<std::tuple<uint64_t, wasm::asset>>(context.trx.data);
      auto owner         = std::get<0>(transfer_data);
      auto quantity      = std::get<1>(transfer_data);

      CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
      CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");

      context.require_auth(owner);

      string symbol      = quantity.symbol.code().to_string();
      CAsset asset;
      CHAIN_ASSERT( context.database.assetCache.GetAsset(symbol, asset),
                    wasm_chain::asset_type_exception,
                    "asset (%s) not found from d/b",
                    symbol );

      auto spOwnerAcct = context.control_trx.GetAccount(context.database, asset.owner_uid);
      CHAIN_ASSERT( spOwnerAcct,
                    wasm_chain::account_access_exception,
                    "asset owner (%s) not found from d/b",
                    asset.owner_uid.ToString() );

      CHAIN_ASSERT( owner == spOwnerAcct->regid.GetIntValue(),
                    wasm_chain::native_contract_assert_exception,
                    "input asset owner (%s) diff from asset owner(%s) from d/b",
                    CRegID(owner).ToString(), spOwnerAcct->regid.ToString() );

      if (isMintOperate) { //mint operation
        CHAIN_ASSERT( spOwnerAcct->OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount, ReceiptType::WASM_MINT_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overminted",
                      spOwnerAcct->regid.ToString())

        asset.total_supply += quantity.amount;

      } else {            //burn operation
        CHAIN_ASSERT( spOwnerAcct->OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount, ReceiptType::WASM_BURN_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overburnt",
                      spOwnerAcct->regid.ToString())

        CHAIN_ASSERT( asset.total_supply >= quantity.amount,
                      wasm_chain::native_contract_assert_exception,
                      "Asset total supply (%lld) < burn amount (%lld)",
                      asset.total_supply, quantity.amount)

        asset.total_supply -= quantity.amount;

      }

      context.notify_recipient(owner);
      CHAIN_ASSERT( context.database.assetCache.SetAsset(asset),
                      wasm_chain::asset_type_exception,
                      "Update Asset (%s) failure",
                      symbol)

  }

}