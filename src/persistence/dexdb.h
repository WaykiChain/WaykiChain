// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <string>
#include <set>
#include <vector>

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
#include "entities/account.h"
#include "entities/dexorder.h"

using namespace std;

class CDexDBCache {
public:
    CDexDBCache() {}
    CDexDBCache(CDBAccess *pDbAccess) : activeOrderCache(pDbAccess), sysOrderCache(pDbAccess) {};

public:
    bool GetActiveOrder(const uint256 &orderTxId, CDEXOrderDetail& activeOrder);
    bool HaveActiveOrder(const uint256 &orderTxId);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool UpdateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool EraseActiveOrder(const uint256 &orderTxId);

    bool Flush() {
        activeOrderCache.Flush();
        sysOrderCache.Flush();
        return true;
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        sysOrderCache.SetBase(&pBaseIn->sysOrderCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        activeOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
        sysOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return activeOrderCache.UndoDatas() &&
               sysOrderCache.UndoDatas();
    }
private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositeKVCache< dbk::DEX_ACTIVE_ORDER,       uint256,             CDEXOrderDetail >     activeOrderCache;
    CCompositeKVCache< dbk::DEX_SYS_ORDER,          uint256,             CDEXSysOrder >        sysOrderCache;
};

#endif //PERSIST_DEX_H
