// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cachewrapper.h"
#include "main.h"

CCacheWrapper::CCacheWrapper() {}

CCacheWrapper::CCacheWrapper(CSysParamDBCache* pSysParamCacheIn,
                             CAccountDBCache* pAccountCacheIn,
                             CContractDBCache* pContractCacheIn,
                             CDelegateDBCache* pDelegateCacheIn,
                             CCdpDBCache* pCdpCacheIn,
                             CDexDBCache* pDexCacheIn,
                             CTxReceiptDBCache* pTxReceiptCacheIn,
                             CTxMemCache* pTxCacheIn) {
    sysParamCache.SetBaseViewPtr(pSysParamCacheIn);
    accountCache.SetBaseViewPtr(pAccountCacheIn);
    contractCache.SetBaseViewPtr(pContractCacheIn);
    delegateCache.SetBaseViewPtr(pDelegateCacheIn);
    cdpCache.SetBaseViewPtr(pCdpCacheIn);
    dexCache.SetBaseViewPtr(pDexCacheIn);
    txReceiptCache.SetBaseViewPtr(pTxReceiptCacheIn);
    txCache.SetBaseViewPtr(pTxCacheIn);
}

CCacheWrapper::CCacheWrapper(CCacheWrapper& cwIn) {
    sysParamCache.SetBaseViewPtr(&cwIn.sysParamCache);
    accountCache.SetBaseViewPtr(&cwIn.accountCache);
    contractCache.SetBaseViewPtr(&cwIn.contractCache);
    delegateCache.SetBaseViewPtr(&cwIn.delegateCache);
    cdpCache.SetBaseViewPtr(&cwIn.cdpCache);
    dexCache.SetBaseViewPtr(&cwIn.dexCache);
    txReceiptCache.SetBaseViewPtr(&cwIn.txReceiptCache);
    txCache.SetBaseViewPtr(&cwIn.txCache);
}

CCacheWrapper::CCacheWrapper(CCacheDBManager* pCdMan) {
    sysParamCache.SetBaseViewPtr(pCdMan->pSysParamCache);
    accountCache.SetBaseViewPtr(pCdMan->pAccountCache);
    contractCache.SetBaseViewPtr(pCdMan->pContractCache);
    delegateCache.SetBaseViewPtr(pCdMan->pDelegateCache);
    cdpCache.SetBaseViewPtr(pCdMan->pCdpCache);
    dexCache.SetBaseViewPtr(pCdMan->pDexCache);
    txReceiptCache.SetBaseViewPtr(pCdMan->pTxReceiptCache);
    txCache.SetBaseViewPtr(pCdMan->pTxCache);
}

void CCacheWrapper::Flush() {
    sysParamCache.Flush();
    accountCache.Flush();
    contractCache.Flush();
    delegateCache.Flush();
    cdpCache.Flush();
    dexCache.Flush();
    txReceiptCache.Flush();
    txCache.Flush();
}