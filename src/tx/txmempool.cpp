// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "commons/uint256.h"
#include "txmempool.h"
#include "main.h"
#include "tx/tx.h"
#include "persistence/txdb.h"

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
    memPoolAccountViewCache = std::make_shared<CAccountViewCache>(*pAccountViewCacheIn);
}

void CTxMemPool::SetScriptDBViewDB(CScriptDBViewCache *pScriptDBViewCacheIn) {
    memPoolScriptDBViewCache = std::make_shared<CScriptDBViewCache>(*pScriptDBViewCacheIn);
}

void CTxMemPool::ReScanMemPoolTx(CAccountViewCache *pAccountViewCacheIn,
                                 CScriptDBViewCache *pScriptDBViewCacheIn) {
    memPoolAccountViewCache.reset(new CAccountViewCache(*pAccountViewCacheIn));
    memPoolScriptDBViewCache.reset(new CScriptDBViewCache(*pScriptDBViewCacheIn));

    {
        LOCK(cs);
        CValidationState state;
        for (map<uint256, CTxMemPoolEntry>::iterator iterTx = memPoolTxs.begin();
             iterTx != memPoolTxs.end();) {
            if (!CheckTxInMemPool(iterTx->first, iterTx->second, state, true)) {
                uint256 hash = iterTx->first;
                iterTx       = memPoolTxs.erase(iterTx++);
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
    if (memPoolTxs.count(hash)) {
        removed.push_front(std::shared_ptr<CBaseTx>(memPoolTxs[hash].GetTx()));
        memPoolTxs.erase(hash);
        EraseTransaction(hash);
        nTransactionsUpdated++;
    }
}

bool CTxMemPool::CheckTxInMemPool(const uint256 &hash, const CTxMemPoolEntry &memPoolEntry,
                                  CValidationState &state, bool bExcute) {
    CTxUndo txundo;
    CTransactionDBCache txCacheTemp(*pTxCacheTip);
    CAccountViewCache acctViewTemp(*memPoolAccountViewCache.get());
    CScriptDBViewCache scriptDBViewTemp(*memPoolScriptDBViewCache.get());

    // is it already confirmed in block
    if (pTxCacheTip->HaveTx(hash))
        return state.Invalid(ERRORMSG("CheckTxInMemPool() : tx hash %s has been confirmed",
                            hash.GetHex()), REJECT_INVALID, "tx-duplicate-confirmed");

    // is it within valid height
    static int validHeight = SysCfg().GetTxCacheHeight();
    if (!memPoolEntry.GetTx()->IsValidHeight(chainActive.Tip()->nHeight, validHeight)) {
        return state.Invalid(ERRORMSG("CheckTxInMemPool() : txhash=%s beyond the scope of valid height",
                            hash.GetHex()), REJECT_INVALID, "tx-invalid-height");
    }

    if (bExcute) {
        if (!memPoolEntry.GetTx()->ExecuteTx(0, acctViewTemp, state, txundo,
                                      chainActive.Tip()->nHeight + 1, txCacheTemp,
                                      scriptDBViewTemp))
            return false;
    }

    acctViewTemp.SetBaseView(memPoolAccountViewCache.get());
    assert(acctViewTemp.Flush());
    scriptDBViewTemp.SetBaseView(memPoolScriptDBViewCache.get());
    assert(scriptDBViewTemp.Flush());

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

        memPoolTxs.insert(make_pair(hash, entry));
        ++nTransactionsUpdated;
    }
    return true;
}

void CTxMemPool::Clear() {
    LOCK(cs);
    memPoolTxs.clear();
    memPoolAccountViewCache.reset(new CAccountViewCache(*pAccountViewTip));
    memPoolScriptDBViewCache.reset(new CScriptDBViewCache(*pScriptDBTip));
    ++nTransactionsUpdated;
}

void CTxMemPool::QueryHash(vector<uint256>& vtxid) {
    LOCK(cs);
    vtxid.clear();
    vtxid.reserve(memPoolTxs.size());
    for (typename map<uint256, CTxMemPoolEntry>::iterator mi = memPoolTxs.begin(); mi != memPoolTxs.end(); ++mi)
        vtxid.push_back((*mi).first);
}

std::shared_ptr<CBaseTx> CTxMemPool::Lookup(uint256 hash) const {
    LOCK(cs);
    typename map<uint256, CTxMemPoolEntry>::const_iterator i = memPoolTxs.find(hash);
    if (i == memPoolTxs.end())
        return std::shared_ptr<CBaseTx>();
    return i->second.GetTx();
}
