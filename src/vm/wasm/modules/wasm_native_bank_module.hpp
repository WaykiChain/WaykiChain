#pragma once
#include "wasm/wasm_context.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/abi_def.hpp"

#include "wasm/exception/exceptions.hpp"
#include "wasm/wasm_log.hpp"
#include "wasm/modules/wasm_router.hpp"
#include "wasm/modules/wasm_native_commons.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/types/regid.hpp"
#include "entities/account.h"
#include "entities/receipt.h"

extern bool ProcessAssetFee(CBaseTx &tx, CCacheWrapper &cw, CAccount *pSrcAccount,
                            const string &action, vector<CReceipt> &receipts, string &errMsg);

namespace wasm {

    static uint64_t bank_native_module_id = wasmio_bank;//REGID(0-800);

    class wasm_bank_native_module: public native_module {
    public:
        wasm_bank_native_module()  {}
        ~wasm_bank_native_module() {}

    public:
        void register_routes(abi_router &abi_r, action_router &act_r) {
            abi_r.add_router(bank_native_module_id, abi_handler);
            act_r.add_router(bank_native_module_id, act_handler);
        }

        public:
            static void act_handler(wasm_context &context, uint64_t action) {
                switch (action) {
                case NAME(issue): // issue a new asset
                    issue(context);
                    return;
                case NAME(mint): // mint new asset tokens
                    mint(context);
                    return;
                case NAME(burn): // burn asset tokens
                    burn(context);
                    return;
                case NAME(update): // update asset properties like owner's regid
                    update(context);
                    return;
                case NAME(transfer): // transfer asset tokens
                    tansfer(context);
                    return;
                case NAME(setasseticon): // set asset icon
                    setasseticon(context);
                    return;
                default:
                    break;
                }

                CHAIN_ASSERT(false, wasm_chain::action_not_found_exception,
                             "handler '%s' does not exist in native contract '%s'",
                             wasm::name(action).to_string(),
                             wasm::regid(bank_native_module_id).to_string())
            };

                static std::vector<char> abi_handler() {
            abi_def abi;

            if (abi.version.size() == 0) {
                abi.version = "wasm::abi/1.0";
            }

            abi.structs.push_back({"issue", "",
                {
                    {"symbol",          "symbol"    }, //target asset symbol to issue
                    {"owner",           "regid"     },
                    {"name",            "string"    },
                    {"total_supply",    "uint64"    },
                    {"mintable",        "bool"      },
                    {"memo",            "string"    }
                }
            });
            abi.structs.push_back({"mint", "",
                {
                    {"to",              "regid"     }, //mint & issue assets to the target holder
                    {"quantity",        "asset"     },
                    {"memo",            "string"    }
                }
            });
            abi.structs.push_back({"burn", "",
                {
                    {"owner",          "regid"     }, //only asset owner can burn assets hold by the owner
                    {"quantity",       "asset"     },
                    {"memo",           "string"    }
                }
            });
            abi.structs.push_back({"update", "",
                {
                    {"symbol_code",    "symbol_code"   }, //target asset symbol to update
                    {"owner",          "regid?"        },
                    {"name",           "string?"       },
                    {"memo",           "string?"       }
                }
            });
            abi.structs.push_back({"transfer", "",
                {
                    {"from",          "regid"    },
                    {"to",            "regid"    },
                    {"quantity",      "asset"    },
                    {"memo",          "string"   }
                }
            });
            abi.structs.push_back({"setasseticon", "",
                {
                    {"symbol_code",   "symbol_code"     },
                    {"icon_url",      "string"          }
                }
            });

            abi.actions.emplace_back( "issue",          "issue",        "" );
            abi.actions.emplace_back( "mint",           "mint",         "" );
            abi.actions.emplace_back( "burn",           "burn",         "" );
            abi.actions.emplace_back( "update",         "update",       "" );
            abi.actions.emplace_back( "transfer",       "transfer",     "" );
            abi.actions.emplace_back( "setasseticon",        "setasseticon",      "" );

            auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
            return abi_bytes;
        }

        /**
         * Usage: issue an UIA asset
         */
        static void issue(wasm_context &context) {

            CHAIN_ASSERT(    context._receiver == bank_native_module_id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(bank_native_module_id).to_string(),
                                wasm::regid(context._receiver).to_string());

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack< std::tuple <
                                            wasm::symbol,
                                            wasm::regid,
                                            string,
                                            uint64_t,
                                            bool,
                                            string >>(context.trx.data);

            auto symbol             = std::get<0>(params);
            auto owner              = std::get<1>(params);
            auto name               = std::get<2>(params);
            auto total_supply       = std::get<3>(params);
            auto mintable           = std::get<4>(params);
            auto memo               = std::get<5>(params);

            CHAIN_CHECK_UIA_SYMBOL( symbol, "UIA asset symbol")

            string msg = "";
            const auto &sym = symbol.code().to_string();

            auto owner_regid = CRegID(owner.value);
            CHAIN_CHECK_REGID(owner_regid, "owner account")

            CHAIN_CHECK_ASSET_NAME(name, "asset name")

            CHAIN_ASSERT(   !context.database.assetCache.HasAsset(sym),
                            wasm_chain::asset_type_exception,
                            "asset (%s) already issued",
                            sym )

            context.require_auth( owner.value );

            auto sp_account = get_account(context, owner_regid, "owner account");

            CHAIN_ASSERT(   ProcessAssetFee(context.control_trx, context.database, sp_account.get(),
                                    "issue", context.receipts, msg),
                            wasm_chain::account_access_exception,
                            "process asset fee error: %s", msg )


            CHAIN_ASSERT(   total_supply <= MAX_ASSET_TOTAL_SUPPLY,
                            wasm_chain::asset_total_supply_exception,
                            "total supply is too large than %llu", MAX_ASSET_TOTAL_SUPPLY );

            CHAIN_ASSERT(   sp_account->OperateBalance(sym, BalanceOpType::ADD_FREE, total_supply,
                                    ReceiptType::ASSET_MINT_NEW_AMOUNT, context.receipts),
                            wasm_chain::account_access_exception,
                            "add total supply to issuer=%s error", owner_regid.ToString() )

            CHAIN_CHECK_MEMO(memo, "memo");

            CAsset asset;
            asset.asset_symbol      = sym;
            asset.asset_name        = name;
            asset.asset_type        = AssetType::UIA;
            asset.owner_regid       = owner_regid;
            asset.total_supply      = total_supply;
            asset.mintable          = mintable;

            CHAIN_ASSERT(     context.database.assetCache.SetAsset(asset),
                            wasm_chain::level_db_update_fail,
                                "Update Asset (%s) failure",
                                symbol.to_string() )

        }

        static void mint(wasm_context &context) {

            CHAIN_ASSERT(    context._receiver == bank_native_module_id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(bank_native_module_id).to_string(),
                                wasm::regid(context._receiver).to_string());

            mint_burn_balance(context, true);
        }

        static void burn(wasm_context &context) {

            CHAIN_ASSERT(     context._receiver == bank_native_module_id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(bank_native_module_id).to_string(),
                                wasm::regid(context._receiver).to_string());

            mint_burn_balance(context, false);
        }

        static void update(wasm_context &context) {

            CHAIN_ASSERT(     context._receiver == bank_native_module_id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(bank_native_module_id).to_string(),
                                wasm::regid(context._receiver).to_string());

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack< std::tuple <
                            wasm::symbol_code,
                            std::optional<wasm::regid>,
                            std::optional<string>,
                            std::optional<string> >>(context.trx.data);

            auto sym_code    = std::get<0>(params);
            auto new_owner   = std::get<1>(params);
            auto new_name    = std::get<2>(params);
            auto memo        = std::get<3>(params);

            auto sym = sym_code.to_string();
            CAsset asset;
            CHAIN_ASSERT(context.database.assetCache.GetAsset(sym, asset),
                         wasm_chain::asset_type_exception,
                         "asset (%s) does not exist",
                         sym)

            CHAIN_CHECK_ASSET_HAS_OWNER(asset, "asset");

            context.require_auth( asset.owner_regid.GetIntValue() );

            auto sp_account = get_account(context, asset.owner_regid, "owner account");

            string msg;
            // CHAIN_ASSERT(   ProcessAssetFee(context.control_trx, context.database, sp_account.get(), "update", context.receipts, msg),
            //                 wasm_chain::account_access_exception,
            //                 "process asset fee error: %s", msg )

            bool to_update = false;

            if (new_owner) {
                auto new_owner_regid = CRegID(new_owner->value);
                CHAIN_CHECK_REGID(new_owner_regid, "new owner regid")
                check_account_exist(context, new_owner_regid, "new owner account");
                to_update             = true;
                asset.owner_regid      = new_owner_regid;
            }

                if (new_name) {
                CHAIN_CHECK_ASSET_NAME(new_name.value(), "new asset name")
                to_update             = true;
                asset.asset_name    = *new_name;
            }

            if (memo) CHAIN_CHECK_MEMO(memo.value(), "memo");

            CHAIN_ASSERT( to_update,
                          wasm_chain::native_contract_assert_exception,
                          "none field found for update")

            CHAIN_ASSERT( context.database.assetCache.SetAsset(asset),
                          wasm_chain::level_db_update_fail,
                          "Update Asset (%s) failure",
                          sym)

        }

        static void tansfer(wasm_context &context) {

            CHAIN_ASSERT(     context._receiver == bank_native_module_id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(bank_native_module_id).to_string(),
                                wasm::regid(context._receiver).to_string());

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto transfer_data = wasm::unpack<std::tuple <uint64_t, uint64_t,
                                    wasm::asset, string >>(context.trx.data);

            auto from                        = std::get<0>(transfer_data);
            auto to                          = std::get<1>(transfer_data);
            auto quantity                    = std::get<2>(transfer_data);
            auto memo                        = std::get<3>(transfer_data);

            context.require_auth(from); //from auth
            auto from_regid = CRegID(from);
            auto to_regid = CRegID(to);

            CHAIN_CHECK_REGID(from_regid, "from regid")
            CHAIN_CHECK_REGID(to_regid, "to regid")
            CHAIN_ASSERT(from != to,             wasm_chain::native_contract_assert_exception, "cannot transfer to self");
            CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
            CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");

            CHAIN_CHECK_MEMO(memo, "memo");

            //may not be txAccount since one trx can have multiple signed/authorized transfers (from->to)
            auto spFromAccount = get_account(context, from_regid, "from account");
            auto spToAccount = get_account(context, to_regid, "to account");

            CAsset asset;
            string symbol = quantity.symbol.code().to_string();
                CHAIN_ASSERT(     context.database.assetCache.GetAsset(symbol, asset),
                            wasm_chain::asset_type_exception,
                            "asset (%s) does not exist", symbol )

            transfer_balance( *spFromAccount, *spToAccount, quantity, context );

            WASM_TRACE("transfer from: %s, to: %s, quantity: %s",
                        spFromAccount->regid.ToString(), spToAccount->regid.ToString(), quantity.to_string().c_str() )

            context.notify_recipient(from);
            context.notify_recipient(to);

        }

        static void setasseticon(wasm_context &context) {

            CHAIN_ASSERT(     context._receiver == bank_native_module_id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(bank_native_module_id).to_string(),
                                wasm::regid(context._receiver).to_string());

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack< std::tuple <
                            wasm::symbol_code,
                            std::string> >(context.trx.data);

            auto sym_code    = std::get<0>(params);
            auto icon_url   = std::get<1>(params);

            auto sym = sym_code.to_string();
            CAsset asset;
            CHAIN_ASSERT(context.database.assetCache.GetAsset(sym, asset),
                         wasm_chain::asset_type_exception,
                         "asset (%s) does not exist",
                         sym)

            CHAIN_CHECK_ASSET_HAS_OWNER(asset, "asset");

            context.require_auth( asset.owner_regid.GetIntValue() );

            auto sp_account = get_account(context, asset.owner_regid, "owner account");

            CHAIN_ASSERT(icon_url.size() <= 256, wasm_chain::native_contract_assert_exception,
                        "icon_url is too long than 256 bytes", icon_url);

            asset.icon_url = icon_url;

            CHAIN_ASSERT( context.database.assetCache.SetAsset(asset),
                          wasm_chain::level_db_update_fail,
                          "Update Asset (%s) failure",
                          sym)

        }

    };
}
