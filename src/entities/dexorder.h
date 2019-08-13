// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_DEX_ORDER_H
#define ENTITIES_DEX_ORDER_H

#include <string>

#include "id.h"
#include "asset.h"
#include "commons/types.h"

enum OrderSide: uint8_t {
    ORDER_BUY  = 1,
    ORDER_SELL = 2,
};

const static std::string OrderSideTitles[] = {"Buy", "Sell"};

enum OrderType: uint8_t {
    ORDER_LIMIT_PRICE   = 1, //!< limit price order type
    ORDER_MARKET_PRICE  = 2  //!< market price order type
};

const static std::string OrderTypeTitles[] = {"LimitPrice", "MarketPrice"};

enum OrderGenerateType {
    EMPTY_ORDER         = 0,
    USER_GEN_ORDER      = 1,
    SYSTEM_GEN_ORDER    = 2
};

static const EnumTypeMap<OrderGenerateType, string> ORDER_GEN_TYPE_NAMES = {
    {EMPTY_ORDER, "EMPTY_ORDER"},
    {USER_GEN_ORDER, "USER_GEN_ORDER"},
    {SYSTEM_GEN_ORDER, "SYSTEM_GEN_ORDER"}
};

inline const string &GetOrderGenTypeName(OrderGenerateType genType) {
    static const string empty = "";
    auto it = ORDER_GEN_TYPE_NAMES.find(genType);
    if (it != ORDER_GEN_TYPE_NAMES.end())
        return it->second;
    return empty;
}

struct CDEXOrderDetail {
    OrderGenerateType generate_type    = EMPTY_ORDER;       //!< generate type
    OrderType order_type               = ORDER_LIMIT_PRICE; //!< order type
    OrderSide order_side               = ORDER_BUY;         //!< order side
    TokenSymbol coin_symbol            = "";                //!< coin symbol
    TokenSymbol asset_symbol           = "";                //!< asset symbol
    uint64_t coin_amount               = 0;                 //!< amount of coin to buy/sell asset
    uint64_t asset_amount              = 0;                 //!< amount of asset to buy/sell
    uint64_t price                     = 0;                 //!< price in coinType want to buy/sell asset
    CTxCord  tx_cord                   = CTxCord();         //!< related tx cord
    CRegID user_regid                  = CRegID();          //!< user regid
    uint64_t total_deal_coin_amount    = 0;                 //!< total deal coin amount
    uint64_t total_deal_asset_amount   = 0;                 //!< total deal asset amount

public:
    static shared_ptr<CDEXOrderDetail> CreateUserBuyLimitOrder(
        const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol, const uint64_t assetAmountIn,
        const uint64_t priceIn, const CTxCord &txCord, const CRegID &userRegid);

public:
    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)generate_type);
        READWRITE((uint8_t&)order_type);
        READWRITE((uint8_t&)order_side);
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
        READWRITE(tx_cord);
        READWRITE(user_regid);
        READWRITE(VARINT(total_deal_coin_amount));
        READWRITE(VARINT(total_deal_asset_amount));

        READWRITE(tx_cord);
    )

    bool IsEmpty() const {
        return generate_type == EMPTY_ORDER;
    }
    void SetEmpty() {
        generate_type             = EMPTY_ORDER;
        order_type                = ORDER_LIMIT_PRICE;
        order_side                = ORDER_BUY;
        coin_symbol               = "";
        asset_symbol              = "";
        coin_amount               = 0;
        asset_amount              = 0;
        price                     = 0;
        tx_cord.SetEmpty();
        user_regid.SetEmpty();
        total_deal_coin_amount    = 0;
        total_deal_asset_amount   = 0;
    }

    string ToString() const;
};


// for all active order db: orderId -> CDEXActiveOrder
struct CDEXActiveOrder {
    OrderGenerateType generate_type     = EMPTY_ORDER;  //!< generate type
    CTxCord  tx_cord                   = CTxCord();    //!< related tx cord
    uint64_t total_deal_coin_amount    = 0;            //!< total deal coin amount
    uint64_t total_deal_asset_amount   = 0;            //!< total deal asset amount

    CDEXActiveOrder() {}
    
    CDEXActiveOrder(OrderGenerateType generateType, const CTxCord &txCord):
        generate_type(generateType), tx_cord(txCord)
    {}

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)generate_type);
        READWRITE(tx_cord);
        READWRITE(VARINT(total_deal_coin_amount));
        READWRITE(VARINT(total_deal_asset_amount));
    )

    bool IsEmpty() const {
        return generate_type == EMPTY_ORDER;
    }
    void SetEmpty() {
        generate_type  = EMPTY_ORDER;
        total_deal_coin_amount    = 0;
        total_deal_asset_amount   = 0;
        tx_cord.SetEmpty();
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
    static shared_ptr<CDEXOrderDetail> CreateBuyLimitOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
        const TokenSymbol &assetSymbol, const uint64_t assetAmountIn, const uint64_t priceIn);

    static shared_ptr<CDEXOrderDetail> CreateSellLimitOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
        const TokenSymbol &assetSymbol, const uint64_t assetAmountIn, const uint64_t priceIn);

    static shared_ptr<CDEXOrderDetail> CreateBuyMarketOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
        const TokenSymbol &assetSymbol, const uint64_t coinAmountIn);

    static shared_ptr<CDEXOrderDetail> CreateSellMarketOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
        const TokenSymbol &assetSymbol, const uint64_t assetAmountIn);

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
                "order_side=%s, order_type=%s, coin_symbol=%d, asset_symbol=%s, coin_amount=%lu, asset_amount=%lu, price=%lu",
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

#endif //ENTITIES_DEX_ORDER_H
