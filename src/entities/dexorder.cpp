// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexorder.h"
#include "config/chainparams.h"

///////////////////////////////////////////////////////////////////////////////
// class CDEXSysOrder

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateBuyLimitOrder(const TokenSymbol& coinTypeIn,
                                                           const AssetSymbol& assetTypeIn,
                                                           const uint64_t assetAmountIn,
                                                           const uint64_t priceIn) {
    auto pSysOrder          = make_shared<CDEXSysOrder>();
    pSysOrder->order_side   = ORDER_BUY;
    pSysOrder->order_type   = ORDER_LIMIT_PRICE;
    pSysOrder->coin_symbol  = coinTypeIn;
    pSysOrder->asset_symbol = assetTypeIn;

    pSysOrder->asset_amount = assetAmountIn;
    pSysOrder->price        = priceIn;
    assert(pSysOrder->coin_amount == 0);
    return pSysOrder;
}

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateSellLimitOrder(const TokenSymbol& coinTypeIn,
                                                            const AssetSymbol& assetTypeIn,
                                                            const uint64_t assetAmountIn,
                                                            const uint64_t priceIn) {
    auto pSysOrder          = make_shared<CDEXSysOrder>();
    pSysOrder->order_side   = ORDER_SELL;
    pSysOrder->order_type   = ORDER_LIMIT_PRICE;
    pSysOrder->coin_symbol  = coinTypeIn;
    pSysOrder->asset_symbol = assetTypeIn;

    pSysOrder->asset_amount = assetAmountIn;
    pSysOrder->price        = priceIn;
    assert(pSysOrder->coin_amount == 0);
    return pSysOrder;
}

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateBuyMarketOrder(const TokenSymbol& coinTypeIn,
                                                            const AssetSymbol& assetTypeIn,
                                                            const uint64_t coinAmountIn) {
    auto pSysOrder          = make_shared<CDEXSysOrder>();
    pSysOrder->order_side   = ORDER_BUY;
    pSysOrder->order_type   = ORDER_MARKET_PRICE;
    pSysOrder->coin_symbol  = coinTypeIn;
    pSysOrder->asset_symbol = assetTypeIn;

    pSysOrder->coin_amount = coinAmountIn;
    assert(pSysOrder->asset_amount == 0);
    assert(pSysOrder->price == 0);
    return pSysOrder;
}

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateSellMarketOrder(const TokenSymbol &coinTypeIn,
                                                             const AssetSymbol &assetTypeIn,
                                                             const uint64_t assetAmountIn) {
    auto pSysOrder          = make_shared<CDEXSysOrder>();
    pSysOrder->order_side   = ORDER_BUY;
    pSysOrder->order_type   = ORDER_MARKET_PRICE;
    pSysOrder->coin_symbol  = coinTypeIn;
    pSysOrder->asset_symbol = assetTypeIn;

    pSysOrder->asset_amount = assetAmountIn;
    assert(pSysOrder->coin_amount == 0);
    assert(pSysOrder->price == 0);
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