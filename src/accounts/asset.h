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
typedef string TokenName;

class CAsset {
public:
    CRegID ownerRegId;  // creator or owner of the asset
    TokenSymbol symbol;      // asset symbol, E.g WICC | WUSD
    TokenName name;        // asset long name, E.g WaykiChain coin
    bool mintable;      // whether this token can be minted in the future.
    uint64_t totalSupply;   // boosted by 1e8 for the decimal part, max is 90 billion.

public:
    CAsset(CRegID ownerRegIdIn, TokenSymbol symbolIn, TokenName nameIn, bool mintableIn, uint64_t totalSupplyIn) :
        owerRegId(ownerRegIdIn), symbol(symbolIn), name(nameIn), mintable(mintableIn), totalSupply(totalSupplyIn) {};
};


#endif //ACCOUNTS_ASSET_H