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

enum OrderSide {
    ORDER_BUY  = 0,
    ORDER_SELL = 1,
};

const static std::string OrderSideTitles[] = {"Buy", "Sell"};

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
    OrderSide       orderSide;
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
    OrderSide       order_side;     //!< order side: buy or sell
    OrderType       order_type;     //!< order type
    TokenSymbol     coin_symbol;    //!< coin type
    TokenSymbol     asset_symbol;   //!< asset type

    uint64_t        coin_amount;    //!< amount of coin to buy/sell asset
    uint64_t        asset_amount;   //!< amount of coin to buy asset
    uint64_t        price;          //!< price in coin_symbol want to buy/sell asset
public:// create functions
    static shared_ptr<CDEXSysOrder> CreateBuyLimitOrder(const TokenSymbol &coinSymbol, TokenSymbol &assetSymbol,
                                                        uint64_t assetAmountIn, uint64_t priceIn);

    static shared_ptr<CDEXSysOrder> CreateSellLimitOrder(const TokenSymbol &coinSymbol, TokenSymbol &assetSymbol,
                                                         uint64_t assetAmountIn, uint64_t priceIn);

    static shared_ptr<CDEXSysOrder> CreateBuyMarketOrder(const TokenSymbol &coinSymbol, TokenSymbol &assetSymbol,
                                                         uint64_t coinAmountIn);

    static shared_ptr<CDEXSysOrder> CreateSellMarketOrder(const TokenSymbol &coinSymbol, TokenSymbol &assetSymbol,
                                                          uint64_t assetAmountIn);

public:
    // default constructor
    CDEXSysOrder():
        order_side(ORDER_BUY),
        order_type(ORDER_LIMIT_PRICE),
        coin_symbol(SYMB::WUSD),
        asset_symbol(SYMB::WICC),
        coin_amount(0),
        asset_amount(0),
        price(0)
        { }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)order_side);
        READWRITE((uint8_t&)order_type);
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);

        READWRITE(VARINT(coin_amount));
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
    )

    string ToString() {
        return strprintf(
                "OrderDir=%s, order_side=%s, CoinSymbol=%d, AssetSymbol=%s, coinAmount=%lu, assetAmount=%lu, price=%lu",
                OrderSideTitles[order_side], OrderTypeTitles[order_type],
                coin_symbol, asset_symbol, coin_amount, asset_amount, price);
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
    CUserID cdp_owner_uid;
    uint64_t bcoins_amount;
    uint64_t scoins_amount;
    double collateral_ratio_by_amount; // fixed: 100 *  bcoinsAmount / scoinsAmount
    double collateral_ratio_by_value;  // collateralRatioAmount * wiccMedianPrice
    uint64_t order_discount; // *1000 E.g. 97% * 1000 = 970

};

class CDexDBCache {
public:
    CDexDBCache() {}
    CDexDBCache(CDBAccess *pDbAccess) : activeOrderCache(pDbAccess), sysOrderCache(pDbAccess) {};

public:
    bool GetActiveOrder(const uint256 &orderTxId, CDEXActiveOrder& activeOrder);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder& activeOrder);
    bool ModifyActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder& activeOrder);
    bool EraseActiveOrder(const uint256 &orderTxId);

    bool GetSysOrder(const uint256 &orderTxId, CDEXSysOrder &buyOrder);
    bool CreateSysOrder(const uint256 &orderTxId, const CDEXSysOrder &buyOrder);

    bool Flush() {
        activeOrderCache.Flush();
        sysOrderCache.Flush();
        return true;
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        sysOrderCache.SetBase(&pBaseIn->sysOrderCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        activeOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
        sysOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return activeOrderCache.UndoDatas() &&
               sysOrderCache.UndoDatas();
    }
private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositeKVCache< dbk::DEX_ACTIVE_ORDER,       uint256,             CDEXActiveOrder >     activeOrderCache;
    CCompositeKVCache< dbk::DEX_SYS_ORDER,          uint256,             CDEXSysOrder >        sysOrderCache;
};

#endif //PERSIST_DEX_H
