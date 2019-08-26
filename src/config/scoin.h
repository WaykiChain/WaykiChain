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

using namespace std;

static const uint16_t kPercentBoost                         = 10000;
static const uint32_t kTotalFundCoinGenesisReleaseAmount    = 10500000; // 21/2 million WGRT

static const uint16_t kFcoinGenesisIssueTxIndex             = 1;
static const uint16_t kFcoinGenesisRegisterTxIndex          = 2;
static const uint16_t kDexMatchSvcRegisterTxIndex           = 3;

static const uint16_t kForceSettleCDPMaxCountPerBlock       = 1000;     // depends on TPS

// Except specific transactions, priority is less than 1000.0
static const double kTransactionPriorityCeiling             = 1000.0;
static const double kPriceFeedTransactionPriority           = 10000.0;

static const uint64_t ASSET_RISK_FEE_RATIO = 4000; // 40% * 10000, the ratio of asset fee into the risk riserve

enum SysParamType : uint8_t {
    NULL_SYS_PARAM_TYPE                     = 0,
    MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT    = 1,
    PRICE_FEED_FCOIN_STAKE_AMOUNT_MIN       = 2,
    PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX = 3,
    PRICE_FEED_DEVIATE_RATIO_MAX            = 4,
    PRICE_FEED_DEVIATE_PENALTY              = 5,
    SCOIN_RESERVE_FEE_RATIO                 = 6,
    DEX_DEAL_FEE_RATIO                      = 7,
    GLOBAL_COLLATERAL_CEILING_AMOUNT        = 8,
    GLOBAL_COLLATERAL_RATIO_MIN             = 9,
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

};

struct SysParamTypeHash {
    size_t operator()(const SysParamType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};


static const unordered_map<SysParamType, std::tuple<string, uint64_t>, SysParamTypeHash> SysParamTable = {
    { MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT,         std::make_tuple("A",    11)         },
    { PRICE_FEED_FCOIN_STAKE_AMOUNT_MIN,            std::make_tuple("B",    210000)     },  // 1%: min 210K fcoins deposited to be a price feeder
    { PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX,      std::make_tuple("C",    10)         },  // after 10 times continuous deviate limit penetration all deposit be deducted
    { PRICE_FEED_DEVIATE_RATIO_MAX,                 std::make_tuple("D",    3000)       },  // must be < 30% * 10000, otherwise penalized
    { PRICE_FEED_DEVIATE_PENALTY,                   std::make_tuple("E",    1000)       },  // deduct 1000 staked bcoins as penalty
    { DEX_DEAL_FEE_RATIO,                           std::make_tuple("F",    4)          },  // 0.04% * 10000
    { SCOIN_RESERVE_FEE_RATIO,                      std::make_tuple("G",    0)          },  // WUSD friction fee to risk reserve
    { GLOBAL_COLLATERAL_CEILING_AMOUNT,             std::make_tuple("H",    21000000)   },  // 10% * 210000000
    { GLOBAL_COLLATERAL_RATIO_MIN,                  std::make_tuple("I",    8000)       },  // 80% * 10000
    { CDP_START_COLLATERAL_RATIO,                   std::make_tuple("J",    19000)      },  // 190% * 10000: starting collateral ratio
    { CDP_START_LIQUIDATE_RATIO,                    std::make_tuple("K",    15000)      },  // 1.13 ~ 1.5  : common liquidation
    { CDP_NONRETURN_LIQUIDATE_RATIO,                std::make_tuple("L",    11300)      },  // 1.04 ~ 1.13 : Non-return to CDP owner
    { CDP_FORCE_LIQUIDATE_RATIO,                    std::make_tuple("M",    10400)      },  // 0 ~ 1.04    : forced liquidation only
    { CDP_LIQUIDATE_DISCOUNT_RATIO,                 std::make_tuple("N",    9700)       },  // discount: 97%
    { CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,        std::make_tuple("O",    90000000)   },  // 0.9 WUSD, dust amount (<0.9) rejected
    { CDP_INTEREST_PARAM_A,                         std::make_tuple("P",    2)          },  // a = 2
    { CDP_INTEREST_PARAM_B,                         std::make_tuple("Q",    1)          },  // b = 1
    { CDP_SYSORDER_PENALTY_FEE_MIN,                 std::make_tuple("R",    10)         },  // min penalty fee = 10
    { ASSET_ISSUE_FEE,                              std::make_tuple("S",    550)        },  // asset issue fee = 550
    { ASSET_UPDATE_FEE,                             std::make_tuple("T",    110)        },  // asset update fee = 110

};

#endif
