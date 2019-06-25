// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <set>
#include <vector>

#include "persistence/leveldbwrapper.h"
#include "accounts/id.h"
#include "accounts/account.h"



enum OrderDirection {
    ORDER_BUY  = 0,
    ORDER_SELL = 1,
};

enum OrderType {
    ORDER_LIMIT_PRICE   = 0, //!< limit price order type
    ORDER_MARKET_PRICE  = 1  //!< market price order type
};

enum OrderGenerateType {
    USER_GEN_ORDER = 0,
    SYSTEM_GEN_ORDER,
};

// target amount type of order to buy/sell
// enum OrderTargetAmountType {
//     COIN_TARGET_AMOUNT,
//     ASSET_TARGET_AMOUNT
// };

class CDEXOrderData {
public:
    OrderType orderType;     //!< order type
    CoinType  coinType;      //!< coin type
    CoinType  assetType;     //!< asset type
    uint64_t  coinAmount;    //!< amount of coin to buy/sell asset
    uint64_t  assetAmount;   //!< amount of asset to buy/sell
    uint64_t  price;         //!< price in coinType want to buy/sell asset
};

/**
 * 
 */
class CDEXBuyOrder {
public:
    OrderType orderType;     //!< order type
    CoinType  coinType;      //!< coin type
    CoinType  assetType;     //!< asset type
    uint64_t  coinAmount;    //!< amount of coin to buy/sell asset
    uint64_t  assetAmount;   //!< amount of asset to buy/sell
    uint64_t  price;         //!< price in coinType want to buy/sell asset
};


class CDEXSellOrder {
public:
    OrderType orderType;     //!< order type
    CoinType  coinType;      //!< coin type
    CoinType  assetType;     //!< asset type
    uint64_t  coinAmount;    //!< amount of coin to buy/sell asset
    uint64_t  assetAmount;   //!< amount of asset to buy/sell
    uint64_t  price;         //!< price in coinType want to buy/sell asset
};


struct CDEXActiveBuyOrder {
    OrderGenerateType generateType;
    uint64_t residualAmount; //!< residual coin/asset amount for buying
    CTxCord  txCord;
};

struct CDEXActiveSellOrder {
    OrderGenerateType generateType;
    uint64_t residualAmount; //!< residual coin/asset amount for selling
    CTxCord  txCord;
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


    bool GetActiveBuyOrder(const uint256& orderId, CDEXActiveBuyOrder& buyOrder) { return false; }; // TODO: ...
    bool SetActiveBuyOrder(const uint256& orderId, const CDEXActiveBuyOrder& buyOrder, CDBOpLogMap &dbOpLogMap) {
        return false;  // TODO: ...
    };
    bool UndoActiveBuyOrder(CDBOpLogMap &dbOpLogMap) {
        return false;  // TODO: ...
    };
    bool GetActiveSellOrder(const uint256& orderId, CDEXActiveSellOrder& sellOrder) { return false; }; // TODO: ...
    bool SetActiveSellOrder(const uint256& orderId, const CDEXActiveSellOrder& sellOrder, CDBOpLogMap &dbOpLogMap) {
        return false;  // TODO: ...
    };
    bool UndoActiveSellOrder(CDBOpLogMap &dbOpLogMap) {
        return false;  // TODO: ...
    };

    bool CreateBuyOrder(uint64_t buyAmount, CoinType targetCoinType); //TODO: ... SystemBuyOrder
    bool CreateSellOrder(uint64_t sellAmount, CoinType targetCoinType); //TODO: ... SystemSellOrder

private:
    // CDBMultiValueCache<CDexFixedPriceOrder> bcoinBuyOrderCache;  // buy wicc with wusd (wusd_wicc)
    // CDBMultiValueCache<CDexFixedPriceOrder> fcoinBuyOrderCache;  // buy micc with wusd (wusd_micc)
    // CDBMultiValueCache<CDexFixedPriceOrder> bcoinSellOrderCache; // sell wicc for wusd (wicc_wusd)
    // CDBMultiValueCache<CDexFixedPriceOrder> fcoinSellOrderCache; // sell micc for wusd (micc_wusd)
};

#endif //PERSIST_DEX_H