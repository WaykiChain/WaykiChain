// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <set>
#include <vector>

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
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
    EMPTY_ORDER         = 0,
    USER_GEN_ORDER      = 1,
    SYSTEM_GEN_ORDER    = 2
};

class CDEXOrderData {
public:
    CRegID          userRegId;
    OrderType       orderType;     //!< order type
    OrderDirection  direction;
    CoinType        coinType;      //!< coin type
    CoinType        assetType;     //!< asset type
    uint64_t        coinAmount;    //!< amount of coin to buy/sell asset
    uint64_t        assetAmount;   //!< amount of asset to buy/sell
    uint64_t        price;         //!< price in coinType want to buy/sell asset
};


// for all active order db: orderId -> CDEXActiveOrder
struct CDEXActiveOrder {
    OrderGenerateType generateType  = EMPTY_ORDER;  //!< generate type
    uint64_t totalDealCoinAmount    = 0;            //!< total deal coin amount
    uint64_t totalDealAssetAmount   = 0;            //!< total deal asset amount
    CTxCord  txCord                 = CTxCord();    //!< related tx cord

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)generateType);
        READWRITE(VARINT(totalDealCoinAmount));
        READWRITE(VARINT(totalDealAssetAmount));
        READWRITE(txCord);
    )

    bool IsEmpty() const {
        return generateType == EMPTY_ORDER;
    }
    void SetEmpty() {
        generateType  = EMPTY_ORDER;
        totalDealCoinAmount    = 0;
        totalDealAssetAmount   = 0;
        txCord.SetEmpty();
    }
};

// for SYSTEM_GEN_ORDER db: txid -> sys order data

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

class CDexDBCache {
public:
    CDexDBCache() {}

public:
    bool GetActiveOrder(const uint256& orderTxId, CDEXActiveOrder& activeOrder) {
        return activeOrderCache.GetData(orderTxId, activeOrder);
    };
    bool SetActiveOrder(const uint256& orderTxId, const CDEXActiveOrder& activeOrder, CDBOpLogMap &dbOpLogMap) {
        return activeOrderCache.SetData(orderTxId, activeOrder, dbOpLogMap);
    };
    bool EraseActiveOrder(const uint256& orderTxId, CDBOpLogMap &dbOpLogMap) {
        return activeOrderCache.SetData(orderTxId, CDEXActiveOrder(), dbOpLogMap);
    };
    bool UndoActiveOrder(CDBOpLogMap &dbOpLogMap) {
        return activeOrderCache.UndoData(dbOpLogMap);
    };

    bool CreateBuyOrder(uint64_t buyAmount, CoinType targetCoinType); //TODO: ... SystemBuyOrder
    bool CreateSellOrder(uint64_t sellAmount, CoinType targetCoinType); //TODO: ... SystemSellOrder

private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CDBMultiValueCache< dbk::DEX_ACTIVE_ORDER,         uint256,                   CDEXActiveOrder >     activeOrderCache;
};

#endif //PERSIST_DEX_H