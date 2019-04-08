// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"
#include "txmempool.h"
#include "database.h"
#include "main.h"

using namespace std;

CTxMemPoolEntry::CTxMemPoolEntry() {
     nFee = 0;
     nTxSize = 0;
     nTime = 0;
     dPriority = 0.0;
     nHeight = 0;
}

CTxMemPoolEntry::CTxMemPoolEntry(CBaseTx *pBaseTx, int64_t _nFee, int64_t _nTime, double _dPriority,
        unsigned int _nHeight) :
        nFee(_nFee), nTime(_nTime), dPriority(_dPriority), nHeight(_nHeight) {
    pTx = pBaseTx->GetNewInstance();
    nTxSize = ::GetSerializeSize(*pTx, SER_NETWORK, PROTOCOL_VERSION);
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTxMemPoolEntry& other) {
    this->dPriority = other.dPriority;
    this->nFee = other.nFee;
    this->nTxSize = other.nTxSize;
    this->nTime = other.nTime;
    this->dPriority = other.dPriority;
    this->nHeight = other.nHeight;
    this->pTx = other.pTx->GetNewInstance();
}

double CTxMemPoolEntry::GetPriority(unsigned int currentHeight) const {
    double dResult = 0;
    dResult = nFee / nTxSize;
    return dResult;
}

CTxMemPool::CTxMemPool() {
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck = false;
    nTransactionsUpdated = 0;
}

void CTxMemPool::SetAccountViewDB(CAccountViewCache *pAccountViewCacheIn) {
    pAccountViewCache = std::make_shared<CAccountViewCache>(*pAccountViewCacheIn, false);
}

void CTxMemPool::SetScriptDBViewDB(CScriptDBViewCache *pScriptDBViewCacheIn) {
    pScriptDBViewCache = std::make_shared<CScriptDBViewCache>(*pScriptDBViewCacheIn, false);
}

void CTxMemPool::ReScanMemPoolTx(CAccountViewCache *pAccountViewCacheIn,
                                 CScriptDBViewCache *pScriptDBViewCacheIn) {
    pAccountViewCache.reset(new CAccountViewCache(*pAccountViewCacheIn, true));
    pScriptDBViewCache.reset(new CScriptDBViewCache(*pScriptDBViewCacheIn, true));
    {
        LOCK(cs);
        CValidationState state;
        for (map<uint256, CTxMemPoolEntry>::iterator iterTx = mapTx.begin();
             iterTx != mapTx.end();) {
            if (!CheckTxInMemPool(iterTx->first, iterTx->second, state, true)) {
                uint256 hash = iterTx->first;
                iterTx       = mapTx.erase(iterTx++);
                uiInterface.RemoveTransaction(hash);
                EraseTransaction(hash);
                continue;
            }
            ++iterTx;
        }
    }
}

unsigned int CTxMemPool::GetTransactionsUpdated() const {
    LOCK(cs);
    return nTransactionsUpdated;
}

void CTxMemPool::AddTransactionsUpdated(unsigned int n) {
    LOCK(cs);
    nTransactionsUpdated += n;
}

void CTxMemPool::Remove(CBaseTx *pBaseTx, list<std::shared_ptr<CBaseTx> >& removed, bool fRecursive) {
    // Remove transaction from memory pool
    LOCK(cs);
    uint256 hash = pBaseTx->GetHash();
    if (mapTx.count(hash)) {
        removed.push_front(std::shared_ptr<CBaseTx>(mapTx[hash].GetTx()));
        mapTx.erase(hash);
        uiInterface.RemoveTransaction(hash);
        EraseTransaction(hash);
        nTransactionsUpdated++;
    }
}

bool CTxMemPool::CheckTxInMemPool(const uint256 &hash, const CTxMemPoolEntry &entry,
                                  CValidationState &state, bool bExcute) {
    CTxUndo txundo;
    CTransactionDBCache txCacheTemp(*pTxCacheTip, true);
    CAccountViewCache acctViewTemp(*pAccountViewCache, true);
    CScriptDBViewCache scriptDBViewTemp(*pScriptDBViewCache, true);

    // is it already confirmed in block
    if (uint256() != pTxCacheTip->HasTx(hash))
        return state.Invalid(
            ERRORMSG("CheckTxInMemPool() : tx hash %s has been confirmed", hash.GetHex()),
            REJECT_INVALID, "tx-duplicate-confirmed");
    // is it within valid height
    static int validHeight = SysCfg().GetTxCacheHeight();
    if (!entry.GetTx()->IsValidHeight(chainActive.Tip()->nHeight, validHeight)) {
        return state.Invalid(
            ERRORMSG("CheckTxInMemPool() : txhash=%s beyond the scope of valid height",
                     hash.GetHex()),
            REJECT_INVALID, "tx-invalid-height");
    }

    if (bExcute) {
        if (!entry.GetTx()->ExecuteTx(0, acctViewTemp, state, txundo,
                                      chainActive.Tip()->nHeight + 1, txCacheTemp,
                                      scriptDBViewTemp))
            return false;
    }

    assert(acctViewTemp.Flush() && scriptDBViewTemp.Flush());
    return true;
}

bool CTxMemPool::AddUnchecked(const uint256 &hash, const CTxMemPoolEntry &entry,
                              CValidationState &state) {
    // Add to memory pool without checking anything.
    // Used by main.cpp AcceptToMemoryPool(), which DOES
    // all the appropriate checks.
    LOCK(cs);
    {
        if (!CheckTxInMemPool(hash, entry, state))
            return false;

        mapTx.insert(make_pair(hash, entry));
        nTransactionsUpdated++;
    }
    return true;
}

void CTxMemPool::Clear() {
    LOCK(cs);
    mapTx.clear();
    pAccountViewCache.reset(new CAccountViewCache(*pAccountViewTip, false));
    ++nTransactionsUpdated;
}

void CTxMemPool::QueryHash(vector<uint256>& vtxid) {
    LOCK(cs);
    vtxid.clear();
    vtxid.reserve(mapTx.size());
    for (typename map<uint256, CTxMemPoolEntry>::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
        vtxid.push_back((*mi).first);
}

std::shared_ptr<CBaseTx> CTxMemPool::Lookup(uint256 hash) const {
    LOCK(cs);
    typename map<uint256, CTxMemPoolEntry>::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end())
        return std::shared_ptr<CBaseTx>();
    return i->second.GetTx();
}
