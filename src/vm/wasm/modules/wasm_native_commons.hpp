#pragma once
#include<map>

namespace wasm {

  inline void transfer_balance(CAccount& fromAccount, CAccount& toAccount, const wasm::asset& quantity, wasm_context &context) {

      string symbol     = quantity.symbol.code().to_string();
      uint8_t precision = quantity.symbol.precision();
      CHAIN_ASSERT( precision == 8,
                    account_access_exception,
                    "The precision of system coin %s must be %d",
                    symbol, 8)

      CAccount *pToAccount = toAccount.IsEmpty() ? nullptr : &toAccount;

      CHAIN_ASSERT( fromAccount.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount,
                                              ReceiptCode::WASM_TRANSFER_ACTUAL_COINS, context.control_trx.receipts, pToAccount),
                                              wasm_chain::account_access_exception,
                                              "Account %s balance overdrawn",
                                              fromAccount.regid.ToString())

      if ( fromAccount.keyid != context.control_trx.txAccount.keyid )
        CHAIN_ASSERT( context.database.accountCache.SetAccount(fromAccount.keyid, fromAccount), account_access_exception, "Save fromAccount error")

      if ( !toAccount.IsEmpty() && toAccount.keyid != context.control_trx.txAccount.keyid )
        CHAIN_ASSERT( context.database.accountCache.SetAccount(toAccount.keyid, toAccount), account_access_exception, "Save toAccount error")
  }

  //only asset owner can invoke this op
  inline void mint_burn_balance(wasm_context &context, bool isMintOperate) {

      context.control_trx.run_cost += context.trx.GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;

      auto transfer_data = wasm::unpack<std::tuple<uint64_t, uint64_t, wasm::asset>>(context.trx.data);
      auto owner         = std::get<0>(transfer_data);
      auto target        = std::get<1>(transfer_data);
      auto quantity      = std::get<2>(transfer_data);

      CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
      CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");

      string symbol      = quantity.symbol.code().to_string();
      CAsset asset;
      CHAIN_ASSERT( context.database.assetCache.GetAsset(symbol, asset),
                    wasm_chain::asset_type_exception,
                    "asset (%s) not found from d/b",
                    symbol );

      CAccount assetOwnerAccount;
      CHAIN_ASSERT( context.database.accountCache.GetAccount(asset.owner_uid, assetOwnerAccount),
                    wasm_chain::account_access_exception,
                    "asset owner (%s) not found from d/b",
                    asset.owner_uid.ToString() );

      CHAIN_ASSERT( owner == assetOwnerAccount.regid.GetIntValue(),
                    wasm_chain::native_contract_assert_exception,
                    "input asset owner (%s) diff from asset owner(%s) from d/b",
                    CRegID(owner).ToString(), assetOwnerAccount.regid.ToString() );

      CAccount targetAccount;
      CHAIN_ASSERT( context.database.accountCache.GetAccount(CRegID(target), targetAccount),
                    wasm_chain::account_access_exception,
                    "target account '%s' does not exist",
                    wasm::regid(target).to_string())

      if (isMintOperate) { //mint operation
        context.require_auth(owner);
        CHAIN_ASSERT( assetOwnerAccount.OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount, ReceiptCode::WASM_MINT_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overminted",
                      assetOwnerAccount.regid.ToString())

        CHAIN_ASSERT( assetOwnerAccount.keyid != context.control_trx.txAccount.keyid &&
                      context.database.accountCache.SetAccount(assetOwnerAccount.keyid, assetOwnerAccount),
                      wasm_chain::account_access_exception,
                      "Save assetOwnerAccount error")

        transfer_balance( assetOwnerAccount, targetAccount, quantity, context );

        asset.total_supply += quantity.amount;

        context.require_recipient(owner);
        context.require_recipient(target);

      } else {            //burn operation
        context.require_auth(target);
        transfer_balance( targetAccount, assetOwnerAccount, quantity, context );

        context.require_auth(owner);
        CHAIN_ASSERT( assetOwnerAccount.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount, ReceiptCode::WASM_BURN_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overburnt",
                      assetOwnerAccount.regid.ToString())

        CHAIN_ASSERT( assetOwnerAccount.keyid != context.control_trx.txAccount.keyid &&
                      context.database.accountCache.SetAccount(assetOwnerAccount.keyid, assetOwnerAccount),
                      wasm_chain::account_access_exception,
                      "Save assetOwnerAccount error")

        CHAIN_ASSERT( asset.total_supply >= quantity.amount,
                      wasm_chain::native_contract_assert_exception,
                      "Asset total supply (%lld) < burn amount (%lld)",
                      asset.total_supply, quantity.amount)

        asset.total_supply -= quantity.amount;

        context.require_recipient(target);
        context.require_recipient(owner);
      }

      CHAIN_ASSERT( context.database.assetCache.SetAsset(asset),
                      wasm_chain::asset_type_exception,
                      "Update Asset (%s) failure",
                      symbol)

  }

}