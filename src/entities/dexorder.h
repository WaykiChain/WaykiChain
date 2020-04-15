// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_DEX_ORDER_H
#define ENTITIES_DEX_ORDER_H

#include <string>

#include "id.h"
#include "asset.h"
#include "commons/util/enumhelper.hpp"
#include "commons/json/json_spirit.h"

typedef uint32_t DexID;

static const DexID DEX_RESERVED_ID = 0;

namespace dex {

    enum OrderSide: uint8_t {
        ORDER_SIDE_NULL = 0,
        ORDER_BUY       = 1,
        ORDER_SELL      = 2,
    };

    static const EnumHelper<OrderSide, uint8_t> kOrderSideHelper = {
        {
            {ORDER_BUY, "BUY"},
            {ORDER_SELL, "SELL"}
        }
    };

    enum OrderType: uint8_t {
        ORDER_TYPE_NULL     = 0,
        ORDER_LIMIT_PRICE   = 1, //!< limit price order type
        ORDER_MARKET_PRICE  = 2  //!< market price order type
    };

    static const EnumHelper<OrderType, uint8_t> kOrderTypeHelper = {
        {
            {ORDER_LIMIT_PRICE, "LIMIT_PRICE"},
            {ORDER_MARKET_PRICE, "MARKET_PRICE"}
        }
    };

////////////////////////////////////////////////////////////////////////////////
// enum class OpenMode

    enum class OpenMode: uint8_t {
        PUBLIC   = 1,
        PRIVATE  = 2
    };

    static const OpenMode PUBLIC_MODE_DEFAULT = OpenMode::PRIVATE;

    static const EnumHelper<OpenMode, uint8_t> kPublicModeHelper (
        {
            {OpenMode::PUBLIC, "PUBLIC"},
            {OpenMode::PRIVATE, "PRIVATE"}
        }
    );

////////////////////////////////////////////////////////////////////////////////
// enum OrderGenerateType

    enum OrderGenerateType: uint8_t {
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
        auto it = ORDER_GEN_TYPE_NAMES.find(genType);
        if (it != ORDER_GEN_TYPE_NAMES.end())
            return it->second;
        return EMPTY_STRING;
    }

    struct COrderOperatorParams {
        OpenMode public_mode   = OpenMode::PUBLIC;//!< order public mode
        uint64_t maker_fee_ratio = 0; //!< match fee ratio
        uint64_t taker_fee_ratio = 0; //!< taker fee ratio

        IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(maker_fee_ratio));
            READWRITE(VARINT(taker_fee_ratio));
        )

        string ToString() const;
        Object ToJson() const;
    };

    struct CDEXOrderDetail {
        OrderGenerateType generate_type = EMPTY_ORDER;       //!< generate type
        dex::OrderType order_type       = dex::ORDER_LIMIT_PRICE; //!< order type
        OrderSide order_side            = ORDER_BUY;         //!< order side
        TokenSymbol coin_symbol         = "";                //!< coin symbol
        TokenSymbol asset_symbol        = "";                //!< asset symbol
        uint64_t coin_amount            = 0;                 //!< amount of coin to buy/sell asset
        uint64_t asset_amount           = 0;                 //!< amount of asset to buy/sell
        uint64_t price                  = 0;                //!< price in coinType want to buy/sell asset
        DexID dex_id                    = 0;                //!< dex id of operator
        optional<COrderOperatorParams> opt_operator_params; //!< operator params, optional
        CTxCord tx_cord                  = CTxCord();       //!< related tx cord
        CRegID user_regid                = CRegID();        //!< user regid
        uint64_t total_deal_coin_amount  = 0;               //!< total deal coin amount
        uint64_t total_deal_asset_amount = 0;               //!< total deal asset amount

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
            READWRITE(VARINT(dex_id));
            READWRITE(opt_operator_params);
            READWRITE(tx_cord);
            READWRITE(user_regid);
            READWRITE(VARINT(total_deal_coin_amount));
            READWRITE(VARINT(total_deal_asset_amount));
        )

        bool IsEmpty() const {
            return generate_type == EMPTY_ORDER;
        }
        void SetEmpty() {
            *this = CDEXOrderDetail();
        }

        string ToString() const;
        void ToJson(json_spirit::Object &obj) const;
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
    class CSysOrder {
    public:// create functions
        static shared_ptr<CDEXOrderDetail> CreateBuyMarketOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
            const TokenSymbol &assetSymbol, uint64_t coinAmountIn);

        static shared_ptr<CDEXOrderDetail> CreateSellMarketOrder(const CTxCord &txCord, const TokenSymbol &coinSymbol,
            const TokenSymbol &assetSymbol, uint64_t assetAmountIn);

        static shared_ptr<CDEXOrderDetail> Create(OrderType orderType, OrderSide orderSide, const CTxCord &txCord,
            const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol, uint64_t coiAmountIn, uint64_t assetAmountIn);
    };

}

// dex operator
struct DexOperatorDetail {
    CRegID owner_regid;                 // owner regid of exchange
    CRegID fee_receiver_regid;          // fee receiver regid
    string name              = "";       // domain name
    string portal_url        = "";       // portal url of dex operator
    dex::OpenMode public_mode   = dex::OpenMode::PRIVATE; // the default public mode for creating order
    uint64_t maker_fee_ratio = 0;    // the default maker fee ratio for creating order
    uint64_t taker_fee_ratio = 0;    // the defalt taker fee ratio for creating order
    set<uint64_t> shared_dexop_set;    // the defalt taker fee ratio for creating order
    string memo              = "";
    bool activated           = false;

    static const DexOperatorDetail EMPTY_OBJ;

    IMPLEMENT_SERIALIZE(
        READWRITE(owner_regid);
        READWRITE(fee_receiver_regid);
        READWRITE(name);
        READWRITE(portal_url);
        READWRITE_ENUM(public_mode, uint8_t);
        READWRITE(VARINT(maker_fee_ratio));
        READWRITE(VARINT(taker_fee_ratio));
        READWRITE(activated) ;
        READWRITE(memo);
    )

    bool IsEmpty() const {
        return owner_regid.IsEmpty() && fee_receiver_regid.IsEmpty() && name.empty() && portal_url.empty() &&
               maker_fee_ratio == 0 && taker_fee_ratio == 0 && memo.empty();
    }

    void SetEmpty() {
        *this = EMPTY_OBJ;
    }

    string ToString() const {
        return  strprintf("owner_regid=%s", owner_regid.ToString()) + ", " +
                strprintf("fee_receiver_regid=%s", fee_receiver_regid.ToString()) + ", " +
                strprintf("name=%s", name) + ", " +
                strprintf("portal_url=%s", portal_url) + ", " +
                strprintf("maker_fee_ratio=%llu", maker_fee_ratio) + ", " +
                strprintf("taker_fee_ratio=%llu", taker_fee_ratio) + ", " +
                strprintf("memo=%s", memo) + ", " +
                strprintf("activated=%d", activated);
    }
};

#endif //ENTITIES_DEX_ORDER_H
