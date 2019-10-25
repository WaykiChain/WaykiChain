// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COIN_TXMEMPOOL_H
#define COIN_TXMEMPOOL_H

#include "entities/account.h"
#include "persistence/cachewrapper.h"
#include "sync.h"

#include <list>
#include <map>
#include <memory>

using namespace std;

class CValidationState;
class CBaseTx;
class uint256;

/*
 * CTxMemPool stores these:
 */
class CTxMemPoolEntry {
private:
    std::shared_ptr<CBaseTx> pTx;
    std::pair<TokenSymbol, uint64_t> nFees;  // Cached to avoid expensive parent-transaction lookups
    uint32_t nTxSize;                     // Cached to avoid recomputing tx size
    double dPriority;                     // Cached to avoid recomputing priority

    int64_t nTime;     // Local time when entering the mempool
    uint32_t height;  // Chain height when entering the mempool

public:
    CTxMemPoolEntry(CBaseTx *ptx, int64_t time, uint32_t height);
    CTxMemPoolEntry();
    CTxMemPoolEntry(const CTxMemPoolEntry &other);

    std::shared_ptr<CBaseTx> GetTransaction() const { return pTx; }

    inline std::pair<TokenSymbol, uint64_t> GetFees() const { return nFees; }
    inline uint32_t GetTxSize() const { return nTxSize; }
    inline double GetPriority() const { return dPriority; }

    inline int64_t GetTime() const { return nTime; }
    inline uint32_t GetHeight() const { return height; }
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
class CTxMemPool {
public:
    mutable CCriticalSection cs;
    map<uint256, CTxMemPoolEntry > memPoolTxs;
    std::shared_ptr<CCacheWrapper> cw;

public:
    CTxMemPool();

public:
    void SetSanityCheck(bool fSanityCheckIn) { fSanityCheck = fSanityCheckIn; }
    bool AddUnchecked(const uint256 &txid, const CTxMemPoolEntry &entry, CValidationState &state);
    void Remove(CBaseTx *pBaseTx, list<std::shared_ptr<CBaseTx> > &removed, bool fRecursive = false);
    void QueryHash(vector<uint256> &txids);
    bool CheckTxInMemPool(const uint256 &txid, const CTxMemPoolEntry &entry, CValidationState &state,
                          bool bExecute = true);
    void SetMemPoolCache(CCacheDBManager *pCdManIn);
    void ReScanMemPoolTx(CCacheDBManager *pCdManIn);
    void Clear();

    uint64_t Size();
    bool Exists(const uint256 txid);
    std::shared_ptr<CBaseTx> Lookup(const uint256 txid) const;

private:
    bool fSanityCheck; // Normally false, true if -checkmempool or -regtest
};


#endif /* COIN_TXMEMPOOL_H */
