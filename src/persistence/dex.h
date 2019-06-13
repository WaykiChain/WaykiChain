// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <map>
#include <string>

#include "accounts/id.h"

using namespace std;


enum OrderType {
    BUY = 1,
    SELL = 2,
    NULL_TYPE = 0
};

enum CoinType {
    WICC = 1,
    WUSD = 2,
    MICC = 3,
};

struct CDexManualOrder {
    CUserID orderUid;
    uint64_t orderAmount;
};

struct CDexForcedOrder {
    CUserID cdpOwnerUid;
    uint64_t bcoinsAmount;
    uint64_t scoinsAmount;
    double collateralRatioByAmount; //fixed: bcoinsAmount / scoinsAmount
    double collateralRatioByValue; //changed according to price

    uint64_t orderDiscount; // *1000 E.g. 97% * 1000 = 970

    bool operator()(const CDexForcedOrder &a, const CDexForcedOrder &b) {
        return a.collateralRatioByValue < b.collateralRatioByValue;
    }
};

class CDexCache {
public:
    CDexCache() {}

private:
    vector<CDexManualOrder> buyMiccManualOrderCache;  // buy micc with wusd, key: dex_bmicc{RegID}
    vector<CDexManualOrder> sellMiccManualOrderCache; // sell micc for wusd, key: dex_smicc{RegID}
    vector<CDexManualOrder> buyWiccManualOrderCache;  // buy wicc with wusd, key: dex_bwicc{RegID}
    vector<CDexManualOrder> sellWiccManualOrderCache; // sell wicc for wusd, key: dex_swicc{RegID}

    set<CDexForcedOrder> sellCdpWiccForcedOrderCache; //sell wicc for wusd with floating wicc price
};

#endif //PERSIST_DEX_H