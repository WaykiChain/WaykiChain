// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// class CDEXSysOrder

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateBuyLimitOrder(TokenSymbol coinTypeIn,
                                                           AssetSymbol assetTypeIn,
                                                           uint64_t assetAmountIn,
                                                           uint64_t priceIn) {
    auto pSysOrder       = make_shared<CDEXSysOrder>();
    pSysOrder->direction = ORDER_BUY;
    pSysOrder->orderType = ORDER_LIMIT_PRICE;
    pSysOrder->coinType  = coinTypeIn;
    pSysOrder->assetType = assetTypeIn;

    pSysOrder->assetAmount = assetAmountIn;
    pSysOrder->price       = priceIn;
    assert(pSysOrder->coinAmount == 0);
    return pSysOrder;
}

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateSellLimitOrder(TokenSymbol coinTypeIn,
                                                            AssetSymbol assetTypeIn,
                                                            uint64_t assetAmountIn,
                                                            uint64_t priceIn) {
    auto pSysOrder       = make_shared<CDEXSysOrder>();
    pSysOrder->direction = ORDER_SELL;
    pSysOrder->orderType = ORDER_LIMIT_PRICE;
    pSysOrder->coinType  = coinTypeIn;
    pSysOrder->assetType = assetTypeIn;

    pSysOrder->assetAmount = assetAmountIn;
    pSysOrder->price       = priceIn;
    assert(pSysOrder->coinAmount == 0);
    return pSysOrder;
}

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateBuyMarketOrder(TokenSymbol coinTypeIn,
                                                            AssetSymbol assetTypeIn,
                                                            uint64_t coinAmountIn) {
    auto pSysOrder       = make_shared<CDEXSysOrder>();
    pSysOrder->direction = ORDER_BUY;
    pSysOrder->orderType = ORDER_MARKET_PRICE;
    pSysOrder->coinType  = coinTypeIn;
    pSysOrder->assetType = assetTypeIn;

    pSysOrder->coinAmount = coinAmountIn;
    assert(pSysOrder->assetAmount == 0);
    assert(pSysOrder->price == 0);
    return pSysOrder;
}

shared_ptr<CDEXSysOrder> CDEXSysOrder::CreateSellMarketOrder(TokenSymbol coinTypeIn,
                                                             AssetSymbol assetTypeIn,
                                                             uint64_t assetAmountIn) {
    auto pSysOrder       = make_shared<CDEXSysOrder>();
    pSysOrder->direction = ORDER_BUY;
    pSysOrder->orderType = ORDER_MARKET_PRICE;
    pSysOrder->coinType  = coinTypeIn;
    pSysOrder->assetType = assetTypeIn;

    pSysOrder->assetAmount = assetAmountIn;
    assert(pSysOrder->coinAmount == 0);
    assert(pSysOrder->price == 0);
    return pSysOrder;
}

bool CDEXSysOrder::IsEmpty() const {
    return  coinAmount == 0
         && assetAmount == 0
         && price == 0;
}
void CDEXSysOrder::SetEmpty() {
    coinAmount = 0;
    assetAmount = 0;
    price = 0;
}

void CDEXSysOrder::GetOrderDetail(CDEXOrderDetail &orderDetail) const {
    orderDetail.userRegId = SysCfg().GetFcoinGenesisRegId();
    orderDetail.orderType = orderType;
    orderDetail.direction = direction;
    orderDetail.coinType = coinType;
    orderDetail.assetType = assetType;
    orderDetail.coinAmount = coinAmount;
    orderDetail.assetAmount = assetAmount;
    orderDetail.price = price;
}


///////////////////////////////////////////////////////////////////////////////
// class CDexDBCache

bool CDexDBCache::GetActiveOrder(const uint256 &orderTxId, CDEXActiveOrder &activeOrder) {
    return activeOrderCache.GetData(orderTxId, activeOrder);
};

bool CDexDBCache::CreateActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder &activeOrder) {
    assert(!activeOrderCache.HaveData(orderTxId));
    return activeOrderCache.SetData(orderTxId, activeOrder);
}

bool CDexDBCache::ModifyActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder &activeOrder) {
    return activeOrderCache.SetData(orderTxId, activeOrder);
};

bool CDexDBCache::EraseActiveOrder(const uint256 &orderTxId) {
    return activeOrderCache.EraseData(orderTxId);
};

bool CDexDBCache::GetSysOrder(const uint256 &orderTxId, CDEXSysOrder &sysOrder) {
    return sysOrderCache.GetData(orderTxId, sysOrder);
}

bool CDexDBCache::CreateSysOrder(const uint256 &orderTxId, const CDEXSysOrder &buyOrder) {
    if (sysOrderCache.HaveData(orderTxId)) {
        return ERRORMSG("CDexDBCache::CreateOrder failed. the order exists. txid=%s",
                        orderTxId.ToString());
    }
    if (!sysOrderCache.SetData(orderTxId, buyOrder)) return false;
    CDEXActiveOrder activeOrder;
    activeOrder.generate_type = SYSTEM_GEN_ORDER;  //!< generate type
    if (!CreateActiveOrder(orderTxId, activeOrder)) return false;
    return true;
};
