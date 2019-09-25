// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cachewrapper.h"
#include "main.h"

CCacheWrapper::CCacheWrapper() {}

CCacheWrapper::CCacheWrapper(CSysParamDBCache* pSysParamCacheIn,
                             CBlockDBCache*  pBlockCacheIn,
                             CAccountDBCache* pAccountCacheIn,
                             CAssetDBCache* pAssetCache,
                             CContractDBCache* pContractCacheIn,
                             CDelegateDBCache* pDelegateCacheIn,
                             CCDPDBCache* pCdpCacheIn,
                             CDexDBCache* pDexCacheIn,
                             CTxReceiptDBCache* pReceiptCacheIn,
                             CTxMemCache* pTxCacheIn,
                             CPricePointMemCache *pPpCacheIn) {
    sysParamCache.SetBaseViewPtr(pSysParamCacheIn);
    blockCache.SetBaseViewPtr(pBlockCacheIn);
    accountCache.SetBaseViewPtr(pAccountCacheIn);
    assetCache.SetBaseViewPtr(pAssetCache);
    contractCache.SetBaseViewPtr(pContractCacheIn);
    delegateCache.SetBaseViewPtr(pDelegateCacheIn);
    cdpCache.SetBaseViewPtr(pCdpCacheIn);
    dexCache.SetBaseViewPtr(pDexCacheIn);
    txReceiptCache.SetBaseViewPtr(pReceiptCacheIn);

    txCache.SetBaseViewPtr(pTxCacheIn);
    ppCache.SetBaseViewPtr(pPpCacheIn);
}

CCacheWrapper::CCacheWrapper(CCacheWrapper& cwIn) {
    sysParamCache.SetBaseViewPtr(&cwIn.sysParamCache);
    blockCache.SetBaseViewPtr(&cwIn.blockCache);
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
    blockCache.SetBaseViewPtr(pCdMan->pBlockCache);
    accountCache.SetBaseViewPtr(pCdMan->pAccountCache);
    assetCache.SetBaseViewPtr(pCdMan->pAssetCache);
    contractCache.SetBaseViewPtr(pCdMan->pContractCache);
    delegateCache.SetBaseViewPtr(pCdMan->pDelegateCache);
    cdpCache.SetBaseViewPtr(pCdMan->pCdpCache);
    dexCache.SetBaseViewPtr(pCdMan->pDexCache);
    txReceiptCache.SetBaseViewPtr(pCdMan->pReceiptCache);

    txCache.SetBaseViewPtr(pCdMan->pTxCache);
    ppCache.SetBaseViewPtr(pCdMan->pPpCache);
}

CCacheWrapper& CCacheWrapper::operator=(CCacheWrapper& other) {
    if (this == &other)
        return *this;

    this->sysParamCache  = other.sysParamCache;
    this->blockCache     = other.blockCache;
    this->accountCache   = other.accountCache;
    this->assetCache     = other.assetCache;
    this->contractCache  = other.contractCache;
    this->delegateCache  = other.delegateCache;
    this->cdpCache       = other.cdpCache;
    this->dexCache       = other.dexCache;
    this->txReceiptCache = other.txReceiptCache;
    this->txCache        = other.txCache;
    this->ppCache        = other.ppCache;

    return *this;
}

void CCacheWrapper::EnableTxUndoLog(const uint256 &txid) {
    txUndo.Clear();
    SetDbOpLogMap(&txUndo.dbOpLogMap);
}

void CCacheWrapper::DisableTxUndoLog() {
    SetDbOpLogMap(nullptr);
}

bool CCacheWrapper::UndoDatas(CBlockUndo &blockUndo) {
    for (auto it = blockUndo.vtxundo.rbegin(); it != blockUndo.vtxundo.rend(); it++) {
        // TODO: should use foreach(it->dbOpLogMap) to dispatch the DbOpLog to the cache (switch case)
        SetDbOpLogMap(&it->dbOpLogMap);
        bool ret =  sysParamCache.UndoDatas() &&
                    blockCache.UndoDatas() &&
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
    blockCache.Flush();
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

void CCacheWrapper::SetDbOpLogMap(CDBOpLogMap *pDbOpLogMap) {
    sysParamCache.SetDbOpLogMap(pDbOpLogMap);
    blockCache.SetDbOpLogMap(pDbOpLogMap);
    accountCache.SetDbOpLogMap(pDbOpLogMap);
    assetCache.SetDbOpLogMap(pDbOpLogMap);
    contractCache.SetDbOpLogMap(pDbOpLogMap);
    delegateCache.SetDbOpLogMap(pDbOpLogMap);
    cdpCache.SetDbOpLogMap(pDbOpLogMap);
    dexCache.SetDbOpLogMap(pDbOpLogMap);
    txReceiptCache.SetDbOpLogMap(pDbOpLogMap);
}
