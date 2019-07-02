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

struct CDEXOrderDetail {
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

// order txid -> sys order data
// order txid:
//   (1) CCDPStakeTx, create sys buy order for WGRT by WUSD when alter CDP and the interest is WUSD
//   (2) CCDPRedeemTx, create sys buy order for WGRT by WUSD when the interest is WUSD
//   (3) CCDPLiquidateTx, create sys buy order for WGRT by WUSD when the penalty is WUSD
class CDEXSysBuyOrder {
public:
    CoinType        coinType;      //!< coin type
    CoinType        assetType;     //!< asset type
    uint64_t        coinAmount;    //!< amount of coin to buy asset
public:
    CDEXSysBuyOrder() {};
    CDEXSysBuyOrder(CoinType coinTypeIn, CoinType assetTypeIn, uint64_t coinAmountIn):
                coinType(coinTypeIn), assetType(assetTypeIn), coinAmount(coinAmountIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);
        READWRITE(VARINT(coinAmount));
    )
    bool IsEmpty() const;
    void SetEmpty();
    void GetOrderDetail(CDEXOrderDetail &orderDetail);
};

// txid -> sys order data
class CDEXSysSellOrder {
public:
    CoinType        coinType;      //!< coin type
    CoinType        assetType;     //!< asset type
    uint64_t        assetAmount;    //!< amount of coin to buy asset

public:
    CDEXSysSellOrder() {};
    CDEXSysSellOrder(CoinType coinTypeIn, CoinType assetTypeIn, uint64_t assetAmountIn):
                coinType(coinTypeIn), assetType(assetTypeIn), assetAmount(assetAmountIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);
        READWRITE(VARINT(assetAmount));
    )

    bool IsEmpty() const;
    void SetEmpty();
    void GetOrderDetail(CDEXOrderDetail &orderDetail) const;
};

// System-generated Market Order
// wicc -> wusd (cdp forced liquidation)
// wgrt -> wusd (inflate wgrt to get wusd)
// wusd -> wgrt (pay interest to get wgrt to burn)
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
    bool GetActiveOrder(const uint256 &orderTxId, CDEXActiveOrder& activeOrder);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder& activeOrder, CDBOpLogMap &dbOpLogMap);
    bool ModifyActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder& activeOrder, CDBOpLogMap &dbOpLogMap);
    bool EraseActiveOrder(const uint256 &orderTxId, CDBOpLogMap &dbOpLogMap);
    bool UndoActiveOrder(CDBOpLogMap &dbOpLogMap);

    bool GetSysBuyOrder(const uint256 &orderTxId, CDEXSysBuyOrder &buyOrder);
    bool CreateSysBuyOrder(const uint256 &orderTxId, const CDEXSysBuyOrder &buyOrder, CDBOpLogMap &dbOpLogMap);
    bool UndoSysBuyOrder(CDBOpLogMap &dbOpLogMap);

    bool GetSysSellOrder(const uint256 &orderTxId, CDEXSysSellOrder &sellOrder);
    bool CreateSysSellOrder(const uint256 &orderTxId, const CDEXSysSellOrder &sellOrder, CDBOpLogMap &dbOpLogMap);
    bool UndoSysSellOrder(CDBOpLogMap &dbOpLogMap);


    bool CreateBuyOrder(uint64_t buyAmount, CoinType targetCoinType); //TODO: ... SystemBuyOrder
    bool CreateSellOrder(uint64_t sellAmount, CoinType targetCoinType); //TODO: ... SystemSellOrder

private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CDBMultiValueCache< dbk::DEX_ACTIVE_ORDER,         uint256,                   CDEXActiveOrder >     activeOrderCache;
    CDBMultiValueCache< dbk::DEX_SYS_BUY_ORDER,        uint256,                   CDEXSysBuyOrder >     sysBuyOrderCache;
    CDBMultiValueCache< dbk::DEX_SYS_SELL_ORDER,       uint256,                   CDEXSysSellOrder >     sysSellOrderCache;
};

#endif //PERSIST_DEX_H
