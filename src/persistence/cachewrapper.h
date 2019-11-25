// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "assetdb.h"
#include "blockdb.h"
#include "cdpdb.h"
#include "commons/uint256.h"
#include "contractdb.h"
#include "delegatedb.h"
#include "dexdb.h"
#include "pricefeeddb.h"
#include "sysparamdb.h"
#include "txdb.h"
#include "txreceiptdb.h"
#include "blockundo.h"
#include "logdb.h"

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

    bool UndoData(CBlockUndo &blockUndo);
    void Flush();

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMap);
private:
    CCacheWrapper(const CCacheWrapper&) = delete;
    CCacheWrapper& operator=(const CCacheWrapper&) = delete;

};


class CCacheDBManager {
public:
    CDBAccess           *pSysParamDb;
    CSysParamDBCache    *pSysParamCache;

    CDBAccess           *pAccountDb;
    CAccountDBCache     *pAccountCache;

    CDBAccess           *pAssetDb;
    CAssetDBCache       *pAssetCache;

    CDBAccess           *pContractDb;
    CContractDBCache    *pContractCache;

    CDBAccess           *pDelegateDb;
    CDelegateDBCache    *pDelegateCache;

    CDBAccess           *pCdpDb;
    CCdpDBCache         *pCdpCache;

    CDBAccess           *pClosedCdpDb;
    CClosedCdpDBCache   *pClosedCdpCache;

    CDBAccess           *pDexDb;
    CDexDBCache         *pDexCache;

    CBlockIndexDB       *pBlockIndexDb;

    CDBAccess           *pBlockDb;
    CBlockDBCache       *pBlockCache;

    CDBAccess           *pLogDb;
    CLogDBCache         *pLogCache;

    CDBAccess           *pReceiptDb;
    CTxReceiptDBCache   *pReceiptCache;

    CTxMemCache         *pTxCache;
    CPricePointMemCache *pPpCache;

public:
    CCacheDBManager(bool fReIndex, bool fMemory);

    ~CCacheDBManager();

    bool Flush();
};  // CCacheDBManager

class CTxUndoOpLogger {
public:
    CCacheWrapper &cw;
    CBlockUndo &block_undo;
    CTxUndo tx_undo;

    CTxUndoOpLogger(CCacheWrapper& cwIn, const TxID& txidIn, CBlockUndo& blockUndoIn)
        : cw(cwIn), block_undo(blockUndoIn) {

        tx_undo.SetTxID(txidIn);
        cw.SetDbOpLogMap(&tx_undo.dbOpLogMap);
    }
    ~CTxUndoOpLogger() {
        block_undo.vtxundo.push_back(tx_undo);
        cw.SetDbOpLogMap(nullptr);
    }
};

#endif //PERSIST_CACHEWRAPPER_H