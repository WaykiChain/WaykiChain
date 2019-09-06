// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"
#include "commons/uint256.h"
#include "main.h"
#include "persistence/txdb.h"
#include "tx/tx.h"

using namespace std;

CTxMemPoolEntry::CTxMemPoolEntry() {
    nTxSize   = 0;
    dPriority = 0.0;

    nTime   = 0;
    height = 0;
}

CTxMemPoolEntry::CTxMemPoolEntry(CBaseTx *pBaseTx, int64_t time, uint32_t height) : nTime(time), height(height) {
    pTx       = pBaseTx->GetNewInstance();
    nFees     = pTx->GetFees();
    nTxSize   = ::GetSerializeSize(*pTx, SER_NETWORK, PROTOCOL_VERSION);
    dPriority = pTx->GetPriority();
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTxMemPoolEntry &other) {
    this->pTx       = other.pTx->GetNewInstance();
    this->nFees     = other.nFees;
    this->nTxSize   = other.nTxSize;
    this->dPriority = other.dPriority;

    this->nTime     = other.nTime;
    this->height   = other.height;
}

CTxMemPool::CTxMemPool() {
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck         = false;
    nTransactionsUpdated = 0;
}

uint32_t CTxMemPool::GetUpdatedTransactionNum() const {
    LOCK(cs);
    return nTransactionsUpdated;
}

void CTxMemPool::AddUpdatedTransactionNum(uint32_t n) {
    LOCK(cs);
    nTransactionsUpdated += n;
}

void CTxMemPool::Remove(CBaseTx *pBaseTx, list<std::shared_ptr<CBaseTx> > &removed, bool fRecursive) {
    // Remove transaction from memory pool
    LOCK(cs);
    uint256 txid = pBaseTx->GetHash();
    if (memPoolTxs.count(txid)) {
        removed.push_front(std::shared_ptr<CBaseTx>(memPoolTxs[txid].GetTransaction()));
        memPoolTxs.erase(txid);
        EraseTransaction(txid);
        nTransactionsUpdated++;
    }
}

bool CTxMemPool::AddUnchecked(const uint256 &txid, const CTxMemPoolEntry &entry, CValidationState &state) {
    // Add to memory pool without checking anything.
    // Used by main.cpp AcceptToMemoryPool(), which DOES
    // all the appropriate checks.
    LOCK(cs);
    {
        if (!CheckTxInMemPool(txid, entry, state))
            return false;

        memPoolTxs.insert(make_pair(txid, entry));
        ++nTransactionsUpdated;
    }
    return true;
}

void CTxMemPool::QueryHash(vector<uint256> &txids) {
    LOCK(cs);

    txids.clear();
    txids.reserve(memPoolTxs.size());
    for (typename map<uint256, CTxMemPoolEntry>::iterator mi = memPoolTxs.begin(); mi != memPoolTxs.end(); ++mi) {
        txids.push_back((*mi).first);
    }
}

bool CTxMemPool::CheckTxInMemPool(const uint256 &txid, const CTxMemPoolEntry &memPoolEntry, CValidationState &state,
                                  bool bExecute) {
    // is it already confirmed in block
    if (cw->txCache.HaveTx(txid))
        return state.Invalid(ERRORMSG("CheckTxInMemPool() : txid: %s has been confirmed", txid.GetHex()), REJECT_INVALID,
                             "tx-duplicate-confirmed");

    // is it within valid height
    static int validHeight = SysCfg().GetTxCacheHeight();
    if (!memPoolEntry.GetTransaction()->IsValidHeight(chainActive.Height(), validHeight)) {
        return state.Invalid(ERRORMSG("CheckTxInMemPool() : txid: %s beyond the scope of valid height", txid.GetHex()),
                             REJECT_INVALID, "tx-invalid-height");
    }

    auto spCW = std::make_shared<CCacheWrapper>(*cw);

    if (bExecute) {
        if (!memPoolEntry.GetTransaction()->ExecuteTx(chainActive.Height(), 0, *spCW, state)) {
            if (SysCfg().IsLogFailures()) {
                pCdMan->pLogCache->SetExecuteFail(chainActive.Height(), memPoolEntry.GetTransaction()->GetHash(),
                                                  state.GetRejectCode(), state.GetRejectReason());
            }
            return false;
        }
    }

    // Need to re-sync all to cache layer except for transaction cache, as it's depend on
    // the global transaction cache to verify whether a transaction(txid) has been confirmed
    // already in block.
    spCW->Flush();

    return true;
}

void CTxMemPool::SetMemPoolCache(CCacheDBManager *pCdManIn) {
    cw.reset(new CCacheWrapper(pCdManIn));
}

void CTxMemPool::ReScanMemPoolTx(CCacheDBManager *pCdManIn) {
    cw.reset(new CCacheWrapper(pCdManIn));

    LOCK(cs);
    CValidationState state;
    for (map<uint256, CTxMemPoolEntry>::iterator iterTx = memPoolTxs.begin(); iterTx != memPoolTxs.end();) {
        if (!CheckTxInMemPool(iterTx->first, iterTx->second, state, true)) {
            uint256 txid = iterTx->first;
            iterTx       = memPoolTxs.erase(iterTx++);
            EraseTransaction(txid);
            continue;
        }
        ++iterTx;
    }
}

void CTxMemPool::Clear() {
    LOCK(cs);

    memPoolTxs.clear();
    cw.reset(new CCacheWrapper(pCdMan));

    ++nTransactionsUpdated;
}

uint64_t CTxMemPool::Size() {
    LOCK(cs);
    return memPoolTxs.size();
}

bool CTxMemPool::Exists(const uint256 txid) {
    LOCK(cs);
    return ((memPoolTxs.count(txid) != 0));
}

std::shared_ptr<CBaseTx> CTxMemPool::Lookup(const uint256 txid) const {
    LOCK(cs);
    typename map<uint256, CTxMemPoolEntry>::const_iterator i = memPoolTxs.find(txid);
    if (i == memPoolTxs.end())
        return std::shared_ptr<CBaseTx>();
    return i->second.GetTransaction();
}