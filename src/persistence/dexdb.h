// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <set>
#include <vector>

#include "accounts/id.h"

// using namespace std;

// enum OrderType {
//     BUY = 1,
//     SELL = 2,
//     NULL_TYPE = 0
// };

// enum CoinType {
//     WICC = 1,
//     WUSD = 2,
//     MICC = 3,
// };

struct CDEXLimitPriceOrder {
    CUserID orderUid;
    uint64_t orderAmount;
    uint64_t orderPrice;

     bool operator()(const CDEXLimitPriceOrder &a, const CDEXLimitPriceOrder &b) {
        return a.orderPrice < b.orderPrice;
    }
};

struct CDEXMarketPriceOrder {
    CUserID orderUid;
    uint64_t orderAmount;

};

// System-generated Market Order
// wicc -> wusd (cdp forced liquidation)
// micc -> wusd (inflate micc to get wusd)
// wusd -> micc (pay interest to get micc to burn)
struct CDEXSysForceSellBcoinsOrder {
    CUserID cdpOwnerUid;
    uint64_t bcoinsAmount;
    uint64_t scoinsAmount;
    double collateralRatioByAmount; // fixed: 100*  bcoinsAmount / scoinsAmount
    double collateralRatioByValue;  // collateralRatioAmount * wiccMedianPrice

    uint64_t orderDiscount; // *1000 E.g. 97% * 1000 = 970

};

class CDexCache {
public:
    CDexCache() {}

public:


private:
    CDBMultiValueCache<CDexFixedPriceOrder> bcoinBuyOrderCache;  // buy wicc with wusd (wusd_wicc)
    CDBMultiValueCache<CDexFixedPriceOrder> fcoinBuyOrderCache;  // buy micc with wusd (wusd_micc)
    CDBMultiValueCache<CDexFixedPriceOrder> bcoinSellOrderCache; // sell wicc for wusd (wicc_wusd)
    CDBMultiValueCache<CDexFixedPriceOrder> fcoinSellOrderCache; // sell micc for wusd (micc_wusd)
};

#endif //PERSIST_DEX_H