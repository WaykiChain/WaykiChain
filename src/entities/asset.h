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

typedef string TokenSymbol;     //8 chars max, E.g. WICC, WCNY, WICC-01D
typedef string TokenName;       //32 chars max, E.g. WaykiChain Coins

class CAssetTradingPair {
public:
    TokenSymbol base_asset_symbol;
    TokenSymbol quote_asset_symbol;

public:
    string ToString() {
        return strprintf("%s-%s", base_asset_symbol, quote_asset_symbol);
    }
};

class CAsset {
public:
    CRegID      ownerRegId;     // creator or owner of the asset
    TokenSymbol symbol;         // asset symbol, E.g WICC | WUSD
    TokenName   name;           // asset long name, E.g WaykiChain coin
    bool        mintable;       // whether this token can be minted in the future.
    uint64_t    totalSupply;    // boosted by 1e8 for the decimal part, max is 90 billion.

    mutable uint256 sigHash;  //!< in-memory only

public:
    CAsset(CRegID ownerRegIdIn, TokenSymbol symbolIn, TokenName nameIn, bool mintableIn, uint64_t totalSupplyIn) :
        ownerRegId(ownerRegIdIn), symbol(symbolIn), name(nameIn), mintable(mintableIn), totalSupply(totalSupplyIn) {};

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << ownerRegId << symbol << name << mintable << VARINT(totalSupply);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

     IMPLEMENT_SERIALIZE(
        READWRITE(ownerRegId);
        READWRITE(symbol);
        READWRITE(name);
        READWRITE(mintable);
        READWRITE(VARINT(totalSupply));)
};


#endif //ACCOUNTS_ASSET_H