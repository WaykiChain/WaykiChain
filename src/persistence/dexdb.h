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

struct CDexFixedPriceOrder {
    CUserID orderUid;
    uint64_t orderAmount;
    uint64_t orderPrice;

     bool operator()(const CDexFixedPriceOrder &a, const CDexFixedPriceOrder &b) {
        return a.orderPrice < b.orderPrice;
    }
};

struct CDexForcedCdpOrder {
    CUserID cdpOwnerUid;
    uint64_t bcoinsAmount;
    uint64_t scoinsAmount;
    double collateralRatioByAmount; //fixed: 100*  bcoinsAmount / scoinsAmount
    double collateralRatioByValue; // collateralRatioAmount * wiccMedianPrice

    uint64_t orderDiscount; // *1000 E.g. 97% * 1000 = 970

    bool operator()(const CDexForcedCdpOrder &a, const CDexForcedCdpOrder &b) {
        return a.collateralRatioByAmount < b.collateralRatioByAmount;
    }
};

class CDexCache {
public:
    CDexCache() {}

public:
    bool MatchFcoinManualSellOrder(uint64_t scoins);

private:
    CDBMultiValueCache<CDexFixedPriceOrder> bcoinBuyOrderCache;  // buy wicc with wusd (wusd_wicc)
    CDBMultiValueCache<CDexFixedPriceOrder> fcoinBuyOrderCache;  // buy micc with wusd (wusd_micc)
    CDBMultiValueCache<CDexFixedPriceOrder> bcoinSellOrderCache; // sell wicc for wusd (wicc_wusd)
    CDBMultiValueCache<CDexFixedPriceOrder> fcoinSellOrderCache; // sell micc for wusd (micc_wusd)

    //floating price
    CDBMultiValueCache<CDexForcedCdpOrder> bcoinCdpSellOrderCache; //sell wicc for wusd (wicc_wusd)
    CDBMultiValueCache<CDexForcedCdpOrder> fcoinCdpSellOrderCache; //sell micc for wusd (micc_wusd)
    CDBMultiValueCache<CDexForcedCdpOrder> fcoinCdpSellOrderCache; //sell micc for wusd (wusd_micc)
};

#endif //PERSIST_DEX_H