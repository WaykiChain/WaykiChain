// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_CDPPARAMS_H
#define CONFIG_CDPPARAMS_H

#include <cstdint>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <tuple>
#include "const.h"

using namespace std;

enum CdpParamType : uint8_t {
    NULL_CDP_PARAM_TYPE                     = 0,
    CDP_SCOIN_RESERVE_FEE_RATIO             = 1,
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



static const unordered_map<string, std::tuple<string,SysParamType>> paramNameToCdpParamTypeMap = {

    {"CDP_SCOIN_RESERVE_FEE_RATIO",                    make_tuple("A", CDP_SCOIN_RESERVE_FEE_RATIO)             },
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
    size_t operator()(const SysParamType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};

static const unordered_map<SysParamType, std::tuple<string, uint64_t,string >, CdpParamTypeHash> CdpParamTypeHash = {
    { CDP_SCOIN_RESERVE_FEE_RATIO,              make_tuple("A",  0,            "CDP_SCOIN_RESERVE_FEE_RATIO")             },  // WUSD friction fee to risk reserve
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

#endif
