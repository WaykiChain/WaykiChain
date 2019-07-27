// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_TXBASE_H
#define CONFIG_TXBASE_H

#include <unordered_map>
#include <string>
#include <cstdint>
#include <tuple>

using namespace std;

static const int32_t nTxVersion1 = 1;
static const int32_t nTxVersion2 = 2;

enum TxType: uint8_t {
    NULL_TX                     = 0     //!< NULL_TX

    /** R1 Tx types */
    BLOCK_REWARD_TX             = 1,    //!< Miner Block Reward Tx
    ACCOUNT_REGISTER_TX         = 2,    //!< Account Registration Tx
    BCOIN_TRANSFER_TX           = 3,    //!< BaseCoin Transfer Tx
    CONTRACT_INVOKE_TX          = 4,    //!< Contract Invocation Tx
    CONTRACT_DEPLOY_TX          = 5,    //!< LuaVM Contract Deployment Tx
    DELEGATE_VOTE_TX            = 6,    //!< Vote Delegate Tx

    /** R2 newly added Tx types below */
    BCOIN_TRANSFER_MTX          = 7,    //!< Multisig Tx
    FCOIN_STAKE_TX              = 8,    //!< Stake Fund Coin Tx in order to become a price feeder

    ASSET_ISSUE_TX              = 10,   //!< a user issues onchain asset
    UCOIN_TRANSFER_TX           = 11,   //!< Universal Coin Transfer Tx
    UCOIN_REWARD_TX             = 12,   //!< Universal Coin Reward Tx
    UCOIN_BLOCK_REWARD_TX       = 13,   //!< Universal Coin Miner Block Reward Tx
    UCONTRACT_DEPLOY_TX         = 14,   //!< universal VM contract deployment
    UCONTRACT_INVOKE_TX         = 15,   //!< universal VM contract invocation
    PRICE_FEED_TX               = 16,   //!< Price Feed Tx: WICC/USD | WGRT/USD | WUSD/USD
    PRICE_MEDIAN_TX             = 17,   //!< Price Median Value on each block Tx
    SYS_PARAM_PROPOSE_TX        = 18,   //!< validators propose Param Set/Update
    SYS_PARAM_RESPONSE_TX       = 19,   //!< validators response Param Set/Update

    CDP_STAKE_TX                = 21,   //!< CDP Staking/Restaking Tx
    CDP_REDEEM_TX               = 22,   //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX            = 23,   //!< CDP Liquidation Tx (partial or full)

    DEX_TRADEPAIR_PROPOSE_TX    = 81,   //!<Owner proposes a trade pair on DEX
    DEX_TRADEPAIR_LIST_TX       = 82,   //!<Owner lists a traide pair on DEX
    DEX_TRADEPAIR_DELIST_TX     = 83,   //!< Owner or validators delist a trade pair
    DEX_LIMIT_BUY_ORDER_TX      = 84,   //!< dex buy limit price order Tx
    DEX_LIMIT_SELL_ORDER_TX     = 85,   //!< dex sell limit price order Tx
    DEX_MARKET_BUY_ORDER_TX     = 86,   //!< dex buy market price order Tx
    DEX_MARKET_SELL_ORDER_TX    = 87,   //!< dex sell market price order Tx
    DEX_CANCEL_ORDER_TX         = 88,   //!< dex cancel order Tx
    DEX_TRADE_SETTLE_TX         = 89,   //!< dex settle Tx

};

struct TxTypeHash {
    size_t operator()(const TxType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};

/**
 * TxTypeKey -> {   TxTypeName,
 *                  InterimPeriodTxFees(WICC), EffectivePeriodTxFees(WICC),
 *                  InterimPeriodTxFees(WUSD), EffectivePeriodTxFees(WUSD)
 *              }
 *
 * Fees are boosted by 10^8
 */
static const unordered_map<TxType, std::tuple<string, uint64_t, uint64_t, uint64_t, uint64_t>, TxTypeHash> kTxFeeTable = {
    { BLOCK_REWARD_TX,          std::make_tuple("BLOCK_REWARD_TX",         0,          0,         0,          0            ) },
    { UCOIN_BLOCK_REWARD_TX,    std::make_tuple("UCOIN_BLOCK_REWARD_TX",   0,          0,         0,          0            ) },
    { UCOIN_REWARD_TX,          std::make_tuple("UCOIN_REWARD_TX",         0,          0,         0,          0            ) },
    { PRICE_MEDIAN_TX,    std::make_tuple("PRICE_MEDIAN_TX",   0,          0,         0,          0            ) },
    { ACCOUNT_REGISTER_TX,      std::make_tuple("ACCOUNT_REGISTER_TX",     0,          10000,     10000,      10000        ) }, //0.0001 WICC, optional
    { BCOIN_TRANSFER_TX,        std::make_tuple("BCOIN_TRANSFER_TX",       10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { UCOIN_TRANSFER_TX,        std::make_tuple("UCOIN_TRANSFER_TX",       10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { CONTRACT_DEPLOY_TX,       std::make_tuple("CONTRACT_DEPLOY_TX",      100000000,  100000000, 100000000,  100000000    ) }, //0.01 WICC (unit fuel rate)
    { CONTRACT_INVOKE_TX,       std::make_tuple("CONTRACT_INVOKE_TX",      1000,       1000,      1000,       1000         ) }, //0.001 WICC, min fees
    { DELEGATE_VOTE_TX,         std::make_tuple("DELEGATE_VOTE_TX",        10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { BCOIN_TRANSFER_MTX,               std::make_tuple("BCOIN_TRANSFER_MTX",              10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { CDP_STAKE_TX,             std::make_tuple("CDP_STAKE_TX",            100000,     100000,    100000,     100000       ) }, //0.001 WICC
    { CDP_REDEEM_TX,            std::make_tuple("CDP_REDEEM_TX",           100000,     100000,    100000,     100000       ) }, //0.001 WICC
    { CDP_LIQUIDATE_TX,         std::make_tuple("CDP_LIQUIDATE_TX",        100000,     100000,    100000,     100000       ) }, //0.001 WICC
    { PRICE_FEED_TX,            std::make_tuple("PRICE_FEED_TX",           10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { SFC_PARAM_MTX,            std::make_tuple("SFC_PARAM_MTX",           10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { FCOIN_STAKE_TX,           std::make_tuple("FCOIN_STAKE_TX",          10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_TRADE_SETTLE_TX,            std::make_tuple("DEX_TRADE_SETTLE_TX",           10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_CANCEL_ORDER_TX,      std::make_tuple("DEX_CANCEL_ORDER_TX",     10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_LIMIT_BUY_ORDER_TX,   std::make_tuple("DEX_LIMIT_BUY_ORDER_TX",  10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_LIMIT_SELL_ORDER_TX,  std::make_tuple("DEX_LIMIT_SELL_ORDER_TX", 10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_MARKET_BUY_ORDER_TX,  std::make_tuple("DEX_MARKET_BUY_ORDER_TX", 10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_MARKET_SELL_ORDER_TX, std::make_tuple("DEX_MARKET_SELL_ORDER_TX",10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { NULL_TX,                  std::make_tuple("NULL_TX",                 0,          0,         0,          0            ) }
};

#endif