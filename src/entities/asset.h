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

struct ComboMoney {
    TokenSymbol     symbol;     //E.g. WICC
    uint64_t        amount;
    CoinUnitName    unit;       //E.g. sawi

    ComboMoney() : symbol(SYMB::WICC), amount(0), unit(COIN_UNIT::SAWI){};

    uint64_t GetSawiAmount() const {
        auto it = CoinUnitTypeTable.find(unit);
        if (it != CoinUnitTypeTable.end()) {
            return amount * it->second;
        } else {
            assert(false && "coin unit not found");
            return amount;
        }
    }
};

static const unordered_set<string> kCoinTypeSet = {
    SYMB::WICC, SYMB::WGRT, SYMB::WUSD
};

static const unordered_set<string> kCurrencyTypeSet = {
    SYMB::USD, SYMB::CNY, SYMB::EUR, SYMB::BTC, SYMB::BTC_USDT, SYMB::ETH_USDT, SYMB::GOLD, SYMB::KWH
};

static const unordered_set<string> kScoinSymbolSet = {
    SYMB::WUSD, SYMB::WCNY
};

static const UnorderedPairSet<TokenSymbol, TokenSymbol> kCDPCoinPairSet = {
    {SYMB::WICC, SYMB::WUSD},
    {SYMB::WBTC, SYMB::WUSD},
    {SYMB::WETH, SYMB::WUSD},
    // {SYMB::WEOS, SYMB::WUSD},

    // {SYMB::WICC, SYMB::WCNY},
    // {SYMB::WBTC, SYMB::WCNY},
    // {SYMB::WETH, SYMB::WCNY},
    // {SYMB::WEOS, SYMB::WCNY},
};

// cdp scoin symbol -> price quote symbol
static const unordered_map<TokenSymbol, TokenSymbol> kCdpScoinToPriceQuoteMap = {
    {SYMB::WUSD, SYMB::USD},
};

inline const TokenSymbol& GetPriceQuoteByCdpScoin(const TokenSymbol &scoinSymbol) {
    auto it = kCdpScoinToPriceQuoteMap.find(scoinSymbol);
    if (it != kCdpScoinToPriceQuoteMap.end())
        return it->second;
    return EMPTY_STRING;
}
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

enum AssetType : uint8_t {
    NULL_ASSET      = 0,
    NIA             = 1, //natively issued asset
    DIA             = 2, //decentralized issued asset
    UIA             = 3, //user issued asset
    MPA             = 4  //market pegged asset
};

enum AssetPermType : uint64_t {
    NULL_ASSET_PERM = 0,
    DEX_BASE        = (1 << 0),
    DEX_QUOTE       = (1 << 1),
    CDP_BCOIN       = (1 << 2),
    CDP_SCOIN       = (1 << 3),
    PRICE_FEED      = (1 << 4),
    XCHAIN_SWAP     = (1 << 5)
};

////////////////////////////////////////////////////////////////////
/// Common Asset Definition, used when persisted inside state DB
////////////////////////////////////////////////////////////////////
class CAsset {
public:
    TokenSymbol asset_symbol;       //asset symbol, E.g WICC | WUSD
    TokenName   asset_name;         //asset long name, E.g WaykiChain coin
    AssetType   asset_type;         //asset type
    uint64_t    asset_perms_sum;    //a sum of asset perms
    CUserID     owner_uid;          //creator or owner user id of the asset
    uint64_t    total_supply;       //boosted by 10^8 for the decimal part, max is 90 billion.
    bool        mintable;           //whether this token can be minted in the future.

public:
    CAsset(): asset_type(AssetType::NULL_ASSET) {}

    CAsset(CAsset *pBaseAsset): asset_type(AssetType::NULL_ASSET) {}

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
        owner_uid.SetEmpty();
        asset_symbol.clear();
        asset_name.clear();
        mintable = false;
        total_supply = 0;
    }

    string ToString() const {
        return strprintf("asset_symbol=%s, asset_name=%s, asset_type=%d, asset_perms_sum=%llu, owner_uid=%s, total_supply=%llu, mintable=%d",
                asset_symbol, asset_name, asset_type, asset_perms_sum, owner_uid.ToString(), total_supply, mintable);
    }
};

bool CheckCoinRange(const TokenSymbol &symbol, const int64_t amount) {
    if (symbol == SYMB::WICC) {
        return CheckBaseCoinRange(amount);
    } else if (symbol == SYMB::WGRT) {
        return CheckFundCoinRange(amount);
    } else if (symbol == SYMB::WUSD) {
        return CheckStableCoinRange(amount);
    } else {
        // TODO: need to check other token range
        return amount >= 0;
    }
}


#endif //ENTITIES_ASSET_H
