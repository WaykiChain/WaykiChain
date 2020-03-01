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
    // {SYMB::WBTC, SYMB::WUSD},
    // {SYMB::WETH, SYMB::WUSD},
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

static const UnorderedPairSet<TokenSymbol, TokenSymbol> kTradingPairSet = {
    {SYMB::WICC, SYMB::WUSD},
    {SYMB::WGRT, SYMB::WUSD}
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

enum AssetType : uint8_t {
    NULL_ASSET      = 0,
    NIA             = 1, //natively issued asset
    DIA             = 2, //decentralized issued asset
    UIA             = 3, //user issued asset
    MPA             = 4  //market pegged asset
};

enum AssetPermType : uint16_t {
    NULL_ASSET_PERM = 0,
    DEX_BASE        = (1 << 0),
    DEX_QUOTE       = (1 << 1),
    CDP_IN          = (1 << 2),
    CDP_OUT         = (1 << 3),
    PRICE_FEED      = (1 << 4),
    XCHAIN_SWAP     = (1 << 5) 
};

struct CBaseAsset {
    TokenSymbol asset_symbol;       //asset symbol, E.g WICC | WUSD
    TokenName   asset_name;         //asset long name, E.g WaykiChain coin
    AssetType   asset_type;         //asset type
    uint16_t    asset_perms_sum;    //sum of asset perms
    CUserID     owner_uid;          //creator or owner user id of the asset
    uint64_t    total_supply;       //boosted by 10^8 for the decimal part, max is 90 billion.
    bool        mintable;           //whether this token can be minted in the future.

    CBaseAsset(): asset_perms_sum(0), total_supply(0), mintable(false) {}

    CBaseAsset(const TokenSymbol& assetSymbol, const TokenName& assetName, const AssetType assetType, 
            const uint16_t assetPermsSum, const CUserID& ownerUid, uint64_t totalSupply, bool mintableIn) :   
            asset_symbol(assetSymbol),
            asset_name(assetName),
            asset_type(assetType),
            asset_perms_sum(assetPermsSum),
            owner_uid(ownerUid),
            total_supply(totalSupply),
            mintable(mintableIn) {};

    CBaseAsset(const TokenSymbol& assetSymbol, const TokenName& assetName, const CUserID& ownerUid, 
            uint64_t totalSupply, bool mintableIn) : 
            asset_symbol(assetSymbol),
            asset_name(assetName),
            asset_type(AssetType::UIA),
            asset_perms_sum(AssetPermType::DEX_BASE),
            owner_uid(ownerUid),
            total_supply(totalSupply),
            mintable(mintableIn) {};


    IMPLEMENT_SERIALIZE(
        READWRITE(asset_symbol); 
        READWRITE(asset_name); 
        READWRITE((uint8_t &) asset_type);
        READWRITE(VARINT(asset_perms_sum));
        READWRITE(VARINT(owner_uid));
        READWRITE(VARINT(total_supply));
        READWRITE(mintable);
    )

    static bool CheckSymbolChar(const char ch) {
        return  ch >= 'A' && ch <= 'Z';
    }

    // @return nullptr if succeed, else err string
    static shared_ptr<string> CheckSymbol(const TokenSymbol &symbol) {
        size_t symbolSize = symbol.size();
        if (symbolSize < MIN_ASSET_SYMBOL_LEN || symbolSize > MAX_TOKEN_SYMBOL_LEN)
            return make_shared<string>(strprintf("length=%d must be in range[%d, %d]",
                symbolSize, MIN_ASSET_SYMBOL_LEN, MAX_TOKEN_SYMBOL_LEN));

        for (auto ch : symbol) {
            if (!CheckSymbolChar(ch))
                return make_shared<string>("there is invalid char in symbol");
        }
        return nullptr;
    }

    string ToString() const {
        return strprintf("asset_symbol=%s, asset_name=%s, asset_type=%s, asset_perms_sum=%d,"
                      "owner_uid=%s, total_supply=%llu, mintable=%d",
                        asset_symbol, asset_name, asset_type, asset_perms_sum, 
                        owner_uid.ToString(), total_supply, mintable);
    }
};

class CAsset: public CBaseAsset {
public:
    uint64_t min_order_amount;  // min amount for submit order tx, 0 is unlimited
    uint64_t max_order_amount;  // max amount for submit order tx, 0 is unlimited

public:
    CAsset(): CBaseAsset(), min_order_amount(0), max_order_amount(0) {}

    CAsset(CBaseAsset *pBaseAsset): CBaseAsset(*pBaseAsset), min_order_amount(0), max_order_amount(0) {}

    CAsset(const TokenSymbol& symbolIn, const CUserID& ownerUseridIn, const TokenName& nameIn,
           uint64_t totalSupplyIn, bool mintableIn, uint64_t minOrderAmountIn, uint64_t maxOrderAmountIn)
        : CBaseAsset(symbolIn, ownerUseridIn, nameIn, totalSupplyIn, mintableIn),
          min_order_amount(minOrderAmountIn), max_order_amount(maxOrderAmountIn){};

    IMPLEMENT_SERIALIZE(
        READWRITE(asset_symbol); 
        READWRITE(asset_name); 
        READWRITE((uint8_t &) asset_type);
        READWRITE(VARINT(asset_perms_sum));
        READWRITE(VARINT(owner_uid));
        READWRITE(VARINT(total_supply));
        READWRITE(mintable);
        
        READWRITE(VARINT(max_order_amount));
        READWRITE(VARINT(min_order_amount));
    )

    bool IsEmpty() const { return owner_uid.IsEmpty(); }

    void SetEmpty() {
        owner_uid.SetEmpty();
        asset_symbol.clear();
        asset_name.clear();
        mintable = false;
        total_supply = 0;
        max_order_amount = 0;
        min_order_amount = 0;
    }

    string ToString() const {
        return CBaseAsset::ToString()+ ", " +
                strprintf("min_order_amount=%llu", min_order_amount) + ", " +
                strprintf("min_order_amount=%llu", min_order_amount);
    }
};

bool CheckCoinRange(const TokenSymbol &symbol, const int64_t amount);

#endif //ENTITIES_ASSET_H
