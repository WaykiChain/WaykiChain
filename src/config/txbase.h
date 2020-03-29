// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_TXBASE_H
#define CONFIG_TXBASE_H

#include "const.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <tuple>

using namespace std;

static const int32_t INIT_TX_VERSION = 1;

enum TxType: uint8_t {
    NULL_TX                     = 0,    //!< NULL_TX

    /** R1 Tx types */
    BLOCK_REWARD_TX             = 1,    //!< Miner Block Reward Tx
    ACCOUNT_REGISTER_TX         = 2,    //!< Account Registration Tx
    BCOIN_TRANSFER_TX           = 3,    //!< BaseCoin Transfer Tx
    LCONTRACT_INVOKE_TX         = 4,    //!< LuaVM Contract Invocation Tx
    LCONTRACT_DEPLOY_TX         = 5,    //!< LuaVM Contract Deployment Tx
    DELEGATE_VOTE_TX            = 6,    //!< Vote Delegate Tx

    /** R2 newly added Tx types below */
    UCOIN_STAKE_TX              = 8,    //!< Stake Fund Coin Tx in order to become a price feeder

    ASSET_ISSUE_TX              = 9,    //!< a user issues onchain asset
    UIA_UPDATE_TX               = 10,   //!< a user update onchain asset

    UCOIN_TRANSFER_TX           = 11,   //!< Universal Coin Transfer Tx
    UCOIN_MINT_TX               = 12,   //!< Universal Coin Mint Tx
    UCOIN_BLOCK_REWARD_TX       = 13,   //!< Universal Coin Miner Block Reward Tx
    UCONTRACT_DEPLOY_TX         = 14,   //!< universal VM contract deployment
    UCONTRACT_INVOKE_TX         = 15,   //!< universal VM contract invocation
    PRICE_FEED_TX               = 16,   //!< Price Feed Tx: WICC/USD | WGRT/USD | WUSD/USD
    PRICE_MEDIAN_TX             = 17,   //!< Price Median Value on each block Tx
    UTXO_TRANSFER_TX            = 18,   //!< UTXO & HTLC Coin
    UTXO_PASSWORD_PROOF_TX      = 19,   //!< UTXO password proof

    CDP_STAKE_TX                = 21,   //!< CDP Staking/Restaking Tx
    CDP_REDEEM_TX               = 22,   //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX            = 23,   //!< CDP Liquidation Tx (partial or full)

    NICKID_REGISTER_TX          = 50,    //!< nickid register Tx
    ACCOUNT_PERMS_CLEAR_TX      = 51,

    WASM_CONTRACT_TX            = 60,   //!< wasm contract tx

    PROPOSAL_REQUEST_TX         = 70,
    PROPOSAL_APPROVAL_TX        = 71,

    DEX_TRADEPAIR_PROPOSE_TX    = 81,   //!< Owner proposes a trade pair on DEX
    DEX_TRADEPAIR_LIST_TX       = 82,   //!< Owner lists a trade pair on DEX
    DEX_TRADEPAIR_DELIST_TX     = 83,   //!< Owner or validators delist a trade pair
    DEX_LIMIT_BUY_ORDER_TX      = 84,   //!< dex buy limit price order Tx
    DEX_LIMIT_SELL_ORDER_TX     = 85,   //!< dex sell limit price order Tx
    DEX_MARKET_BUY_ORDER_TX     = 86,   //!< dex buy market price order Tx
    DEX_MARKET_SELL_ORDER_TX    = 87,   //!< dex sell market price order Tx
    DEX_CANCEL_ORDER_TX         = 88,   //!< dex cancel order Tx
    DEX_TRADE_SETTLE_TX         = 89,   //!< dex settle Tx

    DEX_ORDER_TX                = 90,   //!< dex common order tx, support BUY|SELL LIMIR|MARKET order
    DEX_OPERATOR_ORDER_TX       = 91,   //!< dex operator order tx, need dex operator signing

    DEX_OPERATOR_REGISTER_TX    = 100,  //!< dex operator register tx
    DEX_OPERATOR_UPDATE_TX      = 101, //!< dex operator update tx
};


struct TxTypeHash {
    size_t operator()(const TxType &type) const noexcept { return std::hash<uint8_t>{}(type); }
};

static const unordered_set<string> kFeeSymbolSet = {
    SYMB::WICC,
    SYMB::WUSD
};

inline string GetFeeSymbolSetStr() {
    string ret = "";
    for (auto symbol : kFeeSymbolSet) {
        if (ret.empty()) {
            ret = symbol;
        } else {
            ret += "|" + symbol;
        }
    }
    return ret;
}

/**
 * TxTypeKey -> {   TxTypeName,
 *                  InterimPeriodTxFees(WICC), EffectivePeriodTxFees(WICC),
 *                  InterimPeriodTxFees(WUSD), EffectivePeriodTxFees(WUSD)
 *              }
 *
 * Fees are boosted by COIN=10^8
 */
static const unordered_map<TxType, std::tuple<string, uint64_t, uint64_t, uint64_t, uint64_t, bool>, TxTypeHash> kTxFeeTable = {
/* tx type                                   tx type name               V1:WICC     V2:WICC    V1:WUSD     V2:WUSD     can_update      */
{ NULL_TX,                  std::make_tuple("NULL_TX",                  0,          0,          0,          0,           false ) },

{ BLOCK_REWARD_TX,          std::make_tuple("BLOCK_REWARD_TX",          0,          0,          0,          0,           false) }, //deprecated

{ ACCOUNT_REGISTER_TX,      std::make_tuple("ACCOUNT_REGISTER_TX",      0,          0.1*COIN,   0.1*COIN,   0.1*COIN    ,false) }, //deprecated
{ BCOIN_TRANSFER_TX,        std::make_tuple("BCOIN_TRANSFER_TX",        0,          0.1*COIN,   0.1*COIN,   0.1*COIN    ,false) }, //deprecated
{ LCONTRACT_DEPLOY_TX,      std::make_tuple("LCONTRACT_DEPLOY_TX",      1*COIN,     1*COIN,     1*COIN,     1*COIN      ,true) },
{ LCONTRACT_INVOKE_TX,      std::make_tuple("LCONTRACT_INVOKE_TX",      0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) }, //min fee
{ DELEGATE_VOTE_TX,         std::make_tuple("DELEGATE_VOTE_TX",         0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },

{ UCOIN_STAKE_TX,           std::make_tuple("UCOIN_STAKE_TX",           0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },

{ ASSET_ISSUE_TX,           std::make_tuple("ASSET_ISSUE_TX",           0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) }, //plus 550 WICC
{ UIA_UPDATE_TX,          std::make_tuple("UIA_UPDATE_TX",              0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) }, //plus 110 WICC
{ UCOIN_TRANSFER_TX,        std::make_tuple("UCOIN_TRANSFER_TX",        0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ UTXO_TRANSFER_TX,         std::make_tuple("UTXO_TRANSFER_TX",         0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ UTXO_PASSWORD_PROOF_TX,   std::make_tuple("UTXO_PASSWORD_PROOF_TX",   0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ UCOIN_MINT_TX,          std::make_tuple("UCOIN_MINT_TX",          0,          0,          0,          0           ,false) },
{ UCOIN_BLOCK_REWARD_TX,    std::make_tuple("UCOIN_BLOCK_REWARD_TX",    0,          0,          0,          0           ,false) },
{ ACCOUNT_PERMS_CLEAR_TX,   std::make_tuple("ACCOUNT_PERMS_CLEAR_TX",   0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },

{ UCONTRACT_DEPLOY_TX,      std::make_tuple("UCONTRACT_DEPLOY_TX",      0,          1*COIN,     1*COIN,     1*COIN      ,true) },
{ UCONTRACT_INVOKE_TX,      std::make_tuple("UCONTRACT_INVOKE_TX",      0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) }, //min fee
{ PRICE_FEED_TX,            std::make_tuple("PRICE_FEED_TX",            0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },

{ PRICE_MEDIAN_TX,          std::make_tuple("PRICE_MEDIAN_TX",          0,          0,          0,          0           ,false) },

{ CDP_STAKE_TX,             std::make_tuple("CDP_STAKE_TX",             0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },
{ CDP_REDEEM_TX,            std::make_tuple("CDP_REDEEM_TX",            0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },
{ CDP_LIQUIDATE_TX,         std::make_tuple("CDP_LIQUIDATE_TX",         0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },

{ DEX_LIMIT_BUY_ORDER_TX,   std::make_tuple("DEX_LIMIT_BUY_ORDER_TX",   0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ DEX_LIMIT_SELL_ORDER_TX,  std::make_tuple("DEX_LIMIT_SELL_ORDER_TX",  0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ DEX_MARKET_BUY_ORDER_TX,  std::make_tuple("DEX_MARKET_BUY_ORDER_TX",  0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ DEX_MARKET_SELL_ORDER_TX, std::make_tuple("DEX_MARKET_SELL_ORDER_TX", 0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },

{ DEX_OPERATOR_REGISTER_TX, std::make_tuple("DEX_OPERATOR_REGISTER_TX", 0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },
{ DEX_OPERATOR_UPDATE_TX,   std::make_tuple("DEX_OPERATOR_UPDATE_TX",   0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },
{ DEX_ORDER_TX,             std::make_tuple("DEX_ORDER_TX",             0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ DEX_OPERATOR_ORDER_TX,    std::make_tuple("DEX_OPERATOR_ORDER_TX",    0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ DEX_CANCEL_ORDER_TX,      std::make_tuple("DEX_CANCEL_ORDER_TX",      0,          0.001*COIN, 0.001*COIN, 0.001*COIN  ,true) },
{ DEX_TRADE_SETTLE_TX,      std::make_tuple("DEX_TRADE_SETTLE_TX",      0,          0.0001*COIN,0.0001*COIN,0.0001*COIN ,true) },

{ NICKID_REGISTER_TX,       std::make_tuple("NICKID_REGISTER_TX",       0,          0.0001*COIN,0.0001*COIN,0.0001*COIN ,true) },

{ WASM_CONTRACT_TX,         std::make_tuple("WASM_CONTRACT_TX",         0,          0.01*COIN,  0.01*COIN,  0.01*COIN   ,true) },

{ PROPOSAL_REQUEST_TX,      std::make_tuple("PROPOSAL_REQUEST_TX",      0,          0.01*COIN,  0.01*COIN,  0.01*COIN  ,true) },
{ PROPOSAL_APPROVAL_TX,     std::make_tuple("PROPOSAL_APPROVAL_TX",     0,          0.01*COIN,  0.01*COIN,  0.01*COIN  ,true) },

};

#endif