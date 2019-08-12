// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "main.h"

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
