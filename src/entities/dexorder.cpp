// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexorder.h"
#include "config/chainparams.h"


///////////////////////////////////////////////////////////////////////////////
// class CDEXOrderDetail

string CDEXOrderDetail::ToString() const {
    return strprintf(
            "generate_type=%s, order_side=%s, order_type=%s, coin_symbol=%d, asset_symbol=%s, coin_amount=%lu,"
            " asset_amount=%lu, price=%lu, tx_cord=%s, user_regid=%s, total_deal_coin_amount=%llu,"
            " total_deal_asset_amount=%llu",
            GetOrderGenTypeName(generate_type), OrderSideTitles[order_side], OrderTypeTitles[order_type],
            coin_symbol, asset_symbol, coin_amount, asset_amount, price, tx_cord.ToString(), 
            user_regid.ToString(), total_deal_coin_amount, total_deal_asset_amount);
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSysOrder

shared_ptr<CDEXOrderDetail> CDEXSysOrder::CreateBuyLimitOrder(const CTxCord &txCord, const TokenSymbol& coinTypeIn,
    const AssetSymbol& assetTypeIn, const uint64_t assetAmountIn, const uint64_t priceIn) {

    auto pSysOrder           = make_shared<CDEXOrderDetail>();
    pSysOrder->generate_type = SYSTEM_GEN_ORDER;
    pSysOrder->order_type    = ORDER_LIMIT_PRICE;
    pSysOrder->order_side    = ORDER_BUY;
    pSysOrder->coin_symbol   = coinTypeIn;
    pSysOrder->asset_symbol  = assetTypeIn;
    pSysOrder->coin_amount  = 0; // unknown in buy limit order
    pSysOrder->asset_amount = assetAmountIn;
    pSysOrder->price        = priceIn;
    pSysOrder->tx_cord      = txCord;
    pSysOrder->user_regid   = SysCfg().GetFcoinGenesisRegId();
    // other fields keep default value
    return pSysOrder;
}

shared_ptr<CDEXOrderDetail> CDEXSysOrder::CreateSellLimitOrder(const CTxCord& txCord, const TokenSymbol& coinTypeIn,
    const AssetSymbol& assetTypeIn, const uint64_t assetAmountIn, const uint64_t priceIn) {

    auto pSysOrder           = make_shared<CDEXOrderDetail>();
    pSysOrder->generate_type = SYSTEM_GEN_ORDER;
    pSysOrder->order_type    = ORDER_LIMIT_PRICE;
    pSysOrder->order_side    = ORDER_SELL;
    pSysOrder->coin_symbol   = coinTypeIn;
    pSysOrder->asset_symbol  = assetTypeIn;
    pSysOrder->coin_amount   = 0;  // unknown in sell limit order
    pSysOrder->asset_amount  = assetAmountIn;
    pSysOrder->price         = priceIn;
    pSysOrder->tx_cord       = txCord;
    pSysOrder->user_regid    = SysCfg().GetFcoinGenesisRegId();
    // other fields keep default value

    return pSysOrder;
}

shared_ptr<CDEXOrderDetail> CDEXSysOrder::CreateBuyMarketOrder(const CTxCord& txCord, const TokenSymbol& coinTypeIn,
    const AssetSymbol& assetTypeIn, const uint64_t coinAmountIn) {
        
    auto pSysOrder           = make_shared<CDEXOrderDetail>();
    pSysOrder->generate_type = SYSTEM_GEN_ORDER;
    pSysOrder->order_type    = ORDER_MARKET_PRICE;
    pSysOrder->order_side    = ORDER_BUY;
    pSysOrder->coin_symbol   = coinTypeIn;
    pSysOrder->asset_symbol  = assetTypeIn;
    pSysOrder->coin_amount   = coinAmountIn;
    pSysOrder->asset_amount  = 0; // unknown in buy market order
    pSysOrder->price         = 0; // unknown in buy market order
    pSysOrder->tx_cord       = txCord;
    pSysOrder->user_regid    = SysCfg().GetFcoinGenesisRegId();
    // other fields keep default value
    
    return pSysOrder;
}

shared_ptr<CDEXOrderDetail> CDEXSysOrder::CreateSellMarketOrder(const CTxCord& txCord, const TokenSymbol &coinTypeIn, 
    const AssetSymbol &assetTypeIn, const uint64_t assetAmountIn) {

    auto pSysOrder           = make_shared<CDEXOrderDetail>();
    pSysOrder->generate_type = SYSTEM_GEN_ORDER;
    pSysOrder->order_type    = ORDER_MARKET_PRICE;
    pSysOrder->order_side    = ORDER_BUY;
    pSysOrder->coin_symbol   = coinTypeIn;
    pSysOrder->asset_symbol  = assetTypeIn;
    pSysOrder->coin_amount   = 0; // unknown in buy market order
    pSysOrder->asset_amount  = assetAmountIn;
    pSysOrder->price         = 0; // unknown in buy market order
    pSysOrder->tx_cord       = txCord;
    pSysOrder->user_regid    = SysCfg().GetFcoinGenesisRegId();
    // other fields keep default value

    return pSysOrder;
}

bool CDEXSysOrder::IsEmpty() const { return coin_amount == 0 && asset_amount == 0 && price == 0; }

void CDEXSysOrder::SetEmpty() {
    coin_amount  = 0;
    asset_amount = 0;
    price        = 0;
}

void CDEXSysOrder::GetOrderDetail(CDEXOrderDetail &orderDetail) const {
    orderDetail.user_regid   = SysCfg().GetFcoinGenesisRegId();
    orderDetail.order_type   = order_type;
    orderDetail.order_side   = order_side;
    orderDetail.coin_symbol  = coin_symbol;
    orderDetail.asset_symbol = asset_symbol;
    orderDetail.coin_amount  = coin_amount;
    orderDetail.asset_amount = asset_amount;
    orderDetail.price        = price;
}

