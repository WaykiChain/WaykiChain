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

enum OrderExchangeType {
    // ORDER_LIMIT_PRICE   = 0, //!< limit price order type
    // ORDER_MARKET_PRICE  = 1  //!< market price order type
};

enum OrderGenerateType {
    USER_GEN_ORDER = 0,
    SYSTEM_GEN_ORDER,
};

/**
 * 
 */
class CDEXBuyOrder {
public:
    OrderExchangeType exchangeType;    //!< order exchange type
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t buyAmount;     //!< amount of target asset to buy
    uint64_t bidPrice;      //!< bidding price in coinType willing to buy
};

class CDEXSellOrder {
public:
    OrderExchangeType exchangeType;    //!< order exchange type
    CoinType coinType;      //!< coin type (wusd) to sell asset
    CoinType assetType;     //!< holing asset type (wicc or micc) to sell in coinType
    uint64_t sellAmount;    //!< amount of holding asset to sell
    uint64_t askPrice;      //!< asking price in coinType willing to sell     
};


struct CDEXActiveBuyOrderInfo {
    OrderGenerateType generateType;
    uint64_t residualAmount; //!< residual coin/asset amount for buying
};

struct CDEXActiveSellOrderInfo {
    OrderGenerateType generateType;
    uint64_t residualAmount; //!< residual coin/asset amount for selling
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


    bool GetActiveBuyOrder(const CTxCord& txCord, CDEXActiveBuyOrderInfo& buyOrderInfo) { return false; }; // TODO: ...

    bool HaveBuyOrder(const CTxCord& txCord) { return false; }; // TODO: ...

    bool GetActiveSellOrder(const CTxCord& txCord, CDEXActiveSellOrderInfo& sellOrderInfo) { return false; }; // TODO: ...
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