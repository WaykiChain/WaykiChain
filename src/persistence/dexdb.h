// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <string>
#include <set>
#include <vector>

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
#include "entities/id.h"
#include "entities/account.h"

using namespace std;

enum OrderDirection {
    ORDER_BUY  = 0,
    ORDER_SELL = 1,
};

const static std::string OrderDirectionTitles[] = {"Buy", "Sell"};

enum OrderType {
    ORDER_LIMIT_PRICE   = 0, //!< limit price order type
    ORDER_MARKET_PRICE  = 1  //!< market price order type
};

const static std::string OrderTypeTitles[] = {"LimitPrice", "MarketPrice"};

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
    AssetType       assetType;     //!< asset type
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
//   (1) CCDPStakeTx, create sys buy market order for WGRT by WUSD when alter CDP and the interest is WUSD
//   (2) CCDPRedeemTx, create sys buy market order for WGRT by WUSD when the interest is WUSD
//   (3) CCDPLiquidateTx, create sys buy market order for WGRT by WUSD when the penalty is WUSD

// txid -> sys order data
class CDEXSysOrder {
private:
    OrderDirection  direction;     //!< order direction
    OrderType       orderType;     //!< order type
    CoinType        coinType;      //!< coin type
    AssetType       assetType;     //!< asset type

    uint64_t        coinAmount;    //!< amount of coin to buy/sell asset
    uint64_t        assetAmount;   //!< amount of coin to buy asset
    uint64_t        price;         //!< price in coinType want to buy/sell asset
public:// create functions
    static shared_ptr<CDEXSysOrder> CreateBuyLimitOrder(CoinType coinTypeIn, CoinType assetTypeIn,
                                                        uint64_t assetAmountIn, uint64_t priceIn);

    static shared_ptr<CDEXSysOrder> CreateSellLimitOrder(CoinType coinTypeIn, CoinType assetTypeIn,
                                                         uint64_t assetAmountIn, uint64_t priceIn);

    static shared_ptr<CDEXSysOrder> CreateBuyMarketOrder(CoinType coinTypeIn, CoinType assetTypeIn,
                                                         uint64_t coinAmountIn);

    static shared_ptr<CDEXSysOrder> CreateSellMarketOrder(CoinType coinTypeIn, CoinType assetTypeIn,
                                                          uint64_t assetAmountIn);

public:
    // default constructor
    CDEXSysOrder():
        direction(ORDER_BUY),
        orderType(ORDER_LIMIT_PRICE),
        coinType(WUSD),
        assetType(WICC),
        coinAmount(0),
        assetAmount(0),
        price(0)
        {}

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)direction);
        READWRITE((uint8_t&)orderType);
        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);

        READWRITE(VARINT(coinAmount));
        READWRITE(VARINT(assetAmount));
        READWRITE(VARINT(price));
    )

    string ToString() {
        return strprintf(
                "OrderDir=%s, OrderType=%s, CoinType=%d, AssetType=%s, coinAmount=%lu, assetAmount=%lu, price=%lu",
                OrderDirectionTitles[direction], OrderTypeTitles[orderType],
                kCoinTypeMapName.at(coinType), kCoinTypeMapName.at(assetType),
                coinAmount, assetAmount, price);
    }

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
    CDexDBCache(CDBAccess *pDbAccess) : activeOrderCache(pDbAccess), sysOrderCache(pDbAccess) {};

public:
    bool GetActiveOrder(const uint256 &orderTxId, CDEXActiveOrder& activeOrder);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder& activeOrder, CDBOpLogMap &dbOpLogMap);
    bool ModifyActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder& activeOrder, CDBOpLogMap &dbOpLogMap);
    bool EraseActiveOrder(const uint256 &orderTxId, CDBOpLogMap &dbOpLogMap);
    bool UndoActiveOrder(CDBOpLogMap &dbOpLogMap);

    bool GetSysOrder(const uint256 &orderTxId, CDEXSysOrder &buyOrder);
    bool CreateSysOrder(const uint256 &orderTxId, const CDEXSysOrder &buyOrder, CDBOpLogMap &dbOpLogMap);
    bool UndoSysOrder(CDBOpLogMap &dbOpLogMap);

    bool Flush() {
        activeOrderCache.Flush();
        sysOrderCache.Flush();
        return true;
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        sysOrderCache.SetBase(&pBaseIn->sysOrderCache);
    };

private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositKVCache< dbk::DEX_ACTIVE_ORDER,       uint256,             CDEXActiveOrder >     activeOrderCache;
    CCompositKVCache< dbk::DEX_SYS_ORDER,          uint256,             CDEXSysOrder >        sysOrderCache;
};

#endif //PERSIST_DEX_H
