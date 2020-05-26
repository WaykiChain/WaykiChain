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
typedef std::map<PriceCoinPair, uint64_t> PriceMap;

static const PriceMap EMPTY_PRICE_MAP = {};

static const uint32_t COIN_PRICE_PAIR_COUNT_MAX = 100;

class CPricePoint {
public:
    PriceCoinPair coin_price_pair;
    uint64_t price;  // boosted by 10^8

public:
    CPricePoint() {}

    CPricePoint(const PriceCoinPair &coinPricePair, const uint64_t priceIn)
        : coin_price_pair(coinPricePair), price(priceIn) {}

    CPricePoint(const CPricePoint& other) { *this = other; }

    ~CPricePoint() {}

public:
    uint64_t GetPrice() const { return price; }
    PriceCoinPair GetCoinPricePair() const { return coin_price_pair; }

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

class CMedianPriceDetail {
public:
    uint64_t price = 0;
    HeightType last_feed_height = 0;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(price));
        READWRITE(VARINT(last_feed_height));
    )

    bool IsEmpty() const {
        return price == 0 && last_feed_height == 0;
    }

    void SetEmpty() {
        price = 0;
        last_feed_height = 0;
    }

    string ToString() const {
        return strprintf("price=%llu, last_feed_height=%llu", price, last_feed_height);
    }

    bool IsActive(HeightType curHeight, HeightType priceTimeoutBlocks) const {
        if (GetFeatureForkVersion(curHeight) >= MAJOR_VER_R3)
            return curHeight <= last_feed_height + priceTimeoutBlocks;

        return true;
    }
};

typedef map<PriceCoinPair, CMedianPriceDetail> PriceDetailMap;


inline const string& GetPriceBaseSymbol(const PriceCoinPair &pricePair) {
    return std::get<0>(pricePair);
}

inline const string& GetPriceQuoteSymbol(const PriceCoinPair &pricePair) {
    return std::get<0>(pricePair);
}

#endif //ENTITIES_PRICE_H
