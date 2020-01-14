// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_SCOIN_H
#define CONFIG_SCOIN_H

#include <cstdint>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <tuple>
#include "const.h"

using namespace std;

static const uint16_t RATIO_BOOST          = 10000;
static const uint32_t PRICE_BOOST          = 100000000;
static const uint32_t CDP_BASE_RATIO_BOOST = 100000000;

static const uint64_t FUND_COIN_GENESIS_TOTAL_RELEASE_AMOUNT   = 20160000000;  // 96% * 21 billion
static const uint32_t FUND_COIN_GENESIS_INITIAL_RESERVE_AMOUNT = 1000000;      // 1 m WUSD

static const uint64_t FCOIN_VOTEMINE_EPOCH_FROM = 1665886560;  // Sun Oct 16 2022 10:16:00 GMT+0800
static const uint64_t FCOIN_VOTEMINE_EPOCH_TO   = 1792116960;  // Fri Oct 16 2026 10:16:00 GMT+0800

static const uint16_t FORCE_SETTLE_CDP_MAX_COUNT_PER_BLOCK = 1000;  // depends on TPS

static const double TRANSACTION_PRIORITY_CEILING      = 1000.0;  // Most trx priority is less than 1000.0
static const double PRICE_MEDIAN_TRANSACTION_PRIORITY = 10000.0;
static const double PRICE_FEED_TRANSACTION_PRIORITY   = 20000.0;

static const uint64_t ASSET_RISK_FEE_RATIO  = 4000;        // 40% * 10000, the ratio of asset fee into the risk reserve
static const uint64_t MIN_DEX_ORDER_AMOUNT  = 0.1 * COIN;  // min amount of dex order limit
static const uint64_t MAX_SETTLE_ITEM_COUNT = 10000;       // max count of dex settle item limit.


static const uint64_t DEX_OPERATOR_RISK_FEE_RATIO  = 4000; // 40% * 10000, the ratio of DEX operator fee into the risk reserve
static const uint64_t DEX_ORDER_FEE_RATIO_MAX = 50 * PRICE_BOOST;
static const uint64_t DEX_PRICE_MAX = 1000000 * PRICE_BOOST;

enum SysParamType : uint8_t {
    NULL_SYS_PARAM_TYPE                     = 0,
    MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT    = 1,
    PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN       = 2,
    PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX = 3,
    PRICE_FEED_DEVIATE_RATIO_MAX            = 4,
    PRICE_FEED_DEVIATE_PENALTY              = 5,
    CDP_SCOIN_RESERVE_FEE_RATIO             = 6,
    DEX_DEAL_FEE_RATIO                      = 7,
    CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT    = 8,
    CDP_GLOBAL_COLLATERAL_RATIO_MIN         = 9,
    CDP_START_COLLATERAL_RATIO              = 10,
    CDP_START_LIQUIDATE_RATIO               = 11,
    CDP_NONRETURN_LIQUIDATE_RATIO           = 12,
    CDP_FORCE_LIQUIDATE_RATIO               = 13,
    CDP_LIQUIDATE_DISCOUNT_RATIO            = 14,
    CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN   = 15,
    CDP_INTEREST_PARAM_A                    = 16,
    CDP_INTEREST_PARAM_B                    = 17,
    CDP_SYSORDER_PENALTY_FEE_MIN            = 18,
    ASSET_ISSUE_FEE                         = 19,
    ASSET_UPDATE_FEE                        = 20,
    DEX_OPERATOR_REGISTER_FEE               = 21,
    DEX_OPERATOR_UPDATE_FEE                 = 22,

};

static const unordered_map<string, string> paramNameToKeyMap = {
        {"MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT",           "A"},
        {"PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN",              "B"},
        {"PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX",        "C"},
        {"PRICE_FEED_DEVIATE_RATIO_MAX",                   "D"},
        {"PRICE_FEED_DEVIATE_PENALTY",                     "E"},
        {"DEX_DEAL_FEE_RATIO",                             "F"},
        {"CDP_SCOIN_RESERVE_FEE_RATIO",                    "G"},
        {"CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT",           "H"},
        {"CDP_GLOBAL_COLLATERAL_RATIO_MIN",                "I"},
        {"CDP_START_COLLATERAL_RATIO",                     "J"},
        {"CDP_START_LIQUIDATE_RATIO",                      "K"},
        {"CDP_NONRETURN_LIQUIDATE_RATIO",                  "L"},
        {"CDP_FORCE_LIQUIDATE_RATIO",                      "M"},
        {"CDP_LIQUIDATE_DISCOUNT_RATIO",                   "N"},
        {"CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN",          "O"},
        {"CDP_INTEREST_PARAM_A",                           "P"},
        {"CDP_INTEREST_PARAM_B",                           "Q"},
        {"CDP_SYSORDER_PENALTY_FEE_MIN",                   "R"},
        {"ASSET_ISSUE_FEE",                                "S"},
        {"ASSET_UPDATE_FEE",                               "T"},
        {"DEX_OPERATOR_REGISTER_FEE",                      "U"},
        {"DEX_OPERATOR_UPDATE_FEE",                        "V"}
   };


static const unordered_map<string, SysParamType> paramNameToSysParamTypeMap = {
        {"MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT",           MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT},
        {"PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN",              PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN},
        {"PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX",        PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX},
        {"PRICE_FEED_DEVIATE_RATIO_MAX",                   PRICE_FEED_DEVIATE_RATIO_MAX},
        {"PRICE_FEED_DEVIATE_PENALTY",                     PRICE_FEED_DEVIATE_PENALTY},
        {"DEX_DEAL_FEE_RATIO",                             DEX_DEAL_FEE_RATIO},
        {"CDP_SCOIN_RESERVE_FEE_RATIO",                    CDP_SCOIN_RESERVE_FEE_RATIO},
        {"CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT",           CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT},
        {"CDP_GLOBAL_COLLATERAL_RATIO_MIN",                CDP_GLOBAL_COLLATERAL_RATIO_MIN},
        {"CDP_START_COLLATERAL_RATIO",                     CDP_START_COLLATERAL_RATIO},
        {"CDP_START_LIQUIDATE_RATIO",                      CDP_START_LIQUIDATE_RATIO},
        {"CDP_NONRETURN_LIQUIDATE_RATIO",                  CDP_NONRETURN_LIQUIDATE_RATIO},
        {"CDP_FORCE_LIQUIDATE_RATIO",                      CDP_FORCE_LIQUIDATE_RATIO},
        {"CDP_LIQUIDATE_DISCOUNT_RATIO",                   CDP_LIQUIDATE_DISCOUNT_RATIO},
        {"CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN",          CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN},
        {"CDP_INTEREST_PARAM_A",                           CDP_INTEREST_PARAM_A},
        {"CDP_INTEREST_PARAM_B",                           CDP_INTEREST_PARAM_B},
        {"CDP_SYSORDER_PENALTY_FEE_MIN",                   CDP_SYSORDER_PENALTY_FEE_MIN},
        {"ASSET_ISSUE_FEE",                                ASSET_ISSUE_FEE},
        {"ASSET_UPDATE_FEE",                               ASSET_UPDATE_FEE},
        {"DEX_OPERATOR_REGISTER_FEE",                      DEX_OPERATOR_REGISTER_FEE},
        {"DEX_OPERATOR_UPDATE_FEE",                        DEX_OPERATOR_UPDATE_FEE}
};

struct SysParamTypeHash {
    size_t operator()(const SysParamType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};

static const unordered_map<SysParamType, std::tuple<string, uint64_t>, SysParamTypeHash> SysParamTable = {
    { MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT,         std::make_tuple("A",    11)         },
    { PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN,            std::make_tuple("B",    210000)     },  // 1%: min 210K bcoins staked to be a price feeder for miner
    { PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX,      std::make_tuple("C",    10)         },  // after 10 times continuous deviate limit penetration all deposit be deducted
    { PRICE_FEED_DEVIATE_RATIO_MAX,                 std::make_tuple("D",    3000)       },  // must be < 30% * 10000, otherwise penalized
    { PRICE_FEED_DEVIATE_PENALTY,                   std::make_tuple("E",    1000)       },  // deduct 1000 staked bcoins as penalty
    { CDP_SCOIN_RESERVE_FEE_RATIO,                  std::make_tuple("G",    0)          },  // WUSD friction fee to risk reserve
    { CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT,         std::make_tuple("H",    52500000)   },  // 25% * 210000000
    { CDP_GLOBAL_COLLATERAL_RATIO_MIN,              std::make_tuple("I",    8000)       },  // 80% * 10000
    { CDP_START_COLLATERAL_RATIO,                   std::make_tuple("J",    19000)      },  // 190% * 10000 : starting collateral ratio
    { CDP_START_LIQUIDATE_RATIO,                    std::make_tuple("K",    15000)      },  // 1.13 ~ 1.5  : common liquidation
    { CDP_NONRETURN_LIQUIDATE_RATIO,                std::make_tuple("L",    11300)      },  // 1.04 ~ 1.13 : Non-return to CDP owner
    { CDP_FORCE_LIQUIDATE_RATIO,                    std::make_tuple("M",    10400)      },  // 0 ~ 1.04    : forced liquidation only
    { CDP_LIQUIDATE_DISCOUNT_RATIO,                 std::make_tuple("N",    9700)       },  // discount: 97%
    { CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,        std::make_tuple("O",    90000000)   },  // 0.9 WUSD, dust amount (<0.9) rejected
    { CDP_INTEREST_PARAM_A,                         std::make_tuple("P",    2)          },  // a = 2
    { CDP_INTEREST_PARAM_B,                         std::make_tuple("Q",    1)          },  // b = 1
    { CDP_SYSORDER_PENALTY_FEE_MIN,                 std::make_tuple("R",    10)         },  // min penalty fee = 10
    { ASSET_ISSUE_FEE,                              std::make_tuple("S",    550 * COIN) },  // asset issuance fee = 550 WICC
    { ASSET_UPDATE_FEE,                             std::make_tuple("T",    110 * COIN) },  // asset update fee = 110 WICC
    { DEX_OPERATOR_REGISTER_FEE,                    std::make_tuple("U",    1100 * COIN) }, // dex operator register fee = 1100 WICC
    { DEX_OPERATOR_UPDATE_FEE,                      std::make_tuple("V",    110 * COIN) },  // dex operator update fee = 110 WICC
    { DEX_DEAL_FEE_RATIO,                           std::make_tuple("F",    40000)      },  // 0.04% * 100000000

};

#endif
