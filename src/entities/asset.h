// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_ASSET_H
#define ENTITIES_ASSET_H

#include <boost/variant.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "commons/types.h"
#include "config/const.h"
#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

typedef string TokenSymbol;     //8 chars max, E.g. WICC, WCNY, WICC-01D
typedef string TokenName;       //32 chars max, E.g. WaykiChain Coins
typedef std::pair<TokenSymbol, TokenSymbol> CoinPricePair;
typedef std::pair<TokenSymbol, TokenSymbol> TradingPair;
typedef string AssetSymbol;     //8 chars max, E.g. WICC
typedef string PriceSymbol;     //8 chars max, E.g. USD, CNY, EUR, BTC

static const unordered_set<string> kCoinTypeSet = {
    SYMB::WICC, SYMB::WGRT, SYMB::WUSD
};

static const unordered_set<string> kPriceTypeSet = {
    SYMB::USD, SYMB::CNY, SYMB::EUR, SYMB::BTC, SYMB::USDT, SYMB::GOLD, SYMB::KWH
};


static const UnorderedPairSet<TokenSymbol, TokenSymbol> kTradingPairSet = {
    {SYMB::WICC, SYMB::WUSD},
    {SYMB::WGRT, SYMB::WUSD}
};

class CAssetTradingPair {
public:
    TokenSymbol base_asset_symbol;
    TokenSymbol quote_asset_symbol;

public:
    CAssetTradingPair(const TokenSymbol& baseSymbol, const TokenSymbol& quoteSymbol) :
        base_asset_symbol(baseSymbol), quote_asset_symbol(quoteSymbol) {}

    string ToString() {
        return strprintf("%s-%s", base_asset_symbol, quote_asset_symbol);
    }
};

class CAsset {
public:
    CRegID      owner_regid;    // creator or owner of the asset
    TokenSymbol symbol;         // asset symbol, E.g WICC | WUSD
    TokenName   name;           // asset long name, E.g WaykiChain coin
    bool        mintable;       // whether this token can be minted in the future.
    uint64_t    total_supply;    // boosted by 1e8 for the decimal part, max is 90 billion.

    mutable uint256 sigHash;  //!< in-memory only

public:
    CAsset(CRegID ownerRegIdIn, TokenSymbol symbolIn, TokenName nameIn, bool mintableIn, uint64_t totalSupplyIn) :
        owner_regid(ownerRegIdIn), symbol(symbolIn), name(nameIn), mintable(mintableIn), total_supply(totalSupplyIn) {};

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << owner_regid << symbol << name << mintable << VARINT(total_supply);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

     IMPLEMENT_SERIALIZE(
        READWRITE(owner_regid);
        READWRITE(symbol);
        READWRITE(name);
        READWRITE(mintable);
        READWRITE(VARINT(total_supply));)
};

#endif //ENTITIES_ASSET_H
