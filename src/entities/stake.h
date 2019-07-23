// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_STAKE_H
#define ENTITIES_STAKE_H

#include <string>
#include <unordered_map>

using namespace std;

enum StakeType : uint8_t {
    NULL_STAKE  = 0,  //!< invalid stake operate
    ADD_COIN    = 1,  //!< add operate
    MINUS_COIN  = 2,  //!< minus operate
};

static const unordered_map<unsigned char, string> kStakeTypeMap = {
    {NULL_STAKE,    "NULL_STAKE"},
    {ADD_COIN,      "ADD_COIN"},
    {ADD_COIN,      "ADD_COIN"},
};

inline const string& GetStakeTypeName(const StakeType stakeType) {
    return kStakeTypeMap.at(stakeType);
}

#endif  // ENTITIES_STAKE_H