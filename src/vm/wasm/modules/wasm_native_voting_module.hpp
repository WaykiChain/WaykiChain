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

namespace wasm {

    class native_voting_module: public native_module {
    public:
        static const uint64_t id = wasmio_voting;//REGID(0-900);

    public:
        void register_routes(abi_router &abi_r, action_router &act_r) {
            abi_r.add_router(id, abi_handler);
            act_r.add_router(id, act_handler);
        }

        public:
            static void act_handler(wasm_context &context, uint64_t action) {
                switch (action) {
                case NAME(start): // start the block reward
                    start(context);
                    return;
                case NAME(setvotes): // set votes
                    setvotes(context);
                    return;
                case NAME(mintrewards): // claim rewards
                    mintrewards(context);
                    return;
                default:
                    break;
                }

                CHAIN_ASSERT(false, wasm_chain::action_not_found_exception,
                             "handler '%s' does not exist in native contract '%s'",
                             wasm::name(action).to_string(),
                             wasm::regid(id).to_string())
            };

            static std::vector<char> abi_handler() {
                abi_def abi;

                if (abi.version.size() == 0) {
                    abi.version = "wasm::abi/1.0";
                }

                abi.structs.push_back({"start", "",
                    {
                        // no params
                    }
                });
                abi.structs.push_back({"setvotes", "",
                    {
                        {"candidate",       "regid"     },
                        {"votes",           "uint64"    },
                        {"memo",            "string"    }
                    }
                });
                abi.structs.push_back({"mintrewards", "",
                    {
                        {"to",          "regid"     },
                        {"max_amount",  "uint64_t"  },
                        {"memo",        "string"    }
                    }
                });

                abi.actions.emplace_back( "start",          "start",        "" );
                abi.actions.emplace_back( "setvotes",       "setvotes",     "" );
                abi.actions.emplace_back( "mintrewards",    "mintrewards",  "" );

                auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
                return abi_bytes;
            }

        /**
         * Usage: issue an UIA asset
         */
        static void start(wasm_context &context) {
            // TODO: check version fork
            _check_receiver_is_self(context._receiver);

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            CHAIN_ASSERT(   context.trx.data.empty(),
                            wasm_chain::native_contract_assert_exception,
                            "params must be empty")
            auto &db = context.database;
            uint64_t voting_contract = _get_voting_contract(context);
            CRegID voting_contract_regid = CRegID(voting_contract);
            auto sp_account = get_account(context, voting_contract_regid, "voting_contract");

            context.require_auth( voting_contract );

            CBlockInflatedReward reward_info; // all fields must be 0 or empty
            if (db.blockCache.GetBlockInflatedReward(reward_info)) {
                CHAIN_ASSERT(   reward_info.start_height == 0,
                                wasm_chain::native_contract_assert_exception,
                                "block inflated reward has been start at height=",
                                reward_info.start_height)
            }
            reward_info.start_height = context.trx_cord.GetHeight();

            CHAIN_ASSERT(       db.blockCache.SetBlockInflatedReward(reward_info),
                                wasm_chain::native_contract_assert_exception,
                                "block inflated reward save error")
        }

        /**
         * set received votes of candidate
         */
        static void setvotes(wasm_context &context) {

            _check_receiver_is_self(context._receiver);

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack<std::tuple <regid, uint64_t, string >> (context.trx.data);

            auto candidate              = std::get<0>(params);
            auto votes                  = std::get<1>(params);
            auto memo                   = std::get<2>(params);

            uint64_t voting_contract = _get_voting_contract(context);

            context.require_auth( voting_contract );

            auto candidate_regid = CRegID(candidate.value);

            CHAIN_CHECK_REGID(candidate_regid, "candidate regid")
            CHAIN_CHECK_MEMO(memo, "memo");

            auto &db = context.database;
            CHAIN_ASSERT(    db.delegateCache.SetDelegateVotes(candidate_regid, votes),
                                wasm_chain::native_contract_assert_exception,
                                "save candidate votes failed");

            WASM_TRACE("set received votes=%llu of candidate: %s", votes, candidate_regid.ToString() )

            context.notify_recipient(candidate.value);
        }

        /**
         * set received votes of candidate
         */
        static void mintrewards(wasm_context &context) {

            _check_receiver_is_self(context._receiver);

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack<std::tuple <wasm::regid, uint64_t, string >> (context.trx.data);

            auto to                     = std::get<0>(params);
            auto max_amount             = std::get<1>(params);
            auto memo                   = std::get<2>(params);

            uint64_t voting_contract = _get_voting_contract(context);

            context.require_auth( voting_contract );

            auto to_regid = CRegID(to.value);

            CHAIN_CHECK_REGID(to_regid, "to regid")
            CHAIN_CHECK_MEMO(memo, "memo");

            auto &db = context.database;

            CBlockInflatedReward reward_info; // all fields must be 0 or empty
            CHAIN_ASSERT(      db.blockCache.GetBlockInflatedReward(reward_info) &&
                                    reward_info.start_height > 0,
                                wasm_chain::native_contract_assert_exception,
                                "block inflated reward is not started")

            auto sp_to_account = get_account(context, to_regid, "to account");
            uint64_t new_rewards = reward_info.new_rewards;
            if (new_rewards != 0) {
                if (max_amount != 0) {
                    new_rewards = std::min(new_rewards, max_amount);
                }
                CHAIN_ASSERT(   sp_to_account->OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, new_rewards,
                                        ReceiptType::WASM_MINT_COINS, context.control_trx.receipts, nullptr),
                                wasm_chain::native_contract_assert_exception,
                                "operate balance of to account error")
            }
            reward_info.new_rewards -= new_rewards;
            reward_info.total_claimed += new_rewards;
            reward_info.last_claimed = new_rewards;
            reward_info.last_claimed = context.trx_cord.GetHeight();

            CHAIN_ASSERT(       db.blockCache.SetBlockInflatedReward(reward_info),
                                wasm_chain::native_contract_assert_exception,
                                "save block inflated reward info failed");

            auto minted_rewards = asset(new_rewards, WICC_SYMBOL);
            _on_mint(context, wasm::regid(voting_contract), to, minted_rewards, memo);

            WASM_TRACE("mint rewards=%d to %s. memo:%s", minted_rewards.to_string(), to_regid.ToString(), memo )
        }
        /**
         * on_mint
         * the voting_contract must implement action on_mint() to receive the mint
         * ACTION on_mint(const wasm::regid &to, const asset &minted_rewards, const string &memo)
         */
        static inline void _on_mint(wasm_context &context, const wasm::regid &voting_contract,
                    const wasm::regid &to, const asset &minted_rewards, const string &memo) {
            auto data = wasm::pack(std::make_tuple(to, minted_rewards, memo));
            inline_transaction trx = {voting_contract.value, N(on_mint), {{id, wasmio_code}}, data};
            context.execute_inline(trx);
        }

        static inline uint64_t _get_voting_contract(wasm_context &context) {
            auto &db = context.database;
            uint64_t voting_contract = 0;
            db.sysParamCache.GetParam(SysParamType::VOTING_CONTRACT_REGID, voting_contract);
            CHAIN_ASSERT(   voting_contract != 0,
                            wasm_chain::native_contract_assert_exception,
                            "VOTING_CONTRACT_REGID not set yet");
            return voting_contract;
        }

        static inline void _check_receiver_is_self(uint64_t recever) {
            CHAIN_ASSERT(    recever == native_voting_module::id,
                                wasm_chain::native_contract_assert_exception,
                                "expect contract '%s', but get '%s'",
                                wasm::regid(native_voting_module::id).to_string(),
                                wasm::regid(recever).to_string());
        }
    };
}
