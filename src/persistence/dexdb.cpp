// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "accounts/account.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// class CDEXSysBuyOrder

bool CDEXSysBuyOrder::IsEmpty() const {
    return coinAmount == 0;
}
void CDEXSysBuyOrder::SetEmpty() {
    coinAmount = 0;
}

void CDEXSysBuyOrder::GetOrderDetail(CDEXOrderDetail &orderDetail) {
    orderDetail.userRegId = FcoinGenesisRegId;
    orderDetail.orderType = ORDER_MARKET_PRICE;     //!< order type
    orderDetail.direction = ORDER_BUY;
    orderDetail.coinType = coinType;      //!< coin type
    orderDetail.assetType = assetType;     //!< asset type
    orderDetail.coinAmount = coinAmount;    //!< amount of coin to buy asset
    orderDetail.assetAmount = 0;          //!< unknown assetAmount in order
    orderDetail.price = 0;                //!< unknown price in order
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSysSellOrder
bool CDEXSysSellOrder::IsEmpty() const {
    return assetAmount == 0;
}
void CDEXSysSellOrder::SetEmpty() {
    assetAmount = 0;
}

void CDEXSysSellOrder::GetOrderDetail(CDEXOrderDetail &orderDetail) const {
    orderDetail.userRegId = FcoinGenesisRegId;
    orderDetail.orderType = ORDER_MARKET_PRICE;     //!< order type
    orderDetail.direction = ORDER_BUY;
    orderDetail.coinType = coinType;          //!< coin type
    orderDetail.assetType = assetType;        //!< asset type
    orderDetail.coinAmount = 0;               //!< amount of coin to buy asset
    orderDetail.assetAmount = assetAmount;    //!< unknown assetAmount in order
    orderDetail.price = 0;                    //!< unknown price in order
}

///////////////////////////////////////////////////////////////////////////////
// class CDexDBCache

bool CDexDBCache::GetActiveOrder(const uint256 &orderTxId, CDEXActiveOrder &activeOrder) {
    return activeOrderCache.GetData(orderTxId, activeOrder);
};

bool CDexDBCache::CreateActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder &activeOrder,
                                    CDBOpLogMap& dbOpLogMap) {
    assert(!activeOrderCache.HaveData(orderTxId));
    return activeOrderCache.SetData(orderTxId, activeOrder, dbOpLogMap);
}

bool CDexDBCache::ModifyActiveOrder(const uint256 &orderTxId, const CDEXActiveOrder &activeOrder,
                                    CDBOpLogMap &dbOpLogMap) {
    return activeOrderCache.SetData(orderTxId, activeOrder, dbOpLogMap);
};

bool CDexDBCache::EraseActiveOrder(const uint256 &orderTxId, CDBOpLogMap &dbOpLogMap) {
    return activeOrderCache.EraseData(orderTxId, dbOpLogMap);
};

bool CDexDBCache::UndoActiveOrder(CDBOpLogMap &dbOpLogMap) {
    return activeOrderCache.UndoData(dbOpLogMap);
};

bool CDexDBCache::GetSysBuyOrder(const uint256 &orderTxId, CDEXSysBuyOrder &buyOrder) {
    return sysBuyOrderCache.GetData(orderTxId, buyOrder);
}

bool CDexDBCache::CreateSysBuyOrder(const uint256 &orderTxId, const CDEXSysBuyOrder &buyOrder,
                                    CDBOpLogMap &dbOpLogMap) {
    if (sysBuyOrderCache.HaveData(orderTxId)) {
        return ERRORMSG("CDexDBCache::CreateSysBuyOrder failed. the order exists. txid=%s",
                        orderTxId.ToString());
    }
    if (!sysBuyOrderCache.SetData(orderTxId, buyOrder, dbOpLogMap)) return false;
    CDEXActiveOrder activeOrder;
    activeOrder.generateType = SYSTEM_GEN_ORDER;  //!< generate type
    if (!CreateActiveOrder(orderTxId, activeOrder, dbOpLogMap)) return false;
    return true;
};

bool CDexDBCache::UndoSysBuyOrder(CDBOpLogMap &dbOpLogMap) {
    if (!UndoActiveOrder(dbOpLogMap)) return false;
    return sysBuyOrderCache.UndoData(dbOpLogMap);
}

bool CDexDBCache::GetSysSellOrder(const uint256 &orderTxId, CDEXSysSellOrder &sellOrder) {
    return sysSellOrderCache.GetData(orderTxId, sellOrder);
}

bool CDexDBCache::CreateSysSellOrder(const uint256 &orderTxId, const CDEXSysSellOrder &sellOrder,
                                     CDBOpLogMap &dbOpLogMap) {
    if (sysSellOrderCache.HaveData(orderTxId)) {
        return ERRORMSG("CDexDBCache::CreateSysBuyOrder failed. the order exists. txid=%s",
                        orderTxId.ToString());
    }
    if (!sysSellOrderCache.SetData(orderTxId, sellOrder, dbOpLogMap)) return false;
    CDEXActiveOrder activeOrder;
    activeOrder.generateType = SYSTEM_GEN_ORDER;  //!< generate type
    if (!CreateActiveOrder(orderTxId, activeOrder, dbOpLogMap)) return false;
    return true;
};

bool CDexDBCache::UndoSysSellOrder(CDBOpLogMap &dbOpLogMap) {
    if (!UndoActiveOrder(dbOpLogMap)) return false;
    return sysSellOrderCache.UndoData(dbOpLogMap);
    return true;
}
