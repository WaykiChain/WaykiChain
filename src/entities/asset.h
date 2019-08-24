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
#include "json/json_spirit_utils.h"

using namespace json_spirit;
using namespace std;

typedef string TokenSymbol;     //8 chars max, E.g. WICC, WCNY, WICC-01D
typedef string TokenName;       //32 chars max, E.g. WaykiChain Coins
typedef string CoinUnitName;    //defined in coin unit type table

typedef std::pair<TokenSymbol, TokenSymbol> CoinPricePair;
typedef std::pair<TokenSymbol, TokenSymbol> TradingPair;
typedef string AssetSymbol;     //8 chars max, E.g. WICC
typedef string PriceSymbol;     //8 chars max, E.g. USD, CNY, EUR, BTC

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
    SYMB::USD, SYMB::CNY, SYMB::EUR, SYMB::BTC, SYMB::USDT, SYMB::GOLD, SYMB::KWH
};

static const UnorderedPairSet<TokenSymbol, TokenSymbol> kCDPCoinPairSet = {
    {SYMB::WICC, SYMB::WUSD},
    {SYMB::WGRT, SYMB::WUSD},
    {SYMB::WBTC, SYMB::WUSD},
    {SYMB::WETH, SYMB::WUSD},
    {SYMB::WEOS, SYMB::WUSD},

    {SYMB::WICC, SYMB::WCNY},
    {SYMB::WGRT, SYMB::WCNY},
    {SYMB::WBTC, SYMB::WCNY},
    {SYMB::WETH, SYMB::WCNY},
    {SYMB::WEOS, SYMB::WCNY},
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
    CAssetTradingPair() {}

    CAssetTradingPair(const TokenSymbol& baseSymbol, const TokenSymbol& quoteSymbol) :
        base_asset_symbol(baseSymbol), quote_asset_symbol(quoteSymbol) {}

     IMPLEMENT_SERIALIZE(
        READWRITE(base_asset_symbol);
        READWRITE(quote_asset_symbol);
    )

    friend bool operator<(const CAssetTradingPair& a, const CAssetTradingPair& b) {
        return a.base_asset_symbol < a.base_asset_symbol || b.quote_asset_symbol < b.quote_asset_symbol;
    }

    string ToString() {
        return strprintf("%s-%s", base_asset_symbol, quote_asset_symbol);
    }

    bool IsEmpty() const { return base_asset_symbol.empty() && quote_asset_symbol.empty(); }

    void SetEmpty() {
        base_asset_symbol.clear();
        quote_asset_symbol.clear();
    }
};

class CAsset {
public:
    TokenSymbol symbol;     // asset symbol, E.g WICC | WUSD
    CUserID owner_uid;      // creator or owner user id of the asset
    TokenName name;         // asset long name, E.g WaykiChain coin
    uint64_t total_supply;  // boosted by 10^8 for the decimal part, max is 90 billion.
    bool mintable;          // whether this token can be minted in the future.
public:
    CAsset(): total_supply(0), mintable(false) {}

    CAsset(const TokenSymbol& symbolIn, const CUserID& ownerUseridIn, const TokenName& nameIn,
           uint64_t totalSupplyIn, bool mintableIn)
        : symbol(symbolIn),
          owner_uid(ownerUseridIn),
          name(nameIn),
          total_supply(totalSupplyIn),
          mintable(mintableIn){};

    IMPLEMENT_SERIALIZE(READWRITE(symbol); READWRITE(owner_uid); READWRITE(name);
                        READWRITE(mintable); READWRITE(VARINT(total_supply));)

    bool IsEmpty() const { return owner_uid.IsEmpty(); }

    void SetEmpty() {
        owner_uid.SetEmpty();
        symbol.clear();
        name.clear();
        mintable = false;
        total_supply = 0;
    }

public:
    static bool CheckSymbolChar(const char ch) {
        return  (ch >= '0' && ch <= '9') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= 'a' && ch <= 'z') ||
                ch == '@' ||
                ch == '#' ||
                ch == '.' ||
                ch == '_' ||
                ch == '-' ||
                ch == '/';
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

    Object ToJson() const {
        Object result;
        result.push_back(Pair("asset_symbol",   symbol));
        result.push_back(Pair("owner_uid",      owner_uid.ToString()));
        result.push_back(Pair("asset_name",     name));
        result.push_back(Pair("total_supply",   total_supply));
        result.push_back(Pair("mintable",       mintable));
        return result;
    }
};

#endif //ENTITIES_ASSET_H
