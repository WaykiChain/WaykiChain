// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_PRICE_H
#define ENTITIES_PRICE_H

#include <map>
#include "asset.h"

/**
 * coin price pair, pair<BaseSymbol, QuoteSymbol>
 * always written as BaseSymbol/QuoteSymbol
 * such as: WICC/USD
 */
typedef std::pair<TokenSymbol, TokenSymbol> CoinPricePair;
typedef std::map<CoinPricePair, uint64_t> PriceMap;

static const PriceMap EMPTY_PRICE_MAP = {};

static const CoinPricePair kDefaultFcoinPricePair = {SYMB::WGRT, SYMB::USD};

static const UnorderedPairSet<TokenSymbol, TokenSymbol> kCoinPricePairSet = {
    {SYMB::WICC, SYMB::USD},
    kDefaultFcoinPricePair,
};

static const uint32_t COIN_PRICE_PAIR_COUNT_MAX = 100;

class CPricePoint {
public:
    CoinPricePair coin_price_pair;
    uint64_t price;  // boosted by 10^8

public:
    CPricePoint() {}

    CPricePoint(const CoinPricePair &coinPricePair, const uint64_t priceIn)
        : coin_price_pair(coinPricePair), price(priceIn) {}

    CPricePoint(const CPricePoint& other) { *this = other; }

    ~CPricePoint() {}

public:
    uint64_t GetPrice() const { return price; }
    CoinPricePair GetCoinPricePair() const { return coin_price_pair; }

    string ToString() {
        return strprintf("coin_price_pair:%s:%s, price:%lld", coin_price_pair.first, coin_price_pair.second, price);
    }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(json_spirit::Pair("coin_symbol",      coin_price_pair.first));
        obj.push_back(json_spirit::Pair("price_symbol",     coin_price_pair.second));
        obj.push_back(json_spirit::Pair("price",            price));

        return obj;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coin_price_pair);
        READWRITE(VARINT(price));)

    CPricePoint& operator=(const CPricePoint& other) {
        if (this == &other)
            return *this;

        this->coin_price_pair   = other.coin_price_pair;
        this->price             = other.price;

        return *this;
    }
};

inline const string& GetPriceBaseSymbol(const CoinPricePair &pricePair) {
    return std::get<0>(pricePair);
}

inline const string& GetPriceQuoteSymbol(const CoinPricePair &pricePair) {
    return std::get<0>(pricePair);
}

/**
 * check price pair.
 * @return err_msg(string) return nullptr or error msg ptr
 *
 */
inline shared_ptr<string> CheckPricePair(const CoinPricePair &pricePair) {
    // TODO: support more price pair
    if (GetPriceBaseSymbol(pricePair) != SYMB::WICC && GetPriceBaseSymbol(pricePair) != SYMB::WGRT)
        return make_shared<string>("unsupport base symbol " + GetPriceBaseSymbol(pricePair) + " of price pair");

    if (GetPriceQuoteSymbol(pricePair) != SYMB::USD)
        return make_shared<string>("unsupport quote symbol " + GetPriceQuoteSymbol(pricePair) + " of price pair");
    return nullptr;
}

#endif //ENTITIES_PRICE_H
