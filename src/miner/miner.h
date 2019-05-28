// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MINER_H
#define COIN_MINER_H

#include <stdint.h>
#include <map>
#include <set>
#include <vector>
#include <memory>

#include "boost/tuple/tuple.hpp"
#include "accounts/key.h"
#include "commons/uint256.h"
#include "tx/tx.h"

class CBlock;
class CBlockIndex;
struct CBlockTemplate;
class CWallet;
class CBaseTx;
class CAccountCache;
class CTransactionCache;
class CContractCache;
class CAccount;

typedef boost::tuple<double, double, std::shared_ptr<CBaseTx> > TxPriority;
class TxPriorityCompare {
    bool byFee;

   public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) {}
    bool operator()(const TxPriority &a, const TxPriority &b) {
        if (byFee) {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        } else {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

// mined block info
class MinedBlockInfo {
public:
    int64_t         nTime;              // block time
    int64_t         nNonce;             // nonce
    int             nHeight;            // block height
    int64_t         nTotalFuels;        // the total fuels of all transactions in the block
    int             nFuelRate;          // block fuel rate
    int64_t         nTotalFees;         // the total fees of all transactions in the block
    uint64_t        nTxCount;           // transaction count in block, exclude coinbase
    uint64_t        nBlockSize;         // block size(bytes)
    uint256         hash;               // block hash
    uint256         hashPrevBlock;      // prev block hash

public:
    void SetNull();
    int64_t GetReward();
};

// get the info of mined blocks. thread safe.
std::vector<MinedBlockInfo> GetMinedBlocks(unsigned int count);

/** Run the miner threads */
void GenerateCoinBlock(bool fGenerate, CWallet *pWallet, int nThreads);
/** Generate a new block */
unique_ptr<CBlockTemplate> CreateNewBlock(CCacheWrapper &cwIn);
/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock *pblock, CBlockIndex *pIndexPrev, unsigned int &nExtraNonce);
/** Do mining precalculation */
void FormatHashBuffers(CBlock *pblock, char *pmidstate, char *pdata, char *phash1);

bool CreateBlockRewardTx(const int64_t currentTime, const CAccount &delegate, CAccountCache &view, CBlock *pBlock);

bool GetDelegatesAcctList(vector<CAccount> &vDelegatesAcctList);
bool GetDelegatesAcctList(vector<CAccount> &vDelegatesAcctList, CAccountCache &accViewIn, CContractCache &scriptCacheIn);

void ShuffleDelegates(const int nCurHeight, vector<CAccount> &vDelegatesList);

bool GetCurrentDelegate(const int64_t currentTime, const vector<CAccount> &vDelegatesAcctList, CAccount &delegateAcct);

bool VerifyPosTx(const CBlock *pBlock, CCacheWrapper &cwIn, bool bNeedRunTx = false);
/** Check mined block */
bool CheckWork(CBlock *pblock, CWallet &wallet);
/** Base sha256 mining transform */
void SHA256Transform(void *pstate, void *pinput, const void *pinit);
/** Get burn element */
int GetElementForBurn(CBlockIndex *pIndex);

void GetPriorityTx(vector<TxPriority> &vecPriority, int nFuelRate);

extern uint256 CreateBlockWithAppointedAddr(CKeyID const &keyID);

#endif  // COIN_MINER_H
