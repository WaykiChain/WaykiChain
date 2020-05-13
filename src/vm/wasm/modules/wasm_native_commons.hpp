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
  }


  inline void mint_burn_balance(wasm_context &context, bool isMintOperate) {

      context.control_trx.run_cost += context.trx.GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;
      auto transfer_data  = wasm::unpack<std::tuple<uint64_t, wasm::asset>>(context.trx.data);

      auto target_regid   = std::get<0>(transfer_data);
      auto quantity       = std::get<1>(transfer_data);

      CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
      CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");

      CAsset asset;
      string symbol       = quantity.symbol.code().to_string();
      CHAIN_ASSERT( context.database.assetCache.GetAsset(symbol, asset),
                    wasm_chain::asset_type_exception,
                    "asset (%s) not found from d/b",
                    symbol );

      auto spOwnerAcct    = context.control_trx.GetAccount(context.database, asset.owner_uid);
      CHAIN_ASSERT( spOwnerAcct,
                    wasm_chain::account_access_exception,
                    "asset owner (%s) not found from d/b",
                    asset.owner_uid.ToString() );
      context.require_auth(spOwnerAcct->regid.GetIntValue()); //mint or burn op must be sanctioned by asset owner

      auto target         = context.control_trx.GetAccount(context.database, CRegID(target_regid));

      if (isMintOperate) { //mint operation
        CHAIN_ASSERT( target->OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount, ReceiptType::WASM_MINT_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overminted",
                      target->regid.ToString())

        asset.total_supply += quantity.amount;

      } else {            //burn operation
        //only asset owner can invoke this op!!!
        //     assets to be burnt must be first transferred to asset owner
        CHAIN_ASSERT( target_regid == spOwnerAcct->regid.GetIntValue(),
                      wasm_chain::native_contract_assert_exception,
                      "given asset owner (%s) diff from asset owner(%s) from d/b",
                      CRegID(target_regid).ToString(), spOwnerAcct->regid.ToString() );

        CHAIN_ASSERT( target->OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount, ReceiptType::WASM_BURN_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overburnt",
                      target->regid.ToString())

        CHAIN_ASSERT( asset.total_supply >= quantity.amount,
                      wasm_chain::native_contract_assert_exception,
                      "Asset total supply (%lld) < burn amount (%lld)",
                      asset.total_supply, quantity.amount)

        asset.total_supply -= quantity.amount;

      }

      context.notify_recipient(target_regid);
      CHAIN_ASSERT( context.database.assetCache.SetAsset(asset),
                      wasm_chain::asset_type_exception,
                      "Update Asset (%s) failure",
                      symbol)

  }

}