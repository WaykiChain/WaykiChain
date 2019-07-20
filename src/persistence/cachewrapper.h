// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "cdpdb.h"
#include "contractdb.h"
#include "delegatedb.h"
#include "dexdb.h"
#include "pricefeeddb.h"
#include "sysparamdb.h"
#include "txdb.h"
#include "txreceiptdb.h"

struct CCacheWrapper {
    CSysParamDBCache    sysParamCache;
    CAccountDBCache     accountCache;
    CContractDBCache    contractCache;
    CDelegateDBCache    delegateCache;
    CCdpDBCache         cdpCache;
    CDexDBCache         dexCache;
    CTxReceiptDBCache   txReceiptCache;

    CTxMemCache         txCache;
    CPricePointMemCache ppCache;

    CTxUndo             txUndo;

    CCacheWrapper() {}

    CCacheWrapper(  CSysParamDBCache*   pSysParamCacheIn,
                    CAccountDBCache*    pAccountCacheIn,
                    CContractDBCache*   pContractCacheIn,
                    CDelegateDBCache*   pDelegateCacheIn,
                    CCdpDBCache*        pCdpCacheIn,
                    CDexDBCache*        pDexCacheIn,
                    CTxReceiptDBCache*  pTxReceiptCacheIn) {

        sysParamCache.SetBaseViewPtr(pSysParamCacheIn);
        accountCache.SetBaseViewPtr(pAccountCacheIn);
        contractCache.SetBaseViewPtr(pContractCacheIn);
        delegateCache.SetBaseViewPtr(pDelegateCacheIn);
        cdpCache.SetBaseViewPtr(pCdpCacheIn);
        dexCache.SetBaseViewPtr(pDexCacheIn);
        txReceiptCache.SetBaseViewPtr(pTxReceiptCacheIn);
    }

    CCacheWrapper(CCacheWrapper &cwIn) {
        sysParamCache.SetBaseViewPtr(&cwIn.sysParamCacheIn);
        accountCache.SetBaseViewPtr (&cwIn.accountCacheIn);
        contractCache.SetBaseViewPtr(&cwIn.contractCacheIn);
        delegateCache.SetBaseViewPtr(&cwIn.delegateCacheIn);
        cdpCache.SetBaseViewPtr     (&cwIn.cdpCacheIn);
        dexCache.SetBaseViewPtr     (&cwIn.dexCacheIn);
        txReceiptCache.SetBaseViewPtr(&cwIn.txReceiptCacheIn);
    }

    CCacheWrapper(CCacheDBManager *pCdMan) {
        sysParamCache.SetBaseViewPtr(pCdMan->pSysParamCacheIn);
        accountCache.SetBaseViewPtr(pCdMan->pAccountCacheIn);
        contractCache.SetBaseViewPtr(pCdMan->pContractCacheIn);
        delegateCache.SetBaseViewPtr(pCdMan->pDelegateCacheIn);
        cdpCache.SetBaseViewPtr(pCdMan->pCdpCacheIn);
        dexCache.SetBaseViewPtr(pCdMan->pDexCacheIn);
        txReceiptCache.SetBaseViewPtr(pCdMan->pTxReceiptCacheIn);
    }

    void Flush() {
        accountCache.Flush();
        contractCache.Flush();
        delegateCache.Flush();
        cdpCache.Flush();
        dexCache.Flush();
        sysParamCache.Flush();
        txReceiptCache.Flush();
    }
};

#endif //PERSIST_CACHEWRAPPER_H