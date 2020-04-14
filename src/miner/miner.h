// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MINER_H
#define COIN_MINER_H

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>

#include "entities/key.h"
#include "commons/uint256.h"
#include "tx/tx.h"

class CBlock;
class CBlockIndex;
class CWallet;
class CBaseTx;
class CAccountDBCache;
class CAccount;

#include <cmath>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// ThreadBlockProducing
//

struct Miner {
    VoteDelegate delegate;
    CAccount account;
    CKey key;
};

struct TxPriority {
    double priority;
    double feePerKb;
    std::shared_ptr<CBaseTx> baseTx;

    TxPriority(const double priorityIn, const double feePerKbIn, const std::shared_ptr<CBaseTx> &baseTxIn)
        : priority(priorityIn), feePerKb(feePerKbIn), baseTx(baseTxIn) {}

    bool operator<(const TxPriority &other) const {
        if (fabs(this->priority - other.priority) <= 1000) {
            if (fabs(this->feePerKb < other.feePerKb) <= 1e-8) {
                return this->baseTx->GetHash() < other.baseTx->GetHash();
            } else {
                return this->feePerKb < other.feePerKb;
            }
        } else {
            return this->priority < other.priority;
        }
    }
};

// mined block info
class MinedBlockInfo {
public:
    int64_t time;             // block time
    int64_t nonce;            // nonce
    int32_t height;           // block height
    uint64_t totalFuelFee;    // the total fuel fee of all transactions in the block
    uint fuelRate;            // block fuel rate
    uint64_t txCount;         // transaction count in block, exclude coinbase
    uint64_t totalBlockSize;  // block size(bytes)
    uint256 hash;             // block hash
    uint256 hashPrevBlock;    // prev block has

public:
    MinedBlockInfo() { SetNull(); }
    void SetNull();
    void Set(const CBlock *pBlock);
};

// get the info of mined blocks. thread safe.
vector<MinedBlockInfo> GetMinedBlocks(uint32_t count);

/** Run the miner threads */
void GenerateProduceBlockThread(bool fGenerate, CWallet *pWallet, int32_t nThreads);

bool VerifyRewardTx(const CBlock *pBlock, CCacheWrapper &cwIn, VoteDelegate &curDelegateOut, uint32_t& totalDelegateNumOut);

/** Check mined block */
bool CheckWork(CBlock *pBlock);

/** Get burn element */
uint32_t GetElementForBurn(CBlockIndex *pIndex);

void GetPriorityTx(CCacheWrapper &cw, vector<TxPriority> &vecPriority, int32_t nFuelRate);

void ShuffleDelegates(const int32_t nCurHeight, const int64_t blockTime,
        VoteDelegateVector &delegates);

bool GetCurrentDelegate(const int64_t currentTime, const int32_t currHeight,
                        const VoteDelegateVector &delegates, VoteDelegate &delegate);


#endif  // COIN_MINER_H
