// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexorder.h"
#include "config/chainparams.h"
#include "persistence/dbaccess.h"

using namespace dex;

namespace dex {

    ////////////////////////////////////////////////////////////////////////////////
    // COrderOperatorParams

    string COrderOperatorParams::ToString() const {
        return  strprintf("open_mode=%s", kOpenModeHelper.GetName(open_mode)) + ", " +
                strprintf("taker_fee_ratio=%llu", taker_fee_ratio) + ", " +
                strprintf("maker_fee_ratio=%s", maker_fee_ratio);
    }

    Object COrderOperatorParams::ToJson() const {
        json_spirit::Object obj;
        obj.push_back(Pair("open_mode", kOpenModeHelper.GetName(open_mode)));
        obj.push_back(Pair("taker_fee_ratio", taker_fee_ratio));
        obj.push_back(Pair("maker_fee_ratio", maker_fee_ratio));
        return obj;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXOrderDetail

    string CDEXOrderDetail::ToString() const {
        return  strprintf("generate_type=%s", GetOrderGenTypeName(generate_type)) + ", " +
                strprintf("order_type=%s", kOrderTypeHelper.GetName(order_type)) + ", " +
                strprintf("order_side=%s", kOrderSideHelper.GetName(order_side)) + ", " +
                strprintf("coin_symbol=%s", coin_symbol) + ", " +
                strprintf("asset_symbol=%s", asset_symbol) + ", " +
                strprintf("coin_amount=%llu", coin_amount) + ", " +
                strprintf("asset_amount=%llu", asset_amount) + ", " +
                strprintf("price=%llu", price) + ", " +

                strprintf("dex_id=%u", dex_id) + ", " +
                strprintf("has_operator_params=%d", opt_operator_params.has_value()) + ", " +
                strprintf("operator_params={%s}", opt_operator_params ? opt_operator_params->ToString() : "") + ", " +
                strprintf("tx_cord=%s", tx_cord.ToString()) + ", " +
                strprintf("user_regid=%s", user_regid.ToString()) + ", " +
                strprintf("total_deal_coin_amount=%llu", total_deal_coin_amount) + ", " +
                strprintf("total_deal_asset_amount=%llu", total_deal_asset_amount);
    }


    void CDEXOrderDetail::ToJson(json_spirit::Object &obj) const {
        obj.push_back(Pair("generate_type",             GetOrderGenTypeName(generate_type)));
        obj.push_back(Pair("order_type",                kOrderTypeHelper.GetName(order_type)));
        obj.push_back(Pair("order_side",                kOrderSideHelper.GetName(order_side)));
        obj.push_back(Pair("coin_symbol",               coin_symbol));
        obj.push_back(Pair("asset_symbol",              asset_symbol));
        obj.push_back(Pair("coin_amount",               coin_amount));
        obj.push_back(Pair("asset_amount",              asset_amount));
        obj.push_back(Pair("price",                     price));
        obj.push_back(Pair("dex_id",                    (int64_t)dex_id));
        obj.push_back(Pair("has_operator_config",       opt_operator_params.has_value()));
        if (opt_operator_params) {
            json_spirit::Object operatorObj;
            operatorObj.push_back(Pair("fee_ratios",    opt_operator_params.value().ToJson()));
            obj.push_back(Pair("operator_config",       opt_operator_params->ToJson()));
        }
        obj.push_back(Pair("tx_cord",                   tx_cord.ToString()));
        obj.push_back(Pair("user_regid",                user_regid.ToString()));
        obj.push_back(Pair("total_deal_coin_amount",    total_deal_coin_amount));
        obj.push_back(Pair("total_deal_asset_amount",   total_deal_asset_amount));
    }


    ///////////////////////////////////////////////////////////////////////////////
    // class CSysOrder

    shared_ptr<CDEXOrderDetail> CSysOrder::CreateBuyMarketOrder(const CTxCord &txCord,
                                                                const TokenSymbol &coinSymbol,
                                                                const TokenSymbol &assetSymbol,
                                                                uint64_t coinAmountIn) {
        return Create(ORDER_MARKET_PRICE, ORDER_BUY, txCord, coinSymbol, assetSymbol, coinAmountIn, 0);
    }

    shared_ptr<CDEXOrderDetail> CSysOrder::CreateSellMarketOrder(const CTxCord& txCord,
                                                                    const TokenSymbol& coinSymbol,
                                                                    const TokenSymbol& assetSymbol,
                                                                    uint64_t assetAmount) {
        return Create(ORDER_MARKET_PRICE, ORDER_SELL, txCord, coinSymbol, assetSymbol, 0, assetAmount);
    }

    shared_ptr<CDEXOrderDetail> CSysOrder::Create(OrderType orderType, OrderSide orderSide,
                                                    const CTxCord &txCord,
                                                    const TokenSymbol &coinSymbol,
                                                    const TokenSymbol &assetSymbol,
                                                    uint64_t coiAmountIn,
                                                    uint64_t assetAmountIn) {
        auto pSysOrder                = make_shared<CDEXOrderDetail>();
        pSysOrder->generate_type      = SYSTEM_GEN_ORDER;
        pSysOrder->order_type         = orderType;
        pSysOrder->order_side         = orderSide;
        pSysOrder->coin_symbol        = coinSymbol;
        pSysOrder->asset_symbol       = assetSymbol;
        pSysOrder->coin_amount        = coiAmountIn;
        pSysOrder->asset_amount       = assetAmountIn;
        pSysOrder->price              = 0;
        pSysOrder->dex_id             = 0;
        pSysOrder->opt_operator_params = {OpenMode::PUBLIC, 0, 0}; // no order fee for sys order
        pSysOrder->tx_cord            = txCord;
        pSysOrder->user_regid         = SysCfg().GetFcoinGenesisRegId();
        // other fields keep default value

        return pSysOrder;
    }
}

////////////////////////////////////////////////////////////////////////////////
// class DexOperatorDetail

const DexOperatorDetail DexOperatorDetail::EMPTY_OBJ = {};

string DexOperatorDetail::ToString() const {
    return  strprintf("owner_regid=%s", owner_regid.ToString()) + ", " +
            strprintf("fee_receiver_regid=%s", fee_receiver_regid.ToString()) + ", " +
            strprintf("name=%s", name) + ", " +
            strprintf("portal_url=%s", portal_url) + ", " +
            strprintf("order_open_mode=%llu", dex::kOpenModeHelper.GetName(order_open_mode)) + ", " +
            strprintf("maker_fee_ratio=%llu", maker_fee_ratio) + ", " +
            strprintf("taker_fee_ratio=%llu", taker_fee_ratio) + ", " +
            strprintf("order_open_dexop_set=%s", db_util::ToString(order_open_dexop_set)) + ", " +
            strprintf("memo=%s", memo) + ", " +
            strprintf("activated=%d", activated);
}