// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"
#include "commons/uint256.h"
#include "main.h"
#include "persistence/txdb.h"
#include "tx/tx.h"
#include "miner/miner.h"

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

    this->nTime  = other.nTime;
    this->height = other.height;
}

CTxMemPool::CTxMemPool() {
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck         = false;
}

void CTxMemPool::Remove(CBaseTx *pBaseTx, list<std::shared_ptr<CBaseTx> > &removed, bool fRecursive) {
    // Remove transaction from memory pool
    LOCK(cs);
    uint256 txid = pBaseTx->GetHash();
    if (memPoolTxs.count(txid)) {
        removed.push_front(std::shared_ptr<CBaseTx>(memPoolTxs[txid].GetTransaction()));
        memPoolTxs.erase(txid);
        EraseTransaction(txid);
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
    // is it within valid height
    static int validHeight = SysCfg().GetTxCacheHeight();
    if (!memPoolEntry.GetTransaction()->IsValidHeight(chainActive.Height(), validHeight))
        return state.Invalid(ERRORMSG("CheckTxInMemPool() : txid: %s beyond the scope of valid height", txid.GetHex()),
                             REJECT_INVALID, "tx-invalid-height");

    // is it already confirmed in block
    if (cw->txCache.HaveTx(txid))
        return state.Invalid(ERRORMSG("CheckTxInMemPool() : txid: %s has been confirmed", txid.GetHex()), REJECT_INVALID,
                             "tx-duplicate-confirmed");

    auto spCW = std::make_shared<CCacheWrapper>(cw.get());

    if (bExecute) {
        CBlockIndex *pTip =  chainActive.Tip();
        uint32_t fuelRate  = GetElementForBurn(pTip);
        uint32_t blockTime = pTip->GetBlockTime();
        uint32_t prevBlockTime = pTip->pprev != nullptr ? pTip->pprev->GetBlockTime() : pTip->GetBlockTime();
        CTxExecuteContext context(chainActive.Height(), 0, fuelRate, blockTime, prevBlockTime, spCW.get(), &state, wasm::transaction_status_type::validating);
        if (!memPoolEntry.GetTransaction()->ExecuteTx(context)) {
            pCdMan->pLogCache->SetExecuteFail(chainActive.Height(), memPoolEntry.GetTransaction()->GetHash(),
                                              state.GetRejectCode(), state.GetRejectReason());
            return false;
        }
    }

    spCW->Flush();

    return true;
}

void CTxMemPool::SetMemPoolCache() {
    cw.reset(new CCacheWrapper(pCdMan));
}

void CTxMemPool::ReScanMemPoolTx() {
    cw.reset(new CCacheWrapper(pCdMan));

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