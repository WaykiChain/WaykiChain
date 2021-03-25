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
                case NAME(vote): // set votes
                    vote(context);
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

                abi.structs.push_back({"vote", "",
                    {
                        {"candidate",       "regid"     },
                        {"votes",           "int64"    },
                        {"memo",            "string"    }
                    }
                });
                abi.structs.push_back({"mintrewards", "",
                    {
                        {"to",          "regid"     },
                        {"memo",        "string"    }
                    }
                });

                abi.actions.emplace_back( "vote",       "vote",     "" );
                abi.actions.emplace_back( "mintrewards",    "mintrewards",  "" );

                auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
                return abi_bytes;
            }

        /**
         * set received votes of candidate
         */
        static void vote(wasm_context &context) {

            _check_receiver_is_self(context._receiver);

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack<std::tuple <regid, int64_t, string >> (context.trx.data);

            auto candidate              = std::get<0>(params);
            auto votes                  = std::get<1>(params);
            auto memo                   = std::get<2>(params);

            uint64_t voting_contract = _get_voting_contract(context);

            context.require_auth( voting_contract );

            auto candidate_regid = CRegID(candidate.value);

            CHAIN_CHECK_REGID(candidate_regid, "candidate regid")
            CHAIN_CHECK_MEMO(memo, "memo");

            auto &db = context.database;

            auto sp_candidate_account = get_account(context, candidate_regid, "candidate account");

            int64_t total_votes = sp_candidate_account->received_votes;
            int64_t old_total_votes = total_votes;
            CHAIN_ASSERT(   old_total_votes >= 0,
                            wasm_chain::native_contract_assert_exception,
                            "the old_total_votes=%llu can not be negtive", old_total_votes);
            total_votes += votes;
            CHAIN_ASSERT(   total_votes >= votes && total_votes >= 0,
                            wasm_chain::native_contract_assert_exception,
                            "operate candidate votes overflow! old_total_votes=%lld, votes=%lld",
                            old_total_votes, votes);
            sp_candidate_account->received_votes = total_votes; // total_votes >= 0

            // Votes: set the new value and erase the old value

            CHAIN_ASSERT(   db.delegateCache.SetDelegateVotes(candidate_regid, total_votes),
                            wasm_chain::native_contract_assert_exception,
                            "save candidate=%s votes failed", candidate_regid.ToString());

            CHAIN_ASSERT(   db.delegateCache.EraseDelegateVotes(candidate_regid, old_total_votes),
                            wasm_chain::native_contract_assert_exception,
                            "erase candidate=%s old votes failed", candidate_regid.ToString());

            WASM_TRACE("receive votes=%lld of candidate: %s, total_votes=%lld", votes, candidate_regid.ToString(), total_votes )

            context.notify_recipient(candidate.value);
        }

        /**
         * set received votes of candidate
         */
        static void mintrewards(wasm_context &context) {

            _check_receiver_is_self(context._receiver);

            context.control_trx.fuel   += calc_inline_tx_fuel(context);

            auto params = wasm::unpack<std::tuple <wasm::regid, string >> (context.trx.data);

            auto to                     = std::get<0>(params);
            auto memo                   = std::get<1>(params);

            uint64_t voting_contract = _get_voting_contract(context);

            context.require_auth( voting_contract );

            auto to_regid = CRegID(to.value);

            CHAIN_CHECK_REGID(to_regid, "to regid")
            CHAIN_CHECK_MEMO(memo, "memo");

            auto &db = context.database;
            uint64_t height = context.trx_cord.GetHeight();
            auto sp_to_account = get_account(context, to_regid, "to account");
            uint64_t new_rewards = 0;

            CBlockInflatedReward reward_info; // all fields must be 0 or empty
            db.blockCache.GetBlockInflatedReward(reward_info);
            if (reward_info.start_height == 0) {
                reward_info.start_height = context.trx_cord.GetHeight();
                WASM_TRACE("start rewards at height=%llu. to:%s memo:%s", height, to_regid.ToString(), memo )
            } else {
                CHAIN_ASSERT(       reward_info.start_height <= height,
                                    wasm_chain::native_contract_assert_exception,
                                    "start_height=%llu must <= current height=%llu",
                                    reward_info.start_height, height)
                new_rewards                     = reward_info.new_rewards;
                reward_info.new_rewards         = 0;
                if (new_rewards != 0) {
                    CHAIN_ASSERT(   sp_to_account->OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, new_rewards,
                                            ReceiptType::WASM_MINT_COINS, context.control_trx.receipts, nullptr),
                                    wasm_chain::native_contract_assert_exception,
                                    "operate balance of to account error")
                }

                reward_info.total_minted       += new_rewards;
                reward_info.last_minted         = new_rewards;
                reward_info.last_minted_height  = height;

            }

            CHAIN_ASSERT(       db.blockCache.SetBlockInflatedReward(reward_info),
                                wasm_chain::native_contract_assert_exception,
                                "save block inflated reward info failed");

            auto minted_rewards = asset(new_rewards, WICC_SYMBOL);
            _on_mint(context, wasm::regid(voting_contract), to, minted_rewards, memo);
            WASM_TRACE("mint rewards=%d to %s at height:%llu. memo:%s", minted_rewards.to_string(), to_regid.ToString(), height, memo )
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
