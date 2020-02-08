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

enum SysParamType : uint8_t {
    NULL_SYS_PARAM_TYPE                     = 0,
    MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT    = 1,
    PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN       = 2,
    PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX = 3,
    PRICE_FEED_DEVIATE_RATIO_MAX            = 4,
    PRICE_FEED_DEVIATE_PENALTY              = 5,
    DEX_DEAL_FEE_RATIO                      = 7,
    ASSET_ISSUE_FEE                         = 19,
    ASSET_UPDATE_FEE                        = 20,
    DEX_OPERATOR_REGISTER_FEE               = 21,
    DEX_OPERATOR_UPDATE_FEE                 = 22,
    PROPOSAL_EXPIRE_BLOCK_COUNT             = 23,
    TOTAL_DELEGATE_COUNT                    = 24,

    TRANSFER_SCOIN_RESERVE_FEE_RATIO        = 25,

};



static const unordered_map<string, std::tuple<string,SysParamType>> paramNameToSysParamTypeMap = {
        {"MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT",           make_tuple("A", MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT)    },
        {"PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN",              make_tuple("B", PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN)       },
        {"PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX",        make_tuple("C", PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX) },
        {"PRICE_FEED_DEVIATE_RATIO_MAX",                   make_tuple("D", PRICE_FEED_DEVIATE_RATIO_MAX)            },
        {"PRICE_FEED_DEVIATE_PENALTY",                     make_tuple("E", PRICE_FEED_DEVIATE_PENALTY)              },
        {"DEX_DEAL_FEE_RATIO",                             make_tuple("F", DEX_DEAL_FEE_RATIO)                      },
        {"ASSET_ISSUE_FEE",                                make_tuple("S", ASSET_ISSUE_FEE)                         },
        {"ASSET_UPDATE_FEE",                               make_tuple("T", ASSET_UPDATE_FEE)                        },
        {"DEX_OPERATOR_REGISTER_FEE",                      make_tuple("U", DEX_OPERATOR_REGISTER_FEE)               },
        {"DEX_OPERATOR_UPDATE_FEE",                        make_tuple("V", DEX_OPERATOR_UPDATE_FEE)                 },
        {"PROPOSAL_EXPIRE_BLOCK_COUNT",                    make_tuple("W", PROPOSAL_EXPIRE_BLOCK_COUNT)             },
        {"TOTAL_DELEGATE_COUNT",                           make_tuple("X", TOTAL_DELEGATE_COUNT)                    },
        {"TRANSFER_SCOIN_RESERVE_FEE_RATIO",               make_tuple("Y", TRANSFER_SCOIN_RESERVE_FEE_RATIO)        },
};

struct SysParamTypeHash {
    size_t operator()(const SysParamType &type) const noexcept {
        return std::hash<uint8_t>{}(type);
    }
};

static const unordered_map<SysParamType, std::tuple<string, uint64_t,string >, SysParamTypeHash> SysParamTable = {
        { MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT,     make_tuple("A",  11,           "MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT")    },
        { PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN,        make_tuple("B",  210000,       "PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN")       },  // 1%: min 210K bcoins staked to be a price feeder for miner
        { PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX,  make_tuple("C",  10,           "PRICE_FEED_CONTINUOUS_DEVIATE_TIMES_MAX") },  // after 10 times continuous deviate limit penetration all deposit be deducted
        { PRICE_FEED_DEVIATE_RATIO_MAX,             make_tuple("D",  3000,         "PRICE_FEED_DEVIATE_RATIO_MAX")            },  // must be < 30% * 10000, otherwise penalized
        { PRICE_FEED_DEVIATE_PENALTY,               make_tuple("E",  1000,         "PRICE_FEED_DEVIATE_PENALTY")              },  // deduct 1000 staked bcoins as penalty
        { DEX_DEAL_FEE_RATIO,                       make_tuple("F",  40000,        "DEX_DEAL_FEE_RATIO")                      },  // 0.04% * 100000000
        { ASSET_ISSUE_FEE,                          make_tuple("S",  550 * COIN,   "ASSET_ISSUE_FEE")                         },  // asset issuance fee = 550 WICC
        { ASSET_UPDATE_FEE,                         make_tuple("T",  110 * COIN,   "ASSET_UPDATE_FEE")                        },  // asset update fee = 110 WICC
        { DEX_OPERATOR_REGISTER_FEE,                make_tuple("U",  1100 * COIN,  "DEX_OPERATOR_REGISTER_FEE")               }, // dex operator register fee = 1100 WICC
        { DEX_OPERATOR_UPDATE_FEE,                  make_tuple("V",  110 * COIN,   "DEX_OPERATOR_UPDATE_FEE")                 },  // dex operator update fee = 110 WICC
        { PROPOSAL_EXPIRE_BLOCK_COUNT,              make_tuple("W",  1200,         "PROPOSAL_EXPIRE_BLOCK_COUNT")             },   //
        { TOTAL_DELEGATE_COUNT,                     make_tuple("X",  11,           "TOTAL_DELEGATE_COUNT")                    },
        { TRANSFER_SCOIN_RESERVE_FEE_RATIO,         make_tuple("Y",  0,            "TRANSFER_SCOIN_RESERVE_FEE_RATIO")        },  // WUSD friction fee to risk reserve
};


#endif
