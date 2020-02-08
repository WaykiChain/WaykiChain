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
#include "commons/types.h"
#include "commons/serialize.h"
#include "commons/util/util.h"

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
static const uint64_t DEX_OPERATOR_FEE_RATIO_MAX = 50 * PRICE_BOOST;
static const uint64_t DEX_PRICE_MAX = 1000000 * PRICE_BOOST;


enum CdpParamType : uint8_t {
    NULL_CDP_PARAM_TYPE                     = 0,
    CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT    = 2,
    CDP_GLOBAL_COLLATERAL_RATIO_MIN         = 3,
    CDP_START_COLLATERAL_RATIO              = 4,
    CDP_START_LIQUIDATE_RATIO               = 5,
    CDP_NONRETURN_LIQUIDATE_RATIO           = 6,
    CDP_FORCE_LIQUIDATE_RATIO               = 7,
    CDP_LIQUIDATE_DISCOUNT_RATIO            = 8,
    CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN   = 9,
    CDP_INTEREST_PARAM_A                    = 10,
    CDP_INTEREST_PARAM_B                    = 11,
    CDP_SYSORDER_PENALTY_FEE_MIN            = 12,

};



static const unordered_map<string, std::tuple<string,CdpParamType>> paramNameToCdpParamTypeMap = {

        {"CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT",           make_tuple("B", CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT)    },
        {"CDP_GLOBAL_COLLATERAL_RATIO_MIN",                make_tuple("C", CDP_GLOBAL_COLLATERAL_RATIO_MIN)         },
        {"CDP_START_COLLATERAL_RATIO",                     make_tuple("D", CDP_START_COLLATERAL_RATIO)              },
        {"CDP_START_LIQUIDATE_RATIO",                      make_tuple("E", CDP_START_LIQUIDATE_RATIO)               },
        {"CDP_NONRETURN_LIQUIDATE_RATIO",                  make_tuple("F", CDP_NONRETURN_LIQUIDATE_RATIO)           },
        {"CDP_FORCE_LIQUIDATE_RATIO",                      make_tuple("G", CDP_FORCE_LIQUIDATE_RATIO)               },
        {"CDP_LIQUIDATE_DISCOUNT_RATIO",                   make_tuple("H", CDP_LIQUIDATE_DISCOUNT_RATIO)            },
        {"CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN",          make_tuple("I", CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN)   },
        {"CDP_INTEREST_PARAM_A",                           make_tuple("J", CDP_INTEREST_PARAM_A)                    },
        {"CDP_INTEREST_PARAM_B",                           make_tuple("K", CDP_INTEREST_PARAM_B)                    },
        {"CDP_SYSORDER_PENALTY_FEE_MIN",                   make_tuple("L", CDP_SYSORDER_PENALTY_FEE_MIN)            }
};

struct CdpParamTypeHash {
    size_t operator()(const CdpParamType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};

static const unordered_map<CdpParamType, std::tuple<string, uint64_t,string >, CdpParamTypeHash> CdpParamTable = {
        { CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT,     make_tuple("B",  52500000,     "CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT")    },  // 25% * 210000000
        { CDP_GLOBAL_COLLATERAL_RATIO_MIN,          make_tuple("C",  8000,         "CDP_GLOBAL_COLLATERAL_RATIO_MIN")         },  // 80% * 10000
        { CDP_START_COLLATERAL_RATIO,               make_tuple("D",  19000,        "CDP_START_COLLATERAL_RATIO")              },  // 190% * 10000 : starting collateral ratio
        { CDP_START_LIQUIDATE_RATIO,                make_tuple("E",  15000,        "CDP_START_LIQUIDATE_RATIO")               },  // 1.13 ~ 1.5  : common liquidation
        { CDP_NONRETURN_LIQUIDATE_RATIO,            make_tuple("F",  11300,        "CDP_NONRETURN_LIQUIDATE_RATIO")           },  // 1.04 ~ 1.13 : Non-return to CDP owner
        { CDP_FORCE_LIQUIDATE_RATIO,                make_tuple("G",  10400,        "CDP_FORCE_LIQUIDATE_RATIO")               },  // 0 ~ 1.04    : forced liquidation only
        { CDP_LIQUIDATE_DISCOUNT_RATIO,             make_tuple("H",  9700,         "CDP_LIQUIDATE_DISCOUNT_RATIO")            },  // discount: 97%
        { CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,    make_tuple("I",  90000000,     "CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN")   },  // 0.9 WUSD, dust amount (<0.9) rejected
        { CDP_INTEREST_PARAM_A,                     make_tuple("J",  2,            "CDP_INTEREST_PARAM_A")                    },  // a = 2
        { CDP_INTEREST_PARAM_B,                     make_tuple("K",  1,            "CDP_INTEREST_PARAM_B")                    },  // b = 1
        { CDP_SYSORDER_PENALTY_FEE_MIN,             make_tuple("L",  10,           "CDP_SYSORDER_PENALTY_FEE_MIN")            }  // min penalty fee = 10
};

inline const string& GetCdpParamName(CdpParamType paramType) {
    auto it = CdpParamTable.find(paramType);
    if (it != CdpParamTable.end())
        return std::get<2>(it->second);
}

class CCdpCoinPair {
public:
    TokenSymbol bcoin_symbol;
    TokenSymbol scoin_symbol;

public:
    CCdpCoinPair() {}

    CCdpCoinPair(const TokenSymbol& bcoinSymbol, const TokenSymbol& scoinSymbol) :
            bcoin_symbol(bcoinSymbol), scoin_symbol(scoinSymbol) {}

    IMPLEMENT_SERIALIZE(
            READWRITE(bcoin_symbol);
            READWRITE(scoin_symbol);
    )

    friend bool operator<(const CCdpCoinPair& a, const CCdpCoinPair& b) {
        return a.bcoin_symbol < b.bcoin_symbol || a.scoin_symbol < b.scoin_symbol;
    }

    friend bool operator==(const CCdpCoinPair& a , const CCdpCoinPair& b) {
        return a.bcoin_symbol == b.bcoin_symbol && a.scoin_symbol == b.scoin_symbol;
    }

    string ToString() const {
        return strprintf("%s-%s", bcoin_symbol, scoin_symbol);
    }

    bool IsEmpty() const { return bcoin_symbol.empty() && scoin_symbol.empty(); }

    void SetEmpty() {
        bcoin_symbol.clear();
        scoin_symbol.clear();
    }
};

#endif
