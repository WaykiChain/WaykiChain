// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_ASSET_H
#define ENTITIES_ASSET_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "commons/types.h"
#include "config/const.h"
#include "config/configuration.h"
#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "commons/json/json_spirit_utils.h"

using namespace json_spirit;
using namespace std;

// asset types
enum AssetType : uint8_t {
    NULL_ASSET          = 0,
    NIA                 = 1, //Natively Issued Asset
    DIA                 = 2, //DeGov Issued Asset
    UIA                 = 3, //User Issued Asset
    MPA                 = 4  //Market Pegged Asset
};

// perms for an asset group
enum AssetPermType : uint64_t {
    NULL_ASSET_PERM     = 0,         // no perm at all w/ the asset including coin transfer etc.
    PERM_TRANSFER       = (1 << 0 ), // allow transfer between accounts
    PERM_DEX_BASE       = (1 << 1 ), // as base symbol of dex trading pair(baseSymbol/quoteSymbol)
    PERM_DEX_QUOTE      = (1 << 2 ), // as quote symbol of dex trading pair(baseSymbol/quoteSymbol)
    PERM_CDP_BCOIN      = (1 << 3 ), // bcoins must have the perm while stable coins are only hard coded
    PERM_PRICE_FEED     = (1 << 4 ), // as base symbol of price feed coin pair(baseSymbol/quoteSymbol)

};

const uint64_t kAssetAllPerms = uint64_t(-1); // == 0xFFFFFFFFFFFFFFFF enable all perms
const uint64_t kAssetDefaultPerms = AssetPermType::PERM_TRANSFER | AssetPermType::PERM_DEX_BASE;
const uint64_t kWiccPerms = kAssetAllPerms;
const uint64_t kWusdPerms = PERM_TRANSFER + PERM_DEX_BASE + PERM_DEX_QUOTE;
const uint64_t kWgrtPerms = PERM_TRANSFER + PERM_PRICE_FEED + PERM_DEX_QUOTE + PERM_DEX_BASE;

static const unordered_map<uint64_t, string> kAssetPermTitleMap = {
    {   PERM_TRANSFER,      "PERM_TRANSFER"     },
    {   PERM_DEX_BASE,      "PERM_DEX_BASE"     },
    {   PERM_DEX_QUOTE,     "PERM_DEX_QUOTE"    },
    {   PERM_CDP_BCOIN,     "PERM_CDP_BCOIN"    },
    {   PERM_PRICE_FEED,    "PERM_PRICE_FEED"   },
};


inline bool AssetHasPerms(uint64_t assetPerms, uint64_t specificPerms) {
    return (assetPerms & specificPerms) == specificPerms;
}

enum class AssetPermStatus: uint8_t {
    NONE,
    ENABLED,
    DISABLED,
};

enum TotalSupplyOpType:uint8_t {
    ADD = 0,
    SUB = 1,
};
////////////////////////////////////////////////////////////////////
/// Common Asset Definition, used when persisted inside state DB
////////////////////////////////////////////////////////////////////

class CAsset {
public:
    TokenSymbol asset_symbol;                     // asset symbol, E.g WICC | WUSD
    TokenName asset_name;                         // asset long name, E.g WaykiChain coin
    AssetType asset_type = AssetType::NULL_ASSET; // asset type
    uint64_t perms_sum   = kAssetDefaultPerms;    // a sum of asset perms
    CRegID owner_regid;                            // creator or owner regid of the asset, null for NIA/DIA/MPA
    uint64_t total_supply = 0;                    // boosted by 10^8 for the decimal part, max is 90 billion.
    bool mintable         = false;                // whether this token can be minted in the future.

public:
    CAsset() {}

    CAsset(const TokenSymbol& assetSymbol, const TokenName& assetName, const AssetType AssetType, uint64_t assetPermsSum,
           const CRegID& ownerRegid, uint64_t totalSupply, bool mintableIn)
        : asset_symbol(assetSymbol), asset_name(assetName), asset_type(AssetType), perms_sum(assetPermsSum),
          owner_regid(ownerRegid), total_supply(totalSupply), mintable(mintableIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(asset_symbol);
        READWRITE(asset_name);
        READWRITE((uint8_t &) asset_type);
        READWRITE(VARINT(perms_sum));
        READWRITE(owner_regid);
        READWRITE(VARINT(total_supply));
        READWRITE(mintable);
    )

    bool OperateToTalSupply(uint64_t amount, TotalSupplyOpType op) {
        if (op == TotalSupplyOpType::ADD) {
            total_supply += amount;
        } else if (op == TotalSupplyOpType::SUB) {
            if(total_supply < amount)
                return false;
            total_supply -= amount;
        } else {
            return false;
        }

        return true;
    }
    bool HasPerms(uint64_t perms) const { return AssetHasPerms(perms_sum, perms); }

    bool IsEmpty() const { return asset_symbol.empty(); }

    void SetEmpty() {
        asset_symbol.clear();
        asset_name.clear();
        asset_type = AssetType::NULL_ASSET;
        perms_sum = 0;
        owner_regid.SetEmpty();
        total_supply = 0;
        mintable = false;
    }

    string ToString() const {
        return strprintf("asset_symbol=%s, asset_name=%s, asset_type=%d, perms_sum=%llu, owner_regid=%s, total_supply=%llu, mintable=%d",
                asset_symbol, asset_name, asset_type, perms_sum, owner_regid.ToString(), total_supply, mintable);
    }

    Object ToJsonObj() const {
        Object o;
        string permString;
        ConvertPermsToString(perms_sum, kAssetPermTitleMap.size(), permString);

        o.push_back(Pair("asset_symbol",  asset_symbol));
        o.push_back(Pair("asset_name",    asset_name));
        o.push_back(Pair("asset_type",    asset_type));
        o.push_back(Pair("perms_sum",     permString));
        o.push_back(Pair("owner_regid",   owner_regid.ToString()));
        o.push_back(Pair("total_supply",  total_supply));
        o.push_back(Pair("mintable",      mintable));
        return o;
    }
    // Check it when supplied from external like Tx or RPC calls
    static bool CheckSymbol(uint64_t height, const AssetType assetType, const TokenSymbol &assetSymbol, string &errMsg);
};

inline TokenSymbol GetQuoteSymbolByCdpScoin(const TokenSymbol &scoinSymbol) {
    return (scoinSymbol[0] == 'W') ? scoinSymbol.substr(1, scoinSymbol.size() - 1) : "";
}

inline TokenSymbol GetCdpScoinByQuoteSymbol(const TokenSymbol &quoteSymbol) {
    TokenSymbol scoinSymbol = "W" + quoteSymbol;
    return (kCdpScoinSymbolSet.count(scoinSymbol) > 0) ? scoinSymbol : "";
}

struct ComboMoney {
    TokenSymbol     symbol = SYMB::WICC;     //E.g. WICC
    uint64_t        amount = 0;
    CoinUnitName    unit = "";       //E.g. sawi

    ComboMoney() {};
    ComboMoney(const TokenSymbol &symbolIn, uint64_t amountIn, const CoinUnitName &unit)
        : symbol(symbolIn), amount(amountIn), unit(unit){};

    uint64_t GetAmountInSawi() const {
        auto it = CoinUnitTypeMap.find(unit);
        if (it != CoinUnitTypeMap.end())
            return amount * it->second;

        assert(false && "coin unit not found");
        return amount;
    }

    string ToString() {
        return strprintf("%s:%llu:%s", symbol, amount, unit);
    }
};

class CAssetTradingPair {
public:
    TokenSymbol base_asset_symbol;
    TokenSymbol quote_asset_symbol;

public:
    CAssetTradingPair() {}

    CAssetTradingPair(const TokenSymbol& baseSymbol, const TokenSymbol& quoteSymbol) :
        base_asset_symbol(baseSymbol), quote_asset_symbol(quoteSymbol) {}

     IMPLEMENT_SERIALIZE(
        READWRITE(base_asset_symbol);
        READWRITE(quote_asset_symbol);
    )

    friend bool operator<(const CAssetTradingPair& a, const CAssetTradingPair& b) {
        return a.base_asset_symbol < b.base_asset_symbol || a.quote_asset_symbol < b.quote_asset_symbol;
    }

    friend bool operator==(const CAssetTradingPair& a , const CAssetTradingPair& b) {
        return a.base_asset_symbol == b.base_asset_symbol && a.quote_asset_symbol == b.quote_asset_symbol;
    }

    string ToString() const {
        return strprintf("%s-%s", base_asset_symbol, quote_asset_symbol);
    }

    bool IsEmpty() const { return base_asset_symbol.empty() && quote_asset_symbol.empty(); }

    void SetEmpty() {
        base_asset_symbol.clear();
        quote_asset_symbol.clear();
    }
};

inline string CoinPairToString(const std::pair<TokenSymbol, TokenSymbol> coinPair) {
    return strprintf("%s:%s", coinPair.first, coinPair.second);
}

#endif //ENTITIES_ASSET_H
