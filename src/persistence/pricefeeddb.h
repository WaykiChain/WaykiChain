// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_PRICEFEED_H
#define PERSIST_PRICEFEED_H

#include "commons/uint256.h"
#include "tx/scointx.h"

#include <set>
#include <string>

using namespace std;

// typedef vector<unsigned char> unsigned_vector;
typedef string CCoinPriceType;

struct PriceFeed {
    uint64_t blockHeight;
    uint256 txid;
    unsigned char coinType;
    unsigned char priceType;
    uint64_t price;
};

class CPriceFeedCache {
private:
    set<uint64_t>> baseCoinPriceFeeds;      // memory only
    set<uint64_t>> stableCoinPriceFeeds;    // memory only
    set<uint64_t>> fundCoinPriceFeeds;      // memory only

public:
    uint64_6 ComputeMedianPrice(CoinType cointype, PriceType priceType);

    /**
     * Usage: Flush median coin price into DB: {coinType}_{priceType} -> {medianPrice}
     */
    bool Flush();

};

class CPriceFeedDB {

};

#endif  // PERSIST_PRICEFEED_H