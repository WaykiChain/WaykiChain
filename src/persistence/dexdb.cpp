// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "accounts/account.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// class CDexDBCache

bool CDexDBCache::GetActiveOrder(const uint256& orderTxId, CDEXActiveOrder& activeOrder) {
    return activeOrderCache.GetData(orderTxId, activeOrder);
};

bool CDexDBCache::CreateActiveOrder(const uint256& orderTxId, const CDEXActiveOrder& activeOrder,
                                    CDBOpLogMap& dbOpLogMap) {
    assert(!activeOrderCache.HaveData(orderTxId));
    return activeOrderCache.SetData(orderTxId, activeOrder, dbOpLogMap);
}

bool CDexDBCache::ModifyActiveOrder(const uint256& orderTxId, const CDEXActiveOrder& activeOrder,
                                    CDBOpLogMap& dbOpLogMap) {
    return activeOrderCache.SetData(orderTxId, activeOrder, dbOpLogMap);
};

bool CDexDBCache::EraseActiveOrder(const uint256& orderTxId, CDBOpLogMap& dbOpLogMap) {
    return activeOrderCache.EraseData(orderTxId, dbOpLogMap);
};

bool CDexDBCache::UndoActiveOrder(CDBOpLogMap& dbOpLogMap) {
    return activeOrderCache.UndoData(dbOpLogMap);
};

bool CDexDBCache::CreateSysBuyOrder(uint256 orderTxId, CDEXSysBuyOrder& buyOrder, CDBOpLogMap &dbOpLogMap) {
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

bool CDexDBCache::CreateSysSellOrder(uint256 orderTxId, CDEXSysSellOrder& sellOrder, CDBOpLogMap &dbOpLogMap) {
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
