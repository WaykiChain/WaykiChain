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

#define RANGE(MIN,MAX) make_pair(MIN, MAX)

static const uint64_t PRICE_BOOST          = 100000000;
static const uint64_t CDP_BASE_RATIO_BOOST = 100000000;

static const uint64_t FUND_COIN_GENESIS_TOTAL_RELEASE_AMOUNT   = 20160000000;  // 96% * 21 billion
static const uint64_t FUND_COIN_GENESIS_INITIAL_RESERVE_AMOUNT = 1000000;      // 1 m WUSD

static const uint64_t FCOIN_VOTEMINE_EPOCH_FROM = 1665886560;  // Sun Oct 16 2022 10:16:00 GMT+0800
static const uint64_t FCOIN_VOTEMINE_EPOCH_TO   = 1792116960;  // Fri Oct 16 2026 10:16:00 GMT+0800

static const uint64_t CDP_FORCE_LIQUIDATE_MAX_COUNT = 1000;  // depends on TPS
static const uint64_t CDP_SETTLE_INTEREST_MAX_COUNT = 100;

static const uint64_t CDP_SYSORDER_PENALTY_FEE_MIN = 10;

static const double TRANSACTION_PRIORITY_CEILING      = 1000.0;  // Most trx priority is less than 1000.0
static const double PRICE_MEDIAN_TRANSACTION_PRIORITY = 10000.0;
static const double PRICE_FEED_TRANSACTION_PRIORITY   = 20000.0;

static const uint64_t MIN_DEX_ORDER_AMOUNT  = 0.1 * COIN;  // min amount of dex order limit
static const uint64_t MAX_SETTLE_ITEM_COUNT = 10000;       // max count of dex settle item limit.


static const uint64_t DEX_OPERATOR_FEE_RATIO_MAX = 0.5 * PRICE_BOOST; // 50%
static const uint64_t DEX_PRICE_MAX = 1000000 * PRICE_BOOST;
static const uint64_t MIN_STAKED_WICC_FOR_STEP_FEE = 1000;

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
    CDP_CONVERT_INTEREST_TO_DEBT_DAYS       = 13, //beyond this will result in force converting interest to debt

};



static const unordered_map<string, CdpParamType> paramNameToCdpParamTypeMap = {
    {"CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT",            CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT    },
    {"CDP_GLOBAL_COLLATERAL_RATIO_MIN",                 CDP_GLOBAL_COLLATERAL_RATIO_MIN         },
    {"CDP_START_COLLATERAL_RATIO",                      CDP_START_COLLATERAL_RATIO              },
    {"CDP_START_LIQUIDATE_RATIO",                       CDP_START_LIQUIDATE_RATIO               },
    {"CDP_NONRETURN_LIQUIDATE_RATIO",                   CDP_NONRETURN_LIQUIDATE_RATIO           },
    {"CDP_FORCE_LIQUIDATE_RATIO",                       CDP_FORCE_LIQUIDATE_RATIO               },
    {"CDP_LIQUIDATE_DISCOUNT_RATIO",                    CDP_LIQUIDATE_DISCOUNT_RATIO            },
    {"CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN",           CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN   },
    {"CDP_INTEREST_PARAM_A",                            CDP_INTEREST_PARAM_A                    },
    {"CDP_INTEREST_PARAM_B",                            CDP_INTEREST_PARAM_B                    },
    {"CDP_CONVERT_INTEREST_TO_DEBT_DAYS",               CDP_CONVERT_INTEREST_TO_DEBT_DAYS       }
};

struct CdpParamTypeHash {
    size_t operator()(const CdpParamType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};

static const unordered_map<CdpParamType, std::tuple< uint64_t,string, string >, CdpParamTypeHash> kCdpParamTable = {
        //         type                          default_value  name                                    unit_type
        { CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT,  {52500000,  "CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT"  , "WI"     }},  // 25% * 210000000
        { CDP_GLOBAL_COLLATERAL_RATIO_MIN,       {8000,      "CDP_GLOBAL_COLLATERAL_RATIO_MIN"       , "RATIO"  }},  // 80% * 10000
        { CDP_START_COLLATERAL_RATIO,            {19000,     "CDP_START_COLLATERAL_RATIO"             , "RATIO" }},  // 190% * 10000 : starting collateral ratio
        { CDP_START_LIQUIDATE_RATIO,             {15000,     "CDP_START_LIQUIDATE_RATIO"             , "RATIO"  }},  // 1.13 ~ 1.5  : common liquidation
        { CDP_NONRETURN_LIQUIDATE_RATIO,         {11300,     "CDP_NONRETURN_LIQUIDATE_RATIO"         , "RATIO"  }},  // 1.04 ~ 1.13 : Non-return to CDP owner
        { CDP_FORCE_LIQUIDATE_RATIO,             {10400,     "CDP_FORCE_LIQUIDATE_RATIO"             , "RATIO"  }},  // 0 ~ 1.04    : forced liquidation only
        { CDP_LIQUIDATE_DISCOUNT_RATIO,          {9700,      "CDP_LIQUIDATE_DISCOUNT_RATIO"          , "RATIO"  }},  // discount: 97%
        { CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN, {90000000,  "CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN" , "SAWI"   }},  // 0.9 WUSD, dust amount (<0.9) rejected
        { CDP_INTEREST_PARAM_A,                  {2,         "CDP_INTEREST_PARAM_A"                  , "INT"    }},  // a = 2
        { CDP_INTEREST_PARAM_B,                  {1,         "CDP_INTEREST_PARAM_B"                  , "INT"    }},  // min penalty fee = 10
        { CDP_CONVERT_INTEREST_TO_DEBT_DAYS,     {30,        "CDP_CONVERT_INTEREST_TO_DEBT_DAYS"     , "INT"    }}   // after 30 days, unpaid interest will be converted into debt
};

static const unordered_map<CdpParamType, std::pair<uint64_t,uint64_t>, CdpParamTypeHash> kCdpParamRangeTable = {
        { CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT,     RANGE(0, 210000000)  },  // 0 ~ 210000000 : must <= total of WICC (WI)
        { CDP_GLOBAL_COLLATERAL_RATIO_MIN,          RANGE(0, 1000000)    },  // 0% ~ 10000%
        { CDP_START_COLLATERAL_RATIO,               RANGE(10000, 1000000)},  // 100% * 10000% : starting collateral ratio
        { CDP_START_LIQUIDATE_RATIO,                RANGE(10000, 100000) },  // 100% ~ 1000%  : common liquidation
        { CDP_NONRETURN_LIQUIDATE_RATIO,            RANGE(10000, 100000) },  // 100% ~ 1000% : Non-return to CDP owner
        { CDP_FORCE_LIQUIDATE_RATIO,                RANGE(0, 20000)      },  // 0% ~ 200%    : forced liquidation only
        { CDP_LIQUIDATE_DISCOUNT_RATIO,             RANGE(0, 10000)      },  // 0% ~ 100%
        { CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,    RANGE(0, 0)          },  //
        { CDP_INTEREST_PARAM_A,                     RANGE(0, 0)          },  //
        { CDP_INTEREST_PARAM_B,                     RANGE(1, 100000000)  },  // 1 ~ 100000000
        { CDP_CONVERT_INTEREST_TO_DEBT_DAYS,        RANGE(0, 3650)       },  // 0 ~ 10 years
};

inline bool CheckCdpParamValue(const CdpParamType paramType, uint64_t value, string &errMsg) {

    if (kCdpParamRangeTable.count(paramType) == 0) {
        errMsg = strprintf("check param scope error:don't find param type (%d)", paramType);
        return false;
    }

    auto itr = kCdpParamRangeTable.find(paramType);
    auto min = std::get<0>(itr->second);
    auto max = std::get<1>(itr->second);

    if (min == 0 && max == 0)
        return true;

    if (value < min || value >max) {
        errMsg = strprintf("check param scope error: the scope of param type(%d) "
                         "is [%d,%d],but the value you submited is %d", paramType, min, max, value);

        return false;
    }

    return true;
}

inline uint64_t GetCdpParamDefaultValue(CdpParamType paramType) {
    auto it = kCdpParamTable.find(paramType);
    if (it != kCdpParamTable.end())
        return std::get<0>(it->second);
    return 0;
}

inline const string& GetCdpParamName(CdpParamType paramType) {
    auto it = kCdpParamTable.find(paramType);
    if (it != kCdpParamTable.end())
        return std::get<1>(it->second);
    return EMPTY_STRING;
}

inline CdpParamType  GetCdpParamType(const string  paramName){
    auto itr = paramNameToCdpParamTypeMap.find(paramName);
    if(itr == paramNameToCdpParamTypeMap.end())
        return NULL_CDP_PARAM_TYPE;
    else
        return itr->second;

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
