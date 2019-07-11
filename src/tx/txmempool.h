// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COIN_TXMEMPOOL_H
#define COIN_TXMEMPOOL_H

#include <list>
#include <map>
#include <memory>

#include "persistence/contractdb.h"
#include "sync.h"

using namespace std;

class CAccountDBCache;
class CContractDBCache;
class CValidationState;
class CBaseTx;
class uint256;

/*
 * CTxMemPool stores these:
 */
class CTxMemPoolEntry {
private:
    std::shared_ptr<CBaseTx> pTx;
    int64_t nFees;     // Cached to avoid expensive parent-transaction lookups
    uint32_t nTxSize;  // Cached to avoid recomputing tx size
    double dPriority;  // Cached to avoid recomputing priority

    int64_t nTime;     // Local time when entering the mempool
    uint32_t nHeight;  // Chain height when entering the mempool

public:
    CTxMemPoolEntry(CBaseTx *ptx, int64_t time, uint32_t height);
    CTxMemPoolEntry();
    CTxMemPoolEntry(const CTxMemPoolEntry &other);

    std::shared_ptr<CBaseTx> GetTransaction() const { return pTx; }

    inline int64_t GetFee() const { return nFees; }
    inline uint32_t GetTxSize() const { return nTxSize; }
    inline double GetPriority() const { return dPriority; }

    inline int64_t GetTime() const { return nTime; }
    inline uint32_t GetHeight() const { return nHeight; }
};

/*
 * CTxMemPool stores valid-according-to-the-current-best-chain
 * transactions that may be included in the next block.
 *
 * Transactions are added when they are seen on the network
 * (or created by the local node), but not all transactions seen
 * are added to the pool: if a new transaction double-spends
 * an input of a transaction in the pool, it is dropped,
 * as are non-standard transactions.
 */
class CTxMemPool
{
private:
    bool fSanityCheck; // Normally false, true if -checkmempool or -regtest
    uint32_t nTransactionsUpdated;  //TODO meaning
public:
    mutable CCriticalSection cs;
    map<uint256, CTxMemPoolEntry > memPoolTxs;
    std::shared_ptr<CAccountDBCache> memPoolAccountCache;
    std::shared_ptr<CContractDBCache> memPoolContractCache;

    CTxMemPool();

    void SetSanityCheck(bool _fSanityCheck) { fSanityCheck = _fSanityCheck; }
    bool AddUnchecked(const uint256 &hash, const CTxMemPoolEntry &entry, CValidationState &state);
    void Remove(CBaseTx *pBaseTx, list<std::shared_ptr<CBaseTx> > &removed, bool fRecursive = false);
    void Clear();
    void QueryHash(vector<uint256> &vtxid);
    uint32_t GetUpdatedTransactionNum() const;
    void AddUpdatedTransactionNum(uint32_t n);
    std::shared_ptr<CBaseTx> Lookup(uint256 hash) const;
    void SetAccountCache(CAccountDBCache *pAccountCacheIn);
    void SetContractCache(CContractDBCache *pContractCacheIn);
    bool CheckTxInMemPool(const uint256 &hash, const CTxMemPoolEntry &entry, CValidationState &state, bool bExecute = true);
    void ReScanMemPoolTx(CAccountDBCache *pAccountCacheIn, CContractDBCache *pContractCacheIn);

    unsigned long Size() {
        LOCK(cs);
        return memPoolTxs.size();
    }

    bool Exists(uint256 hash) {
        LOCK(cs);
        return ((memPoolTxs.count(hash) != 0));
    }
};


#endif /* COIN_TXMEMPOOL_H */
