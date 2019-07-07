// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_TXBASE_H
#define CONFIG_TXBASE_H

#include <unordered_map>
#include <string>
#include <cstdint>

using namespace std;

static const int32_t nTxVersion1 = 1;
static const int32_t nTxVersion2 = 2;
#define SCRIPT_ID_SIZE (6)

enum TxType: unsigned char {
    BLOCK_REWARD_TX     = 1,  //!< Miner Block Reward Tx
    ACCOUNT_REGISTER_TX = 2,  //!< Account Registration Tx
    BCOIN_TRANSFER_TX   = 3,  //!< BaseCoin Transfer Tx
    CONTRACT_INVOKE_TX  = 4,  //!< Contract Invocation Tx
    CONTRACT_DEPLOY_TX  = 5,  //!< Contract Deployment Tx
    DELEGATE_VOTE_TX    = 6,  //!< Vote Delegate Tx
    COMMON_MTX          = 7,  //!< Multisig Tx

    MCOIN_BLOCK_REWARD_TX    = 11,  //!< Multi Coin Miner Block Reward Tx
    MCOIN_CONTRACT_INVOKE_TX = 12,  //!< Multi Coin Contract Invocation Tx
    MCOIN_TRANSFER_TX        = 13,  //!< Coin Transfer Tx
    MCOIN_REWARD_TX          = 14,  //!< Coin Reward Tx

    CDP_STAKE_TX            = 21,  //!< CDP Staking/Restaking Tx
    CDP_REDEEMP_TX          = 22,  //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX        = 23,  //!< CDP Liquidation Tx (partial or full)

    PRICE_FEED_TX         = 31,  //!< Price Feed Tx: WICC/USD | WGRT/USD | WUSD/USD
    BLOCK_PRICE_MEDIAN_TX = 32,  //!< Block Median Price Tx

    SFC_PARAM_MTX         = 41,  //!< StableCoin Fund Committee invokes Param Set/Update MulSigTx
    SFC_GLOBAL_HALT_MTX   = 42,  //!< StableCoin Fund Committee invokes Global Halt CDP Operations MulSigTx
    SFC_GLOBAL_SETTLE_MTX = 43,  //!< StableCoin Fund Committee invokes Global Settle Operation MulSigTx

    SCOIN_TRANSFER_TX = 51,  //!< StableCoin Transfer Tx
    FCOIN_TRANSFER_TX = 52,  //!< FundCoin Transfer Tx
    FCOIN_STAKE_TX    = 53,  //!< Stake Fund Coin Tx in order to become a price feeder

    DEX_SETTLE_TX            = 61,  //!< dex settle Tx
    DEX_CANCEL_ORDER_TX      = 62,  //!< dex cancel order Tx
    DEX_BUY_LIMIT_ORDER_TX   = 63,  //!< dex buy limit price order Tx
    DEX_SELL_LIMIT_ORDER_TX  = 64,  //!< dex sell limit price order Tx
    DEX_BUY_MARKET_ORDER_TX  = 65,  //!< dex buy market price order Tx
    DEX_SELL_MARKET_ORDER_TX = 66,  //!< dex sell market price order Tx

    NULL_TX = 0  //!< NULL_TX
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
static const unordered_map<TxType, std::tuple<string, uint64_t, uint64_t, uint64_t, uint64_t>, TxTypeHash> kTxTypeMap = {
    { BLOCK_REWARD_TX,          std::make_tuple("BLOCK_REWARD_TX",         0,          0,         0,          0            ) },
    { MCOIN_REWARD_TX,          std::make_tuple("MCOIN_REWARD_TX",         0,          0,         0,          0            ) },
    { BLOCK_PRICE_MEDIAN_TX,    std::make_tuple("BLOCK_PRICE_MEDIAN_TX",   0,          0,         0,          0            ) },
    { ACCOUNT_REGISTER_TX,      std::make_tuple("ACCOUNT_REGISTER_TX",     10000,      10000,     10000,      10000        ) }, //0.0001 WICC, optional
    { BCOIN_TRANSFER_TX,        std::make_tuple("BCOIN_TRANSFER_TX",       10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { MCOIN_TRANSFER_TX,        std::make_tuple("MCOIN_TRANSFER_TX",       10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { CONTRACT_DEPLOY_TX,       std::make_tuple("CONTRACT_DEPLOY_TX",      100000000,  100000000, 100000000,  100000000    ) }, //0.01 WICC
    { CONTRACT_INVOKE_TX,       std::make_tuple("CONTRACT_INVOKE_TX",      1000,       1000,      1000,       1000         ) }, //0.001 WICC, min fees
    { DELEGATE_VOTE_TX,         std::make_tuple("DELEGATE_VOTE_TX",        10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { COMMON_MTX,               std::make_tuple("COMMON_MTX",              10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { CDP_STAKE_TX,             std::make_tuple("CDP_STAKE_TX",            100000,     100000,    100000,     100000       ) }, //0.001 WICC
    { CDP_REDEEMP_TX,           std::make_tuple("CDP_REDEEMP_TX",          100000,     100000,    100000,     100000       ) }, //0.001 WICC
    { CDP_LIQUIDATE_TX,         std::make_tuple("CDP_LIQUIDATE_TX",        100000,     100000,    100000,     100000       ) }, //0.001 WICC
    { PRICE_FEED_TX,            std::make_tuple("PRICE_FEED_TX",           10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { SFC_PARAM_MTX,            std::make_tuple("SFC_PARAM_MTX",           10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { FCOIN_STAKE_TX,           std::make_tuple("FCOIN_STAKE_TX",          10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_SETTLE_TX,            std::make_tuple("DEX_SETTLE_TX",           10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_CANCEL_ORDER_TX,      std::make_tuple("DEX_CANCEL_ORDER_TX",     10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_BUY_LIMIT_ORDER_TX,   std::make_tuple("DEX_BUY_LIMIT_ORDER_TX",  10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_SELL_LIMIT_ORDER_TX,  std::make_tuple("DEX_SELL_LIMIT_ORDER_TX", 10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_BUY_MARKET_ORDER_TX,  std::make_tuple("DEX_BUY_MARKET_ORDER_TX", 10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { DEX_SELL_MARKET_ORDER_TX, std::make_tuple("DEX_SELL_MARKET_ORDER_TX",10000,      10000,     10000,      10000        ) }, //0.0001 WICC
    { NULL_TX,                  std::make_tuple("NULL_TX",                 0,          0,         0,          0            ) }
};

#endif