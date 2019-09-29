// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "blockdb.h"
#include "cdpdb.h"
#include "contractdb.h"
#include "delegatedb.h"
#include "dexdb.h"
#include "pricefeeddb.h"
#include "sysparamdb.h"
#include "txdb.h"
#include "txreceiptdb.h"
#include "assetdb.h"

class CCacheDBManager;
class CBlockUndo;

class CCacheWrapper {
public:
    CSysParamDBCache    sysParamCache;
    CBlockDBCache       blockCache;
    CAccountDBCache     accountCache;
    CAssetDBCache       assetCache;
    CContractDBCache    contractCache;
    CDelegateDBCache    delegateCache;
    CCdpDBCache         cdpCache;
    CClosedCdpDBCache   closedCdpCache;
    CDexDBCache         dexCache;
    CTxReceiptDBCache   txReceiptCache;

    CTxMemCache         txCache;
    CPricePointMemCache ppCache;

    CTxUndo             txUndo;
public:
    static std::shared_ptr<CCacheWrapper> NewCopyFrom(CCacheDBManager* pCdMan);
public:
    CCacheWrapper();

    CCacheWrapper(CSysParamDBCache* pSysParamCacheIn,
                  CBlockDBCache*  pBlockCacheIn,
                  CAccountDBCache* pAccountCacheIn,
                  CAssetDBCache* pAssetCache,
                  CContractDBCache* pContractCacheIn,
                  CDelegateDBCache* pDelegateCacheIn,
                  CCdpDBCache* pCdpCacheIn,
                  CClosedCdpDBCache* pClosedCdpCacheIn,
                  CDexDBCache* pDexCacheIn,
                  CTxReceiptDBCache* pReceiptCacheIn,
                  CTxMemCache *pTxCacheIn,
                  CPricePointMemCache *pPpCacheIn);
    CCacheWrapper(CCacheWrapper* cwIn);
    CCacheWrapper(CCacheDBManager* pCdMan);

    CCacheWrapper& operator=(CCacheWrapper& other);

    void CopyFrom(CCacheDBManager* pCdMan);

    void EnableTxUndoLog();
    void DisableTxUndoLog();
    const CTxUndo& GetTxUndo() const { return txUndo; }
    bool UndoData(CBlockUndo &blockUndo);
    void Flush();

private:
    CCacheWrapper(const CCacheWrapper&) = delete;
    CCacheWrapper& operator=(const CCacheWrapper&) = delete;

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMap);
};

#endif //PERSIST_CACHEWRAPPER_H