#pragma once
#include<map>

namespace wasm {

#define CHAIN_CHECK_REGID(regid, title)                                                            \
    CHAIN_ASSERT(!regid.IsEmpty(), wasm_chain::regid_type_exception, "invalid %s=%s", title,       \
                 regid.ToString())

#define CHAIN_CHECK_ASSET_NAME(name, title)                                                        \
    CHAIN_ASSERT(name.size() <= MAX_ASSET_NAME_LEN, wasm_chain::asset_name_exception,              \
                 "size=%s of %s is too large than %llu", name.size(), title, MAX_ASSET_NAME_LEN)

#define CHAIN_CHECK_UIA_SYMBOL(symbol, title)                                                      \
    {                                                                                              \
        CHAIN_ASSERT(symbol.is_valid(), wasm_chain::asset_type_exception, "invalid %s=%s", title,  \
                     symbol.to_string())                                                           \
        CHAIN_ASSERT(symbol.precision() == 8, wasm_chain::asset_type_exception,                    \
                     "the precision of %s=%s must be 8", title, symbol.to_string())                \
        string msg;                                                                                \
        CHAIN_ASSERT(CAsset::CheckSymbol(AssetType::UIA, symbol.code().to_string(), msg),          \
                     wasm_chain::asset_type_exception, "invalid UIA symbol=%s, %s",                \
                     symbol.to_string(), msg)                                                      \
    }

#define CHAIN_CHECK_MEMO(memo, title)                                                              \
    CHAIN_ASSERT(memo.size() <= 256, wasm_chain::native_contract_assert_exception,                 \
                 "%s has more than 256 bytes", title);

inline void transfer_balance(CAccount &fromAccount, CAccount &toAccount,
                             const wasm::asset &quantity, wasm_context &context) {

    string symbol     = quantity.symbol.code().to_string();
    uint8_t precision = quantity.symbol.precision();
    CHAIN_ASSERT(precision == 8, wasm_chain::account_access_exception,
                 "The precision of system coin %s must be %d", symbol, 8)

    CAccount *pToAccount = toAccount.IsEmpty() ? nullptr : &toAccount;

    CHAIN_ASSERT(fromAccount.OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount,
                                            ReceiptType::WASM_TRANSFER_ACTUAL_COINS,
                                            context.control_trx.receipts, pToAccount),
                 wasm_chain::account_access_exception, "Account %s balance overdrawn",
                 fromAccount.regid.ToString())
  }


  inline void mint_burn_balance(wasm_context &context, bool isMintOperate) {

      context.control_trx.run_cost += context.get_runcost();
      auto transfer_data  = wasm::unpack<std::tuple<uint64_t, wasm::asset, optional<string> >>(context.trx.data);

      auto target         = std::get<0>(transfer_data);
      auto quantity       = std::get<1>(transfer_data);
	  auto memo			  = std::get<2>(transfer_data);

      auto target_regid = CRegID(target);
      CHAIN_ASSERT(!target_regid.IsEmpty(), wasm_chain::regid_type_exception, "invalid target regid=%s", target_regid.ToString());
      CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
      CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");

      CAsset asset;
      string symbol       = quantity.symbol.code().to_string();
      CHAIN_ASSERT( context.database.assetCache.GetAsset(symbol, asset),
                    wasm_chain::asset_type_exception,
                    "asset (%s) not found from d/b",
                    symbol )

      CHAIN_ASSERT( asset.mintable,
                    wasm_chain::asset_type_exception,
                    "asset (%s) not mintable!",
                    symbol )

      auto spAssetOwnerAcct = context.control_trx.GetAccount(context.database, asset.owner_regid);
      CHAIN_ASSERT( spAssetOwnerAcct,
                    wasm_chain::account_access_exception,
                    "asset owner account (%s) not found from db",
                    asset.owner_regid.ToString() )

      context.require_auth(spAssetOwnerAcct->regid.GetIntValue()); //mint or burn op must be sanctioned by asset owner

      auto spTargetAcct   = context.control_trx.GetAccount(context.database, target_regid);
      CHAIN_ASSERT( spTargetAcct,
                    wasm_chain::account_access_exception,
                    "target account (%s) not found from db",
                    target_regid.ToString() )

      if (isMintOperate) { //mint operation

      uint64_t new_total_supply = asset.total_supply + quantity.amount;
      CHAIN_ASSERT(new_total_supply <= MAX_ASSET_TOTAL_SUPPLY &&
                        new_total_supply >= asset.total_supply, // to prevent overflow
                    wasm_chain::asset_total_supply_exception,
                    "new total supply is too large than %llu after mint %llu",
                    MAX_ASSET_TOTAL_SUPPLY, quantity.amount);

        CHAIN_ASSERT( spTargetAcct->OperateBalance(symbol, BalanceOpType::ADD_FREE, quantity.amount,
                                                  ReceiptType::WASM_MINT_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overminted",
                      spTargetAcct->regid.ToString())

        asset.total_supply = new_total_supply;


        context.notify_recipient(target);

      } else {            //burn operation
        //only asset owner can invoke this op!!!
        //     assets to be burnt must be first transferred to asset owner
        CHAIN_ASSERT( target == spAssetOwnerAcct->regid.GetIntValue(),
                      wasm_chain::native_contract_assert_exception,
                      "given asset owner (%s) diff from asset owner(%s) from d/b",
                      CRegID(target).ToString(), spAssetOwnerAcct->regid.ToString() );

        CHAIN_ASSERT( asset.total_supply >= quantity.amount,
                      wasm_chain::native_contract_assert_exception,
                      "Asset total supply (%lld) < burn amount (%lld)",
                      asset.total_supply, quantity.amount)

        CHAIN_ASSERT( spTargetAcct->OperateBalance(symbol, BalanceOpType::SUB_FREE, quantity.amount,
                                                    ReceiptType::WASM_BURN_COINS, context.control_trx.receipts),
                      wasm_chain::account_access_exception,
                      "Asset Owner (%s) balance overburnt",
                      spTargetAcct->regid.ToString())

        asset.total_supply -= quantity.amount;

      }
	  if (memo) CHAIN_CHECK_MEMO(memo.value(), "memo");

      CHAIN_ASSERT( context.database.assetCache.SetAsset(asset),
                      wasm_chain::native_contract_assert_exception,
                      "Update Asset (%s) failure",
                      symbol)
    }
}