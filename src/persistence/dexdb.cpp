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

bool CDexDBCache::GetActiveOrder(const uint256 &orderTxId, CDEXOrderDetail &activeOrder) {
    return activeOrderCache.GetData(orderTxId, activeOrder);
};

bool CDexDBCache::HaveActiveOrder(const uint256 &orderTxId) {
    return activeOrderCache.HaveData(orderTxId);
};

bool CDexDBCache::CreateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail &activeOrder) {
    assert(!activeOrderCache.HaveData(orderTxId));
    return activeOrderCache.SetData(orderTxId, activeOrder);
}

bool CDexDBCache::UpdateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail &activeOrder) {
    return activeOrderCache.SetData(orderTxId, activeOrder);
};

bool CDexDBCache::EraseActiveOrder(const uint256 &orderTxId) {
    return activeOrderCache.EraseData(orderTxId);
};

