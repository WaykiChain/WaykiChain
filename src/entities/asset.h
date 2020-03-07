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
    NULL_ASSET      = 0,
    NIA             = 1, //Natively Issued Asset
    DIA             = 2, //DeGov Issued Asset
    UIA             = 3, //User Issued Asset
    MPA             = 4  //Market Pegged Asset
};

// perms for an asset group
enum AssetPermType : uint64_t {
    NULL_ASSET_PERM     = 0,        // no perm at all w/ the asset including coin transfer etc.
    PERM_DEX_BASE       = (1 << 1 ),
    PERM_DEX_QUOTE      = (1 << 2 ),
    PERM_CDP_BCOIN      = (1 << 3 ),
    PERM_CDP_SCOIN      = (1 << 4 ),
    PERM_PRICE_FEED     = (1 << 5 ),
    PERM_XCHAIN_SWAP    = (1 << 6 ),

};

////////////////////////////////////////////////////////////////////
/// Common Asset Definition, used when persisted inside state DB
////////////////////////////////////////////////////////////////////
class CAsset {
public:
    TokenSymbol asset_symbol;       //asset symbol, E.g WICC | WUSD
    TokenName   asset_name;         //asset long name, E.g WaykiChain coin
    AssetType   asset_type;         //asset type
    uint64_t    asset_perms_sum = 0;//a sum of asset perms
    CUserID     owner_uid;          //creator or owner user id of the asset, null for NIA/DIA/MPA
    uint64_t    total_supply;       //boosted by 10^8 for the decimal part, max is 90 billion.
    bool        mintable;           //whether this token can be minted in the future.

public:
    CAsset(): asset_type(AssetType::NULL_ASSET), asset_perms_sum(AssetPermType::PERM_DEX_BASE) {}

    CAsset(const TokenSymbol& assetSymbol, const TokenName& assetName, const AssetType assetType, uint64_t assetPermsSum,
            const CUserID& ownerUid, uint64_t totalSupply, bool mintableIn)
        : asset_symbol(assetSymbol), asset_name(assetName), asset_type(assetType), asset_perms_sum(assetPermsSum),
        owner_uid(ownerUid), total_supply(totalSupply), mintable(mintableIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(asset_symbol);
        READWRITE(asset_name);
        READWRITE((uint8_t &) asset_type);
        READWRITE(VARINT(asset_perms_sum));
        READWRITE(owner_uid);
        READWRITE(VARINT(total_supply));
        READWRITE(mintable);
    )

    bool IsEmpty() const { return owner_uid.IsEmpty(); }

    void SetEmpty() {
        asset_symbol.clear();
        asset_name.clear();
        asset_type = AssetType::NULL_ASSET;
        asset_perms_sum = 0;
        owner_uid.SetEmpty();
        total_supply = 0;
        mintable = false;
    }

    string ToString() const {
        return strprintf("asset_symbol=%s, asset_name=%s, asset_type=%d, asset_perms_sum=%llu, owner_uid=%s, total_supply=%llu, mintable=%d",
                asset_symbol, asset_name, asset_type, asset_perms_sum, owner_uid.ToString(), total_supply, mintable);
    }
};


inline const TokenSymbol& GetPriceQuoteByCdpScoin(const TokenSymbol &scoinSymbol) {
    auto it = kCdpScoinToPriceQuoteMap.find(scoinSymbol);
    if (it != kCdpScoinToPriceQuoteMap.end())
        return it->second;

    return EMPTY_STRING;
}

struct ComboMoney {
    TokenSymbol     symbol;     //E.g. WICC
    uint64_t        amount;
    CoinUnitName    unit;       //E.g. sawi

    ComboMoney() : symbol(SYMB::WICC), amount(0), unit(COIN_UNIT::SAWI){};

    uint64_t GetAmountInSawi() const {
        auto it = CoinUnitTypeMap.find(unit);
        if (it != CoinUnitTypeMap.end())
            return amount * it->second;
        
        assert(false && "coin unit not found");
        return amount;
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

#endif //ENTITIES_ASSET_H
