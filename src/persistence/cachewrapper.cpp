// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cachewrapper.h"
#include "main.h"
#include "logging.h"

////////////////////////////////////////////////////////////////////////////////
// class CCacheWrapper

std::shared_ptr<CCacheWrapper>CCacheWrapper::NewCopyFrom(CCacheDBManager* pCdMan) {
    auto pNewCopy = make_shared<CCacheWrapper>();
    pNewCopy->CopyFrom(pCdMan);
    return pNewCopy;
}

CCacheWrapper::CCacheWrapper() {}

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
    txUtxoCache.SetBaseViewPtr(&cwIn->txUtxoCache);

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
    txUtxoCache.SetBaseViewPtr(pCdMan->pUtxoCache);

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
    txUtxoCache    = *pCdMan->pUtxoCache;

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
    this->txUtxoCache    = other.txUtxoCache;
    this->txCache        = other.txCache;
    this->ppCache        = other.ppCache;
    this->sysGovernCache = other.sysGovernCache;

    return *this;
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
    txUtxoCache.Flush();

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
    txUtxoCache.SetDbOpLogMap(pDbOpLogMap);
    sysGovernCache.SetDbOpLogMap(pDbOpLogMap) ;
}

UndoDataFuncMap CCacheWrapper::GetUndoDataFuncMap() {
    UndoDataFuncMap undoDataFuncMap;
    sysParamCache.RegisterUndoFunc(undoDataFuncMap);
    blockCache.RegisterUndoFunc(undoDataFuncMap);
    accountCache.RegisterUndoFunc(undoDataFuncMap);
    assetCache.RegisterUndoFunc(undoDataFuncMap);
    contractCache.RegisterUndoFunc(undoDataFuncMap);
    delegateCache.RegisterUndoFunc(undoDataFuncMap);
    cdpCache.RegisterUndoFunc(undoDataFuncMap);
    closedCdpCache.RegisterUndoFunc(undoDataFuncMap);
    dexCache.RegisterUndoFunc(undoDataFuncMap);
    txReceiptCache.RegisterUndoFunc(undoDataFuncMap);
    txUtxoCache.RegisterUndoFunc(undoDataFuncMap);
    sysGovernCache.RegisterUndoFunc(undoDataFuncMap);
    return undoDataFuncMap;
}

////////////////////////////////////////////////////////////////////////////////
// class CCacheDBManager

CCacheDBManager::CCacheDBManager(bool fReIndex, bool fMemory) {
    const boost::filesystem::path& dbDir = GetDataDir() / "blocks";
    pSysParamDb     = new CDBAccess(dbDir, DBNameType::SYSPARAM, false, fReIndex);
    pSysParamCache  = new CSysParamDBCache(pSysParamDb);

    pAccountDb      = new CDBAccess(dbDir, DBNameType::ACCOUNT, false, fReIndex);
    pAccountCache   = new CAccountDBCache(pAccountDb);

    pAssetDb        = new CDBAccess(dbDir, DBNameType::ASSET, false, fReIndex);
    pAssetCache     = new CAssetDBCache(pAssetDb);

    pContractDb     = new CDBAccess(dbDir, DBNameType::CONTRACT, false, fReIndex);
    pContractCache  = new CContractDBCache(pContractDb);

    pDelegateDb     = new CDBAccess(dbDir, DBNameType::DELEGATE, false, fReIndex);
    pDelegateCache  = new CDelegateDBCache(pDelegateDb);

    pCdpDb          = new CDBAccess(dbDir, DBNameType::CDP, false, fReIndex);
    pCdpCache       = new CCdpDBCache(pCdpDb);

    pClosedCdpDb    = new CDBAccess(dbDir, DBNameType::CLOSEDCDP, false, fReIndex);
    pClosedCdpCache = new CClosedCdpDBCache(pClosedCdpDb);

    pDexDb          = new CDBAccess(dbDir, DBNameType::DEX, false, fReIndex);
    pDexCache       = new CDexDBCache(pDexDb);

    pBlockIndexDb   = new CBlockIndexDB(false, fReIndex);

    pBlockDb        = new CDBAccess(dbDir, DBNameType::BLOCK, false, fReIndex);
    pBlockCache     = new CBlockDBCache(pBlockDb);

    pLogDb          = new CDBAccess(dbDir, DBNameType::LOG, false, fReIndex);
    pLogCache       = new CLogDBCache(pLogDb);

    pReceiptDb      = new CDBAccess(dbDir, DBNameType::RECEIPT, false, fReIndex);
    pReceiptCache   = new CTxReceiptDBCache(pReceiptDb);

    pUtxoDb         = new CDBAccess(dbDir, DBNameType::UTXO, false, fReIndex);
    pUtxoCache      = new CTxUTXODBCache(pUtxoDb);

    pSysGovernDb    = new CDBAccess(dbDir, DBNameType::SYSGOVERN, false, fReIndex);
    pSysGovernCache = new CSysGovernDBCache(pSysGovernDb);

    // memory-only cache
    pTxCache        = new CTxMemCache();
    pPpCache        = new CPricePointMemCache();
}

CCacheDBManager::~CCacheDBManager() {
    delete pSysParamCache;  pSysParamCache = nullptr;
    delete pAccountCache;   pAccountCache = nullptr;
    delete pAssetCache;     pAssetCache = nullptr;
    delete pContractCache;  pContractCache = nullptr;
    delete pDelegateCache;  pDelegateCache = nullptr;
    delete pCdpCache;       pCdpCache = nullptr;
    delete pClosedCdpCache; pClosedCdpCache = nullptr;
    delete pDexCache;       pDexCache = nullptr;
    delete pBlockCache;     pBlockCache = nullptr;
    delete pLogCache;       pLogCache = nullptr;
    delete pReceiptCache;   pReceiptCache = nullptr;
    delete pSysGovernCache; pSysGovernCache = nullptr;
    delete pUtxoCache;      pUtxoCache = nullptr;

    delete pSysParamDb;     pSysParamDb = nullptr;
    delete pAccountDb;      pAccountDb = nullptr;
    delete pAssetDb;        pAssetDb = nullptr;
    delete pContractDb;     pContractDb = nullptr;
    delete pDelegateDb;     pDelegateDb = nullptr;
    delete pCdpDb;          pCdpDb = nullptr;
    delete pClosedCdpDb;    pClosedCdpDb = nullptr;
    delete pDexDb;          pDexDb = nullptr;
    delete pBlockIndexDb;   pBlockIndexDb = nullptr;
    delete pBlockDb;        pBlockDb = nullptr;
    delete pLogDb;          pLogDb = nullptr;
    delete pReceiptDb;      pReceiptDb = nullptr;
    delete pSysGovernDb;    pSysGovernDb = nullptr;
    delete pUtxoDb;         pUtxoDb = nullptr;

    // memory-only cache
    delete pTxCache;        pTxCache = nullptr;
    delete pPpCache;        pPpCache = nullptr;
}

bool CCacheDBManager::Flush() {
    if (pSysParamCache) pSysParamCache->Flush();

    if (pAccountCache) pAccountCache->Flush();

    if (pAssetCache) pAssetCache->Flush();

    if (pContractCache) pContractCache->Flush();

    if (pDelegateCache) pDelegateCache->Flush();

    if (pCdpCache) pCdpCache->Flush();

    if (pClosedCdpCache) pClosedCdpCache->Flush();

    if (pDexCache) pDexCache->Flush();

    if (pBlockIndexDb) pBlockIndexDb->Flush();

    if (pBlockCache) pBlockCache->Flush();

    if (pLogCache) pLogCache->Flush();

    if (pReceiptCache) pReceiptCache->Flush();

    if (pSysGovernCache) pSysGovernCache->Flush();

    if (pUtxoCache) pUtxoCache->Flush();

    // Memory only cache, not bother to flush.
    // if (pTxCache)
    //     pTxCache->Flush();
    // if (pPpCache)
    //     pPpCache->Flush();

    return true;
}
