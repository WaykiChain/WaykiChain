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
    axcCache.SetBaseViewPtr(&cwIn->axcCache);

    txCache.SetBaseViewPtr(&cwIn->txCache);
    ppCache.SetBaseViewPtr(&cwIn->ppCache);
    sysGovernCache.SetBaseViewPtr(&cwIn->sysGovernCache);
    priceFeedCache.SetBaseViewPtr(&cwIn->priceFeedCache);

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
    axcCache.SetBaseViewPtr(pCdMan->pAxcCache);

    txCache.SetBaseViewPtr(pCdMan->pTxCache);
    ppCache.SetBaseViewPtr(pCdMan->pPpCache);
    sysGovernCache.SetBaseViewPtr(pCdMan->pSysGovernCache);
    priceFeedCache.SetBaseViewPtr(pCdMan->pPriceFeedCache);
}

void CCacheWrapper::CopyFrom(CCacheDBManager* pCdMan){
    sysParamCache   = *pCdMan->pSysParamCache;
    blockCache      = *pCdMan->pBlockCache;
    accountCache    = *pCdMan->pAccountCache;
    assetCache      = *pCdMan->pAssetCache;
    contractCache   = *pCdMan->pContractCache;
    delegateCache   = *pCdMan->pDelegateCache;
    cdpCache        = *pCdMan->pCdpCache;
    closedCdpCache  = *pCdMan->pClosedCdpCache;
    dexCache        = *pCdMan->pDexCache;
    txReceiptCache  = *pCdMan->pReceiptCache;
    txUtxoCache     = *pCdMan->pUtxoCache;
    axcCache        = *pCdMan->pAxcCache;

    txCache         = *pCdMan->pTxCache;
    ppCache         = *pCdMan->pPpCache;
    sysGovernCache = *pCdMan->pSysGovernCache;
    priceFeedCache = *pCdMan->pPriceFeedCache;
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
    this->axcCache       = other.axcCache;
    this->ppCache        = other.ppCache;
    this->sysGovernCache = other.sysGovernCache;
    this->priceFeedCache = other.priceFeedCache;

    return *this;
}

void CCacheWrapper::Flush() {

    auto bm = MAKE_BENCHMARK("CCacheWrapper::Flush()");
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
    axcCache.Flush();

    txCache.Flush();
    ppCache.Flush();
    sysGovernCache.Flush();
    priceFeedCache.Flush();
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
    axcCache.SetDbOpLogMap(pDbOpLogMap);
    sysGovernCache.SetDbOpLogMap(pDbOpLogMap);
    priceFeedCache.SetDbOpLogMap(pDbOpLogMap);
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
    axcCache.RegisterUndoFunc(undoDataFuncMap);
    sysGovernCache.RegisterUndoFunc(undoDataFuncMap);
    priceFeedCache.RegisterUndoFunc(undoDataFuncMap);
    return undoDataFuncMap;
}

////////////////////////////////////////////////////////////////////////////////
// class CCacheDBManager

CCacheDBManager::CCacheDBManager(bool isReindex, bool isMemory): is_reindex(isReindex), is_memory(isMemory) {

    pSysParamDb     = CreateDbAccess(DBNameType::SYSPARAM);
    pSysParamCache  = new CSysParamDBCache(pSysParamDb);

    pAccountDb      = CreateDbAccess(DBNameType::ACCOUNT);
    pAccountCache   = new CAccountDBCache(pAccountDb);

    pAssetDb        = CreateDbAccess(DBNameType::ASSET);
    pAssetCache     = new CAssetDbCache(pAssetDb);

    pContractDb     = CreateDbAccess(DBNameType::CONTRACT);
    pContractCache  = new CContractDBCache(pContractDb);

    pDelegateDb     = CreateDbAccess(DBNameType::DELEGATE);
    pDelegateCache  = new CDelegateDBCache(pDelegateDb);

    pCdpDb          = CreateDbAccess(DBNameType::CDP);
    pCdpCache       = new CCdpDBCache(pCdpDb);

    pClosedCdpDb    = CreateDbAccess(DBNameType::CLOSEDCDP);
    pClosedCdpCache = new CClosedCdpDBCache(pClosedCdpDb);

    pDexDb          = CreateDbAccess(DBNameType::DEX);
    pDexCache       = new CDexDBCache(pDexDb);


    pBlockIndexDb   = new CBlockIndexDB(memory, wipe);

    pBlockDb        = CreateDbAccess(DBNameType::BLOCK);
    pBlockCache     = new CBlockDBCache(pBlockDb);

    pLogDb          = CreateDbAccess(DBNameType::LOG);
    pLogCache       = new CLogDBCache(pLogDb);

    pReceiptDb      = CreateDbAccess(DBNameType::RECEIPT);
    pReceiptCache   = new CTxReceiptDBCache(pReceiptDb);

    pUtxoDb         = CreateDbAccess(DBNameType::UTXO);
    pUtxoCache      = new CTxUTXODBCache(pUtxoDb);

    pAxcDb          = CreateDbAccess(DBNameType::AXC);
    pAxcCache       = new CAxcDBCache(pAxcDb);

    pSysGovernDb    = CreateDbAccess(DBNameType::SYSGOVERN);
    pSysGovernCache = new CSysGovernDBCache(pSysGovernDb);

    pPriceFeedDb    = CreateDbAccess(DBNameType::PRICEFEED);
    pPriceFeedCache = new CPriceFeedCache(pPriceFeedDb);


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
    delete pAxcCache;       pAxcCache = nullptr;
    delete pPriceFeedCache; pPriceFeedCache = nullptr;

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
    delete pAxcDb;          pAxcDb = nullptr;
    delete pPriceFeedDb;    pPriceFeedDb = nullptr;
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

    if (pAxcCache) pAxcCache->Flush();

    if (pPriceFeedCache) pPriceFeedCache->Flush();

    // Memory only cache, not bother to flush.
    // if (pTxCache)
    //     pTxCache->Flush();
    // if (pPpCache)
    //     pPpCache->Flush();

    return true;
}

CDBAccess* CCacheDBManager::CreateDbAccess(DBNameType dbNameTypeIn) {

    const boost::filesystem::path& path = GetDataDir() / "blocks" / ::GetDbName(dbNameTypeIn);
    uint32_t cacheSize = kDBCacheSizeMap.at(dbNameTypeIn);

    return new CDBAccess(dbNameTypeIn, path, cacheSize, memory, wipe);
}