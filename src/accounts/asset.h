// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ACCOUNTS_ASSET_H
#define ACCOUNTS_ASSET_H

#include <boost/variant.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

typedef string TokenSymbol;

class CAsset {
public:
    CRegID ownerRegID;  // creator or owner of the asset
    TokenSymbol symbol;      // asset symbol, E.g WICC | WUSD
    string name;        // asset long name, E.g WaykiChain coin
    bool mintable;      // whether this token can be minted in the future.

    uint64_t totalSupply;   // boosted by 1e8 for the decimal part, max is 90 billion.
};


#endif //ACCOUNTS_ASSET_H