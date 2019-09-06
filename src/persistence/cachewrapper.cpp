// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cachewrapper.h"
#include "main.h"

CCacheWrapper::CCacheWrapper() {}

CCacheWrapper::CCacheWrapper(CSysParamDBCache* pSysParamCacheIn,
                             CAccountDBCache* pAccountCacheIn,
                             CAssetDBCache* pAssetCache,
                             CContractDBCache* pContractCacheIn,
                             CDelegateDBCache* pDelegateCacheIn,
                             CCDPDBCache* pCdpCacheIn,
                             CDexDBCache* pDexCacheIn,
                             CTxReceiptDBCache* pTxReceiptCacheIn,
                             CTxMemCache* pTxCacheIn,
                             CPricePointMemCache *pPpCacheIn) {
    sysParamCache.SetBaseViewPtr(pSysParamCacheIn);
    accountCache.SetBaseViewPtr(pAccountCacheIn);
    assetCache.SetBaseViewPtr(pAssetCache);
    contractCache.SetBaseViewPtr(pContractCacheIn);
    delegateCache.SetBaseViewPtr(pDelegateCacheIn);
    cdpCache.SetBaseViewPtr(pCdpCacheIn);
    dexCache.SetBaseViewPtr(pDexCacheIn);
    txReceiptCache.SetBaseViewPtr(pTxReceiptCacheIn);

    txCache.SetBaseViewPtr(pTxCacheIn);
    ppCache.SetBaseViewPtr(pPpCacheIn);
}

CCacheWrapper::CCacheWrapper(CCacheWrapper& cwIn) {
    sysParamCache.SetBaseViewPtr(&cwIn.sysParamCache);
    accountCache.SetBaseViewPtr(&cwIn.accountCache);
    assetCache.SetBaseViewPtr(&cwIn.assetCache);
    contractCache.SetBaseViewPtr(&cwIn.contractCache);
    delegateCache.SetBaseViewPtr(&cwIn.delegateCache);
    cdpCache.SetBaseViewPtr(&cwIn.cdpCache);
    dexCache.SetBaseViewPtr(&cwIn.dexCache);
    txReceiptCache.SetBaseViewPtr(&cwIn.txReceiptCache);

    txCache.SetBaseViewPtr(&cwIn.txCache);
    ppCache.SetBaseViewPtr(&cwIn.ppCache);
}

CCacheWrapper::CCacheWrapper(CCacheDBManager* pCdMan) {
    sysParamCache.SetBaseViewPtr(pCdMan->pSysParamCache);
    accountCache.SetBaseViewPtr(pCdMan->pAccountCache);
    assetCache.SetBaseViewPtr(pCdMan->pAssetCache);
    contractCache.SetBaseViewPtr(pCdMan->pContractCache);
    delegateCache.SetBaseViewPtr(pCdMan->pDelegateCache);
    cdpCache.SetBaseViewPtr(pCdMan->pCdpCache);
    dexCache.SetBaseViewPtr(pCdMan->pDexCache);
    txReceiptCache.SetBaseViewPtr(pCdMan->pTxReceiptCache);

    txCache.SetBaseViewPtr(pCdMan->pTxCache);
    ppCache.SetBaseViewPtr(pCdMan->pPpCache);
}

void CCacheWrapper::EnableTxUndoLog(const uint256 &txid) {
    txUndo.Clear();
    SetDbOpMapLog(&txUndo.dbOpLogMap);
}

void CCacheWrapper::DisableTxUndoLog() {
    SetDbOpMapLog(nullptr);
}

bool CCacheWrapper::UndoDatas(CBlockUndo &blockUndo) {
    for (auto it = blockUndo.vtxundo.rbegin(); it != blockUndo.vtxundo.rend(); it++) {
        // TODO: should use foreach(it->dbOpLogMap) to dispatch the DbOpLog to the cache (switch case)
        SetDbOpMapLog(&it->dbOpLogMap);
        bool ret = sysParamCache.UndoDatas() &&
                    accountCache.UndoDatas() &&
                    assetCache.UndoDatas() &&
                    contractCache.UndoDatas() &&
                    delegateCache.UndoDatas() &&
                    cdpCache.UndoDatas() &&
                    dexCache.UndoDatas() &&
                    txReceiptCache.UndoDatas();
        if (!ret) {
            return ERRORMSG("CCacheWrapper::UndoDatas() : undo datas of tx failed! txUndo=%s", txUndo.ToString());
        }
    }
    return true;
}


void CCacheWrapper::Flush() {
    sysParamCache.Flush();
    accountCache.Flush();
    assetCache.Flush();
    contractCache.Flush();
    delegateCache.Flush();
    cdpCache.Flush();
    dexCache.Flush();
    txReceiptCache.Flush();

    txCache.Flush();
    ppCache.Flush();
}

void CCacheWrapper::SetDbOpMapLog(CDBOpLogMap *pDbOpLogMap) {
    sysParamCache.SetDbOpLogMap(pDbOpLogMap);
    accountCache.SetDbOpLogMap(pDbOpLogMap);
    assetCache.SetDbOpLogMap(pDbOpLogMap);
    contractCache.SetDbOpLogMap(pDbOpLogMap);
    delegateCache.SetDbOpLogMap(pDbOpLogMap);
    cdpCache.SetDbOpLogMap(pDbOpLogMap);
    dexCache.SetDbOpLogMap(pDbOpLogMap);
    txReceiptCache.SetDbOpLogMap(pDbOpLogMap);
}
