// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cachewrapper.h"
#include "main.h"


std::shared_ptr<CCacheWrapper>CCacheWrapper::NewCopyFrom(CCacheDBManager* pCdMan) {
    auto pNewCopy = make_shared<CCacheWrapper>();
    pNewCopy->CopyFrom(pCdMan);
    return pNewCopy;
}

CCacheWrapper::CCacheWrapper() {}

CCacheWrapper::CCacheWrapper(CSysParamDBCache* pSysParamCacheIn,
                             CBlockDBCache*  pBlockCacheIn,
                             CAccountDBCache* pAccountCacheIn,
                             CAssetDBCache* pAssetCache,
                             CContractDBCache* pContractCacheIn,
                             CDelegateDBCache* pDelegateCacheIn,
                             CCdpDBCache* pCdpCacheIn,
                             CClosedCdpDBCache* pClosedCdpCacheIn,
                             CDexDBCache* pDexCacheIn,
                             CTxReceiptDBCache* pReceiptCacheIn,
                             CTxMemCache* pTxCacheIn,
                             CPricePointMemCache *pPpCacheIn,
                             CSysGovernDBCache *pSysGovernCacheIn) {
    sysParamCache.SetBaseViewPtr(pSysParamCacheIn);
    blockCache.SetBaseViewPtr(pBlockCacheIn);
    accountCache.SetBaseViewPtr(pAccountCacheIn);
    assetCache.SetBaseViewPtr(pAssetCache);
    contractCache.SetBaseViewPtr(pContractCacheIn);
    delegateCache.SetBaseViewPtr(pDelegateCacheIn);
    cdpCache.SetBaseViewPtr(pCdpCacheIn);
    closedCdpCache.SetBaseViewPtr(pClosedCdpCacheIn);
    dexCache.SetBaseViewPtr(pDexCacheIn);
    txReceiptCache.SetBaseViewPtr(pReceiptCacheIn);
    txCache.SetBaseViewPtr(pTxCacheIn);
    ppCache.SetBaseViewPtr(pPpCacheIn);
    sysGovernCache.SetBaseViewPtr(pSysGovernCacheIn);
}

CCacheWrapper::CCacheWrapper(CCacheWrapper *cwIn) {
    sysParamCache.SetBaseViewPtr(&cwIn->sysParamCache);
    blockCache.SetBaseViewPtr(&cwIn->blockCache);
    accountCache.SetBaseViewPtr(&cwIn->accountCache);
    assetCache.SetBaseViewPtr(&cwIn->assetCache);
    contractCache.SetBaseViewPtr(&cwIn->contractCache);
    delegateCache.SetBaseViewPtr(&cwIn->delegateCache);
    cdpCache.SetBaseViewPtr(&cwIn->cdpCache);
    closedCdpCache.SetBaseViewPtr(&cwIn->closedCdpCache);
    dexCache.SetBaseViewPtr(&cwIn->dexCache);
    txReceiptCache.SetBaseViewPtr(&cwIn->txReceiptCache);

    txCache.SetBaseViewPtr(&cwIn->txCache);
    ppCache.SetBaseViewPtr(&cwIn->ppCache);
    sysGovernCache.SetBaseViewPtr(&cwIn->sysGovernCache);
}

CCacheWrapper::CCacheWrapper(CCacheDBManager* pCdMan) {
    sysParamCache.SetBaseViewPtr(pCdMan->pSysParamCache);
    blockCache.SetBaseViewPtr(pCdMan->pBlockCache);
    accountCache.SetBaseViewPtr(pCdMan->pAccountCache);
    assetCache.SetBaseViewPtr(pCdMan->pAssetCache);
    contractCache.SetBaseViewPtr(pCdMan->pContractCache);
    delegateCache.SetBaseViewPtr(pCdMan->pDelegateCache);
    cdpCache.SetBaseViewPtr(pCdMan->pCdpCache);
    closedCdpCache.SetBaseViewPtr(pCdMan->pClosedCdpCache);
    dexCache.SetBaseViewPtr(pCdMan->pDexCache);
    txReceiptCache.SetBaseViewPtr(pCdMan->pReceiptCache);

    txCache.SetBaseViewPtr(pCdMan->pTxCache);
    ppCache.SetBaseViewPtr(pCdMan->pPpCache);
    sysGovernCache.SetBaseViewPtr(pCdMan->pSysGovernCache);
}

void CCacheWrapper::CopyFrom(CCacheDBManager* pCdMan){
    sysParamCache  = *pCdMan->pSysParamCache;
    blockCache     = *pCdMan->pBlockCache;
    accountCache   = *pCdMan->pAccountCache;
    assetCache     = *pCdMan->pAssetCache;
    contractCache  = *pCdMan->pContractCache;
    delegateCache  = *pCdMan->pDelegateCache;
    cdpCache       = *pCdMan->pCdpCache;
    closedCdpCache = *pCdMan->pClosedCdpCache;
    dexCache       = *pCdMan->pDexCache;
    txReceiptCache = *pCdMan->pReceiptCache;

    txCache = *pCdMan->pTxCache;
    ppCache = *pCdMan->pPpCache;
    sysGovernCache = *pCdMan->pSysGovernCache ;
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
    this->closedCdpCache = other.closedCdpCache;
    this->dexCache       = other.dexCache;
    this->txReceiptCache = other.txReceiptCache;
    this->txCache        = other.txCache;
    this->ppCache        = other.ppCache;
    this->sysGovernCache = other.sysGovernCache;

    return *this;
}

void CCacheWrapper::EnableTxUndoLog(const TxID& txid) {
    txUndo.Clear();

    txUndo.SetTxID(txid);
    SetDbOpLogMap(&txUndo.dbOpLogMap);
}

void CCacheWrapper::DisableTxUndoLog() {
    SetDbOpLogMap(nullptr);
}

bool CCacheWrapper::UndoData(CBlockUndo &blockUndo) {
    for (auto it = blockUndo.vtxundo.rbegin(); it != blockUndo.vtxundo.rend(); it++) {
        // TODO: should use foreach(it->dbOpLogMap) to dispatch the DbOpLog to the cache (switch case)
        SetDbOpLogMap(&it->dbOpLogMap);
        bool ret =  sysParamCache.UndoData() &&
                    blockCache.UndoData() &&
                    accountCache.UndoData() &&
                    assetCache.UndoData() &&
                    contractCache.UndoData() &&
                    delegateCache.UndoData() &&
                    cdpCache.UndoData() &&
                    closedCdpCache.UndoData() &&
                    dexCache.UndoData() &&
                    txReceiptCache.UndoData() &&
                    sysGovernCache.UndoData();

        if (!ret) {
            return ERRORMSG("CCacheWrapper::UndoData() : undo data of tx failed! txUndo=%s", txUndo.ToString());
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
    closedCdpCache.Flush();
    dexCache.Flush();
    txReceiptCache.Flush();

    txCache.Flush();
    ppCache.Flush();
    sysGovernCache.Flush();
}

void CCacheWrapper::SetDbOpLogMap(CDBOpLogMap *pDbOpLogMap) {
    sysParamCache.SetDbOpLogMap(pDbOpLogMap);
    blockCache.SetDbOpLogMap(pDbOpLogMap);
    accountCache.SetDbOpLogMap(pDbOpLogMap);
    assetCache.SetDbOpLogMap(pDbOpLogMap);
    contractCache.SetDbOpLogMap(pDbOpLogMap);
    delegateCache.SetDbOpLogMap(pDbOpLogMap);
    cdpCache.SetDbOpLogMap(pDbOpLogMap);
    closedCdpCache.SetDbOpLogMap(pDbOpLogMap);
    dexCache.SetDbOpLogMap(pDbOpLogMap);
    txReceiptCache.SetDbOpLogMap(pDbOpLogMap);
    sysGovernCache.SetDbOpLogMap(pDbOpLogMap) ;
}
