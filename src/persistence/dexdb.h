// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <set>
#include <vector>

#include "accounts/id.h"
#include "accounts/account.h"

enum OrderDirection {
    ORDER_BUY  = 0,
    ORDER_SELL = 1,
};

class CDEXSellOrderInfo {
public:
    uint64_t sellRemains; //!< 剩余可卖的资产数量
};

class CDEXBuyOrderInfo {
public:
    uint64_t buyRemains; //!< 剩余可买的资产数量
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


    bool GetBuyOrder(const CTxCord& txCord, CDEXBuyOrderInfo& buyOrderInfo) { return false; }; // TODO: ...

    bool HaveBuyOrder(const CTxCord& txCord) { return false; }; // TODO: ...

    bool GetSellOrder(const CTxCord& txCord, CDEXSellOrderInfo& sellOrderInfo) { return false; }; // TODO: ...
    bool HaveSellOrder(const CTxCord& txCord) { return false; }; // TODO: ...

    bool CreateBuyOrder(uint64_t buyAmount, CoinType targetCoinType); //TODO: ... SystemBuyOrder
    bool CreateSellOrder(uint64_t sellAmount, CoinType targetCoinType); //TODO: ... SystemSellOrder

private:
    // CDBMultiValueCache<CDexFixedPriceOrder> bcoinBuyOrderCache;  // buy wicc with wusd (wusd_wicc)
    // CDBMultiValueCache<CDexFixedPriceOrder> fcoinBuyOrderCache;  // buy micc with wusd (wusd_micc)
    // CDBMultiValueCache<CDexFixedPriceOrder> bcoinSellOrderCache; // sell wicc for wusd (wicc_wusd)
    // CDBMultiValueCache<CDexFixedPriceOrder> fcoinSellOrderCache; // sell micc for wusd (micc_wusd)
};

#endif //PERSIST_DEX_H