// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "main.h"

#include "logging.h"
#include "entities/id.h"
#include "p2p/addrman.h"
#include "alert.h"
#include "config/chainparams.h"
#include "config/configuration.h"
#include "config/scoin.h"
#include "init.h"
#include "miner/miner.h"
#include "net.h"
#include "tx/merkletx.h"
#include "commons/util/util.h"

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_writer_template.h"
#include "p2p/chainmessage.h"
#include "p2p/processmessage.hpp"
#include "p2p/sendmessage.hpp"
#include "chain/blockdelegates.h"
#include "persistence/blockundo.h"
#include "tx/txserializer.h"

#include <sstream>
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <memory>

using namespace json_spirit;
using namespace std;
using namespace boost;

#if defined(NDEBUG)
#error "Coin cannot be compiled without assertions."
#endif

#define LOG_CATEGORY_BENCH "BENCH"  // log category: BENCH
#define MILLI 0.001                 // conversation rate: milli

//
// Global state
//
CCacheDBManager *pCdMan = nullptr;
CCriticalSection cs_main;
CTxMemPool mempool;
map<uint256, CBlockIndex *> mapBlockIndex;
int32_t nSyncTipHeight = 0;
string publicIp;
map<uint256/* blockhash */, std::shared_ptr<CCacheWrapper>> mapForkCache;
CSignatureCache signatureCache;
CChain chainActive;
CChain chainMostWork;
bool mining;        // could change from time to time due to vote change
CKeyID minerKeyId;  // miner accout keyId
CKeyID nodeKeyId;   // 1st keyId of the node
extern CPBFTMan pbftMan;

map<uint256/* blockhash */, COrphanBlock *> mapOrphanBlocks;
multimap<uint256/* blockhash */, COrphanBlock *> mapOrphanBlocksByPrev;
map<uint256/* blockhash */, std::shared_ptr<CBaseTx> > mapOrphanTransactions;
extern CPBFTContext pbftContext ;
const string strMessageMagic = "Coin Signed Message:\n";


// Internal stuff
namespace {

    void InitializeNode(NodeId nodeid, const CNode *pNode) {
        LOCK(cs_mapNodeState);
        CNodeState &state = mapNodeState.insert(make_pair(nodeid, CNodeState())).first->second;
        state.name        = pNode->addrName;
    }

    int32_t GetHeight() {
        LOCK(cs_main);
        return chainActive.Height();
    }

    void FinalizeNode(NodeId nodeid) {
        LOCK(cs_mapNodeState);
        CNodeState *state = State(nodeid);

        for (const auto &entry : state->vBlocksInFlight)
            mapBlocksInFlight.erase(entry.hash);

        for (const auto &hash : state->vBlocksToDownload)
            mapBlocksToDownload.erase(hash);

        mapNodeState.erase(nodeid);
    }

    struct CBlockIndexWorkComparator {
    bool operator()(CBlockIndex *pa, CBlockIndex *pb) const {

        // First sort by most total work, ...
        if (pa->nChainWork != pb->nChainWork) {
            return (pa->nChainWork < pb->nChainWork) ;
        }

        // ... then by earliest time received, ...
        if (pa->nSequenceId != pb->nSequenceId) {
            return (pa->nSequenceId > pb->nSequenceId) ;
        }

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those all have id 0).

        return pa > pb ;
    }
};

CBlockIndex *pIndexBestInvalid;
// may contain all CBlockIndex*'s that have validness >=BLOCK_VALID_TRANSACTIONS, and must contain those who aren't
// failed
set<CBlockIndex *, CBlockIndexWorkComparator> setBlockIndexValid;  //an ordered set sorted by height

struct COrphanBlockComparator {
    bool operator()(COrphanBlock *pa, COrphanBlock *pb) const{
        if (pa->height > pb->height)
            return false;

        if (pa->height < pb->height)
            return true;

        return false;
    }
};
set<COrphanBlock *, COrphanBlockComparator> setOrphanBlock;  //set of Orphan Blocks

CCriticalSection cs_LastBlockFile;
CBlockFileInfo infoLastBlockFile;
int32_t nLastBlockFile = 0;

// Every received block is assigned a unique and increasing identifier, so we
// know which one to give priority in case of a fork.
CCriticalSection cs_nBlockSequenceId;
// Blocks loaded from disk are assigned id 0, so start the counter at 1.
uint32_t nBlockSequenceId = 1;


}  // namespace

//////////////////////////////////////////////////////////////////////////////
//
// dispatching functions
//

// These functions dispatch to one or all registered wallets

void RegisterWallet(CWalletInterface *pWalletIn) {
    g_signals.SyncTransaction.connect(boost::bind(&CWalletInterface::SyncTransaction, pWalletIn, _1, _2, _3));
    g_signals.EraseTransaction.connect(boost::bind(&CWalletInterface::EraseTransaction, pWalletIn, _1));
    g_signals.SetBestChain.connect(boost::bind(&CWalletInterface::SetBestChain, pWalletIn, _1));
    // g_signals.Inventory.connect(boost::bind(&CWalletInterface::Inventory, pWalletIn, _1));
    g_signals.Broadcast.connect(boost::bind(&CWalletInterface::ResendWalletTransactions, pWalletIn));
}

void UnregisterWallet(CWalletInterface *pWalletIn) {
    g_signals.Broadcast.disconnect(boost::bind(&CWalletInterface::ResendWalletTransactions, pWalletIn));
    // g_signals.Inventory.disconnect(boost::bind(&CWalletInterface::Inventory, pWalletIn, _1));
    g_signals.SetBestChain.disconnect(boost::bind(&CWalletInterface::SetBestChain, pWalletIn, _1));
    g_signals.EraseTransaction.disconnect(boost::bind(&CWalletInterface::EraseTransaction, pWalletIn, _1));
    g_signals.SyncTransaction.disconnect(boost::bind(&CWalletInterface::SyncTransaction, pWalletIn, _1, _2, _3));
}

void UnregisterAllWallets() {
    g_signals.Broadcast.disconnect_all_slots();
    // g_signals.Inventory.disconnect_all_slots();
    g_signals.SetBestChain.disconnect_all_slots();
    g_signals.EraseTransaction.disconnect_all_slots();
    g_signals.SyncTransaction.disconnect_all_slots();
}

void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pBlock) {
    g_signals.SyncTransaction(hash, pBaseTx, pBlock);
}

void EraseTransactionFromWallet(const uint256 &hash) { g_signals.EraseTransaction(hash); }

//////////////////////////////////////////////////////////////////////////////
//
// Registration of network node signals.
//


bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats) {
    LOCK(cs_mapNodeState);
    CNodeState *state = State(nodeid);
    if (state == nullptr)
        return false;

    stats.nMisbehavior = state->nMisbehavior;
    return true;
}

void RegisterNodeSignals(CNodeSignals &nodeSignals) {
    nodeSignals.GetHeight.connect(&GetHeight);
    nodeSignals.ProcessMessages.connect(&ProcessMessages);
    nodeSignals.SendMessages.connect(&SendMessages);
    nodeSignals.InitializeNode.connect(&InitializeNode);
    nodeSignals.FinalizeNode.connect(&FinalizeNode);
}

void UnregisterNodeSignals(CNodeSignals &nodeSignals) {
    nodeSignals.GetHeight.disconnect(&GetHeight);
    nodeSignals.ProcessMessages.disconnect(&ProcessMessages);
    nodeSignals.SendMessages.disconnect(&SendMessages);
    nodeSignals.InitializeNode.disconnect(&InitializeNode);
    nodeSignals.FinalizeNode.disconnect(&FinalizeNode);
}

bool IsStandardTx(CBaseTx *pBaseTx, string &reason) {
    AssertLockHeld(cs_main);
    if (pBaseTx->nVersion > CBaseTx::CURRENT_VERSION || pBaseTx->nVersion < 1) {
        reason = "version";
        return false;
    }

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_SIZE mitigates CPU exhaustion attacks.
    uint32_t sz = ::GetSerializeSize(pBaseTx->GetNewInstance(), SER_NETWORK, CBaseTx::CURRENT_VERSION);
    if (sz >= MAX_STANDARD_TX_SIZE) {
        reason = "tx-size";
        return false;
    }

    return true;
}

bool VerifySignature(const uint256 &sigHash, const std::vector<uint8_t> &signature, const CPubKey &pubKey) {
    if (signatureCache.Get(sigHash, signature, pubKey))
        return true;

    if (!pubKey.Verify(sigHash, signature))
        return false;

    signatureCache.Set(sigHash, signature, pubKey);
    return true;
}

bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx,
                        bool fLimitFree, bool fRejectInsaneFee) {
    AssertLockHeld(cs_main);

    // is it already in the memory pool?
    uint256 hash = pBaseTx->GetHash();
    if (pool.Exists(hash))
        return state.Invalid(ERRORMSG("AcceptToMemoryPool() : txid: %s already in mempool", hash.GetHex()),
                            REJECT_INVALID, "tx-already-in-mempool");

    // is it a miner reward tx or price median tx?
    if (pBaseTx->IsForbidRelay())
        return state.Invalid(ERRORMSG("AcceptToMemoryPool() : forbid tx=%s to accept to memory pool! txid=%s",
            pBaseTx->GetTxTypeName(), hash.GetHex()), REJECT_INVALID, "tx-coinbase-to-mempool");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    string reason;
    if (SysCfg().NetworkID() == MAIN_NET && !IsStandardTx(pBaseTx, reason))
        return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : txid: %s is nonstandard transaction due to %s",
                        hash.GetHex(), reason), REJECT_NONSTANDARD, reason);

    auto spCW = std::make_shared<CCacheWrapper>(mempool.cw.get());

    CBlockIndex *pTip =  chainActive.Tip();
    if (pTip == nullptr) throw runtime_error("AcceptToMemoryPool(), pChainTip is nullptr");
    HeightType newHeight = pTip->height + 1;
    uint32_t fuelRate  = GetElementForBurn(pTip);
    uint32_t blockTime = pTip->GetBlockTime();
    uint32_t prevBlockTime = pTip->pprev != nullptr ? pTip->pprev->GetBlockTime() : pTip->GetBlockTime();

    CTxExecuteContext context(newHeight, 0, fuelRate, blockTime, prevBlockTime, spCW.get(), &state);
    if (!pBaseTx->CheckBaseTx(context) || !pBaseTx->CheckTx(context))
        return ERRORMSG("AcceptToMemoryPool() : CheckBaseTx/CheckTx failed, txid: %s", hash.GetHex());

    CTxMemPoolEntry entry(pBaseTx, GetTime(), newHeight);
    auto nFees = std::get<1>(entry.GetFees());
    auto nSize = entry.GetTxSize();
    // Continuously rate-limit free trx
    // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
    // be annoying or make others' transactions take longer to confirm.
    if (fLimitFree && nFees < MIN_RELAY_TX_FEE) {
        static CCriticalSection csFreeLimiter;
        static double dFreeCount;
        static int64_t nLastTime;
        int64_t nNow = GetTime();

        LOCK(csFreeLimiter);
        // Use an exponentially decaying ~10-second window:
        dFreeCount *= pow(1.0 - 1.0 / 10.0, (double)(nNow - nLastTime));
        nLastTime = nNow;
        // -limitfreerelay unit is thousand-bytes-per-minute
        // At default rate it would take over a month to fill 1GB
        if (dFreeCount >= SysCfg().GetArg("-limitfreerelay", 15) * 10 * 1000 / 60)
            return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : txid: %s is a free transaction, rejected by rate limiter",
                            hash.GetHex()), REJECT_INSUFFICIENTFEE, "insufficient priority");

        LogPrint(BCLog::INFO, "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount + nSize);
        dFreeCount += nSize;
    }

    if (fRejectInsaneFee && nFees > SysCfg().GetMaxFee())
        return ERRORMSG("AcceptToMemoryPool() : txid: %s pay insane fees, %d > %d", hash.GetHex(), nFees, SysCfg().GetMaxFee());

    return pool.AddUnchecked(hash, entry, state);
}

int32_t CMerkleTx::GetDepthInMainChainINTERNAL(CBlockIndex *&pindexRet) const {
    if (blockHash.IsNull() || index == -1)
        return 0;

    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(blockHash);
    if (mi == mapBlockIndex.end())
        return 0;

    CBlockIndex *pIndex = (*mi).second;
    if (!pIndex || !chainActive.Contains(pIndex))
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified) {
        if (CBlock::CheckMerkleBranch(pTx->GetHash(), vMerkleBranch, index) != pIndex->merkleRootHash)
            return 0;

        fMerkleVerified = true;
    }

    pindexRet = pIndex;
    return chainActive.Height() - pIndex->height + 1;
}

int32_t CMerkleTx::GetDepthInMainChain(CBlockIndex *&pindexRet) const {
    AssertLockHeld(cs_main);
    int32_t nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.Exists(pTx->GetHash()))
        return -1;  // Not in chain, not in mempool

    return nResult;
}

int32_t CMerkleTx::GetBlocksToMaturity() const {
    if (!pTx->IsBlockRewardTx())
        return 0;

    return max(0, (BLOCK_REWARD_MATURITY + 1) - GetDepthInMainChain());
}

int32_t GetTxConfirmHeight(const uint256 &hash, CBlockDBCache &blockCache) {
    if (SysCfg().IsTxIndex()) {
        CDiskTxPos diskTxPos;
        if (blockCache.ReadTxIndex(hash, diskTxPos)) {
            CAutoFile file(OpenBlockFile(diskTxPos, true), SER_DISK, CLIENT_VERSION);
            CBlockHeader header;
            try {
                file >> header;
            } catch (std::exception &e) {
                ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
                return -1;
            }
            return header.GetHeight();
        }
    }

    return -1;
}

// Return transaction in tx, and if it was found inside a block, its hash is placed in blockHash
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CBlockDBCache &blockCache,
                    bool bSearchMemPool) {
    {
        LOCK(cs_main);
        {
            if (bSearchMemPool == true) {
                pBaseTx = mempool.Lookup(hash);
                if (pBaseTx.get())
                    return true;
            }
        }

        if (SysCfg().IsTxIndex()) {
            CDiskTxPos diskTxPos;
            if (blockCache.ReadTxIndex(hash, diskTxPos)) {
                CAutoFile file(OpenBlockFile(diskTxPos, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, diskTxPos.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                } catch (std::exception &e) {
                    return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
                }
                return true;
            }
        }
    }
    return false;
}

uint256 GetOrphanRoot(const uint256 &hash) {
    map<uint256, COrphanBlock *>::iterator it = mapOrphanBlocks.find(hash);
    if (it == mapOrphanBlocks.end())
        return hash;

    // Work back to the first block in the orphan chain
    do {
        map<uint256, COrphanBlock *>::iterator it2 = mapOrphanBlocks.find(it->second->prevBlockHash);
        if (it2 == mapOrphanBlocks.end())
            return it->first;
        it = it2;
    } while (true);
}

// Remove a random orphan block (which does not have any dependent orphans).
bool static PruneOrphanBlocks(int32_t height) {
    if (mapOrphanBlocksByPrev.size() <= MAX_ORPHAN_BLOCKS) {
        return true;
    }

    COrphanBlock *pOrphanBlock = *setOrphanBlock.rbegin();
    if (pOrphanBlock->height <= height) {
        return false;
    }
    uint256 hash     = pOrphanBlock->blockHash;
    uint256 prevHash = pOrphanBlock->prevBlockHash;
    setOrphanBlock.erase(pOrphanBlock);
    multimap<uint256, COrphanBlock *>::iterator beg = mapOrphanBlocksByPrev.lower_bound(prevHash);
    multimap<uint256, COrphanBlock *>::iterator end = mapOrphanBlocksByPrev.upper_bound(prevHash);
    while (beg != end) {
        if (beg->second->blockHash == hash) {
            mapOrphanBlocksByPrev.erase(beg);
            break;
        }
        ++beg;
    }
    mapOrphanBlocks.erase(hash);
    delete pOrphanBlock;
    return true;
}

arith_uint256 GetBlockProof(const CBlockIndex &block) {
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

bool fLargeWorkForkFound         = false;
bool fLargeWorkInvalidChainFound = false;
CBlockIndex *pIndexBestForkTip   = nullptr;
CBlockIndex *pIndexBestForkBase  = nullptr;

void CheckForkWarningConditions() {
    AssertLockHeld(cs_main);
    // Before we get past initial download, we cannot reliably alert about forks
    if (IsInitialBlockDownload())
        return;

    // If our best fork is no longer within 72 blocks (+/- 12 hours if no one mines it)
    // of our head, drop it
    if (pIndexBestForkTip && chainActive.Height() - pIndexBestForkTip->height >= 72)
        pIndexBestForkTip = nullptr;

    if (pIndexBestForkTip ||
        (pIndexBestInvalid &&
         pIndexBestInvalid->nChainWork > chainActive.Tip()->nChainWork + (GetBlockProof(*chainActive.Tip()) * 6))) {
        if (!fLargeWorkForkFound && pIndexBestForkBase) {
            string strCmd = SysCfg().GetArg("-alertnotify", "");
            if (!strCmd.empty()) {
                string warning = string("'Warning: Large-work fork detected, forking after block ") +
                                 pIndexBestForkBase->pBlockHash->ToString() + string("'");
                boost::replace_all(strCmd, "%s", warning);
                boost::thread t(runCommand, strCmd);  // thread runs free
            }
        }
        if (pIndexBestForkTip && pIndexBestForkBase) {
            LogPrint(BCLog::INFO,
                     "CheckForkWarningConditions: Warning: Large valid fork found\n"
                     "  forking from height %d (%s)\n"
                     "  lasting to   height %d (%s)\n",
                     pIndexBestForkBase->height, pIndexBestForkBase->pBlockHash->ToString(),
                     pIndexBestForkTip->height, pIndexBestForkTip->pBlockHash->ToString());

            fLargeWorkForkFound = true;
        } else {
            LogPrint(BCLog::INFO,
                     "CheckForkWarningConditions: Warning: Found invalid chain at least ~6 blocks longer than our best "
                     "chain.\nChain state database corruption likely.\n");

            fLargeWorkInvalidChainFound = true;
        }
    } else {
        fLargeWorkForkFound         = false;
        fLargeWorkInvalidChainFound = false;
    }
}

void CheckForkWarningConditionsOnNewFork(CBlockIndex *pindexNewForkTip) {
    AssertLockHeld(cs_main);
    // If we are on a fork that is sufficiently large, set a warning flag
    CBlockIndex *pfork   = pindexNewForkTip;
    CBlockIndex *plonger = chainActive.Tip();
    while (pfork && pfork != plonger) {
        while (plonger && plonger->height > pfork->height)
            plonger = plonger->pprev;
        if (pfork == plonger)
            break;
        pfork = pfork->pprev;
    }

    // We define a condition which we should warn the user about as a fork of at least 7 blocks
    // who's tip is within 72 blocks (+/- 12 hours if no one mines it) of ours
    // We use 7 blocks rather arbitrarily as it represents just under 10% of sustained network
    // hash rate operating on the fork.
    // or a chain that is entirely longer than ours and invalid (note that this should be detected by both)
    // We define it this way because it allows us to only store the highest fork tip (+ base) which meets
    // the 7-block condition and from this always have the most-likely-to-cause-warning fork
    if (pfork &&
        (!pIndexBestForkTip || (pIndexBestForkTip && pindexNewForkTip->height > pIndexBestForkTip->height)) &&
        pindexNewForkTip->nChainWork - pfork->nChainWork > (GetBlockProof(*pfork) * 7) &&
        chainActive.Height() - pindexNewForkTip->height < 72) {
        pIndexBestForkTip  = pindexNewForkTip;
        pIndexBestForkBase = pfork;
    }

    CheckForkWarningConditions();
}

void Misbehaving(NodeId pNode, int32_t howmuch) {
    if (howmuch == 0)
        return;

    LOCK(cs_mapNodeState);
    CNodeState *state = State(pNode);
    if (state == nullptr)
        return;

    state->nMisbehavior += howmuch;
    if (state->nMisbehavior >= SysCfg().GetArg("-banscore", 100)) {
        LogPrint(BCLog::INFO, "Misbehaving: %s (%d -> %d) BAN THRESHOLD EXCEEDED\n", state->name,
                 state->nMisbehavior - howmuch, state->nMisbehavior);
        state->fShouldBan = true;
    } else {
        LogPrint(BCLog::INFO, "Misbehaving: %s (%d -> %d)\n", state->name, state->nMisbehavior - howmuch,
                 state->nMisbehavior);
    }
}

void static InvalidChainFound(CBlockIndex *pIndexNew) {
    if (!pIndexBestInvalid || pIndexNew->nChainWork > pIndexBestInvalid->nChainWork) {
        pIndexBestInvalid = pIndexNew;
        // The current code doesn't actually read the BestInvalidWork entry in
        // the block database anymore, as it is derived from the flags in block
        // index entry. We only write it for backward compatibility.
        // TODO: need to remove the indexBestInvalid
        //pCdMan->pBlockCache->WriteBestInvalidWork(ArithToUint256(pIndexBestInvalid->nChainWork));
    }
    LogPrint(BCLog::INFO, "InvalidChainFound: invalid block=%s  height=%d  log2_work=%.8g  date=%s\n",
             pIndexNew->GetBlockHash().ToString(), pIndexNew->height,
             log(pIndexNew->nChainWork.getdouble()) / log(2.0),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pIndexNew->GetBlockTime()));
    LogPrint(BCLog::INFO, "InvalidChainFound:  current best=%s  height=%d  log2_work=%.8g  date=%s\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
             log(chainActive.Tip()->nChainWork.getdouble()) / log(2.0),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()));
    CheckForkWarningConditions();
}

void static InvalidBlockFound(CBlockIndex *pIndex, const CValidationState &state) {
    int32_t nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        LOCK(cs_mapNodeState);
        map<uint256, NodeId>::iterator it = mapBlockSource.find(pIndex->GetBlockHash());
        if (it != mapBlockSource.end() && State(it->second)) {
            CBlockReject reject = {state.GetRejectCode(), state.GetRejectReason(), pIndex->GetBlockHash()};
            State(it->second)->rejects.push_back(reject);
            if (nDoS > 0) {
                LogPrint(BCLog::INFO, "Misebehaving: found invalid block, hash:%s, Misbehavior add %d\n", it->first.GetHex(),
                         nDoS);
                Misbehaving(it->second, nDoS);
            }
        }
    }

    if (!state.CorruptionPossible()) {
        pIndex->nStatus |= BLOCK_FAILED_VALID;
        pCdMan->pBlockIndexDb->WriteBlockIndex(CDiskBlockIndex(pIndex));
        setBlockIndexValid.erase(pIndex);
        InvalidChainFound(pIndex);
    }
}

bool InvalidateBlock(CValidationState &state, CBlockIndex *pIndex) {
    AssertLockHeld(cs_main);

    // Mark the block itself as invalid.
    pIndex->nStatus |= BLOCK_FAILED_VALID;
    pCdMan->pBlockIndexDb->WriteBlockIndex(CDiskBlockIndex(pIndex));
    setBlockIndexValid.erase(pIndex);

    LogPrint(BCLog::INFO, "Invalidate block[%d]: %s BLOCK_FAILED_VALID\n", pIndex->height,
             pIndex->GetBlockHash().ToString());

    while (chainActive.Contains(pIndex)) {
        CBlockIndex *pindexWalk = chainActive.Tip();
        pindexWalk->nStatus |= BLOCK_FAILED_CHILD;
        pCdMan->pBlockIndexDb->WriteBlockIndex(CDiskBlockIndex(pindexWalk));
        setBlockIndexValid.erase(pindexWalk);

        LogPrint(BCLog::INFO, "Invalidate block[%d]: %s BLOCK_FAILED_CHILD\n", pindexWalk->height,
                 pindexWalk->GetBlockHash().ToString());

        // ActivateBestChain considers blocks already in chainActive
        // unconditionally valid already, so force disconnect away from it.
        if (!DisconnectBlockFromTip(state)) {
            return false;
        }
    }

    InvalidChainFound(pIndex);
    return true;
}

bool ReconsiderBlock(CValidationState &state, CBlockIndex *pIndex) {
    AssertLockHeld(cs_main);

    // Remove the invalidity flag from this block and all its descendants.
    map<uint256, CBlockIndex *>::const_iterator it = mapBlockIndex.begin();
    int32_t height                                    = pIndex->height;
    while (it != mapBlockIndex.end()) {
        if (it->second->nStatus & BLOCK_FAILED_MASK && it->second->GetAncestor(height) == pIndex) {
            it->second->nStatus &= ~BLOCK_FAILED_MASK;
            pCdMan->pBlockIndexDb->WriteBlockIndex(CDiskBlockIndex(it->second));
            setBlockIndexValid.insert(it->second);
            if (it->second == pIndexBestInvalid) {
                // Reset invalid block marker if it was pointing to one of those.
                pIndexBestInvalid = nullptr;
            }
        }
        it++;
    }

    // Remove the invalidity flag from all ancestors too.
    while (pIndex != nullptr) {
        if (pIndex->nStatus & BLOCK_FAILED_MASK) {
            pIndex->nStatus &= ~BLOCK_FAILED_MASK;
            setBlockIndexValid.insert(pIndex);
            pCdMan->pBlockIndexDb->WriteBlockIndex(CDiskBlockIndex(pIndex));
        }
        pIndex = pIndex->pprev;
    }

    return true;
}

void UpdateTime(CBlockHeader &block, const CBlockIndex *pIndexPrev) {
    block.SetTime(max(pIndexPrev->GetMedianTimePast() + 1, GetAdjustedTime()));
}

bool DisconnectBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool *pfClean) {
    assert(pIndex->GetBlockHash() == cw.blockCache.GetBestBlockHash());

    if (pfClean)
        *pfClean = false;

    bool fClean = true;

    CBlockUndo blockUndo;
    CDiskBlockPos pos = pIndex->GetUndoPos();
    if (pos.IsNull())
        return ERRORMSG("DisconnectBlock() : no undo data available");

    if (!blockUndo.ReadFromDisk(pos, pIndex->pprev->GetBlockHash()))
        return ERRORMSG("DisconnectBlock() : failure reading undo data");

    if ((blockUndo.vtxundo.size() != block.vptx.size()) && (blockUndo.vtxundo.size() != (block.vptx.size() + 1)))
        return ERRORMSG("DisconnectBlock() : block and undo data inconsistent");
    CBlockUndoExecutor undoExecutor(cw, blockUndo);
    if (!undoExecutor.Execute()) {
        return ERRORMSG("DisconnectBlock() : Undo all data in block failed");
    }

    // Set previous block as the best block
    cw.blockCache.SetBestBlock(pIndex->pprev->GetBlockHash());

    // Delete the disconnected block's transactions from transaction memory cache.
    if (!cw.txCache.RemoveBlockTx(block)) {
        return state.Abort(_("DisconnectBlock() : failed to delete block from transaction memory cache"));
    }

    // Load transactions into transaction memory cache.
    if (pIndex->height > SysCfg().GetTxCacheHeight()) {
        CBlockIndex *pReLoadBlockIndex = pIndex;
        int32_t nCacheHeight           = SysCfg().GetTxCacheHeight();
        while (pReLoadBlockIndex && nCacheHeight-- > 0) {
            pReLoadBlockIndex = pReLoadBlockIndex->pprev;
        }

        CBlock reLoadblock;
        if (!ReadBlockFromDisk(pReLoadBlockIndex, reLoadblock)) {
            return state.Abort(_("DisconnectBlock() : failed to read block"));
        }

        if (!cw.txCache.AddBlockTx(reLoadblock)) {
            return state.Abort(_("DisconnectBlock() : failed to add block into transaction memory cache"));
        }
    }

    // undo block prices of price point memory cache.
    if (!cw.ppCache.UndoBlock(cw.sysParamCache, pIndex))
        return state.Abort(_("DisconnectBlock() : undo block prices of memory cache"));

    if (pfClean) {
        *pfClean = fClean;
        return true;
    } else {
        return fClean;
    }
}

void static FlushBlockFile(bool fFinalize = false) {
    LOCK(cs_LastBlockFile);

    CDiskBlockPos posOld(nLastBlockFile, 0);

    FILE *fileOld = OpenBlockFile(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, infoLastBlockFile.nSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }

    fileOld = OpenUndoFile(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, infoLastBlockFile.nUndoSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }
}

static bool FindUndoPos(CValidationState &state, int32_t nFile, CDiskBlockPos &pos, uint32_t nAddSize) {
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    uint32_t nNewSize;
    if (nFile == nLastBlockFile) {
        pos.nPos = infoLastBlockFile.nUndoSize;
        nNewSize = (infoLastBlockFile.nUndoSize += nAddSize);
        if (!pCdMan->pBlockIndexDb->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
            return state.Abort(_("Failed to write block info"));
    } else {
        CBlockFileInfo info;
        if (!pCdMan->pBlockIndexDb->ReadBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to read block info"));
        pos.nPos = info.nUndoSize;
        nNewSize = (info.nUndoSize += nAddSize);
        if (!pCdMan->pBlockIndexDb->WriteBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to write block info"));
    }

    uint32_t nOldChunks = (pos.nPos + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    uint32_t nNewChunks = (nNewSize + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks) {
        if (CheckDiskSpace(nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos)) {
            FILE *file = OpenUndoFile(pos);
            if (file) {
                LogPrint(BCLog::INFO, "Pre-allocating up to position 0x%x in rev%05u.dat\n", nNewChunks * UNDOFILE_CHUNK_SIZE, pos.nFile);
                AllocateFileRange(file, pos.nPos, nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos);
                fclose(file);
            }
        } else
            return state.Error("out of disk space");
    }

    return true;
}

static bool GenNativeAsset(CCacheWrapper& cw) {
    //save native asset
    CAsset wicc(SYMB::WICC,SYMB::WICC,AssetType::NIA, 15, CNullID(), INITIAL_BASE_COIN_AMOUNT * COIN, true);
    CAsset wusd(SYMB::WUSD,SYMB::WUSD,AssetType::MPA, 6,  CNullID(), 0, false);
    CAsset wgrt(SYMB::WGRT,SYMB::WGRT,AssetType::NIA, 13, CNullID(), FUND_COIN_GENESIS_TOTAL_RELEASE_AMOUNT * COIN, false);
    return cw.assetCache.SetAsset(wicc)
            && cw.assetCache.SetAsset(wusd)
            && cw.assetCache.SetAsset(wgrt);

}

static bool GenDefaultFeedCoinPairs(CCacheWrapper& cw) {

    return cw.priceFeedCache.AddFeedCoinPair(PriceCoinPair(SYMB::WICC,SYMB::USD)) &&
            cw.priceFeedCache.AddFeedCoinPair(PriceCoinPair(SYMB::WGRT,SYMB::USD));

}
static bool ProcessGenesisBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state) {

    cw.blockCache.SetBestBlock(pIndex->GetBlockHash());
    for (uint32_t i = 1; i < block.vptx.size(); i++) {
        if (block.vptx[i]->nTxType == BLOCK_REWARD_TX) {
            assert(i <= 1);
            CBlockRewardTx *pRewardTx = (CBlockRewardTx *)block.vptx[i].get();
            CAccount account;
            CRegID regId(pIndex->height, i);
            CPubKey pubKey       = pRewardTx->txUid.get<CPubKey>();
            CKeyID keyId         = pubKey.GetKeyId();
            account.keyid        = keyId;
            account.owner_pubkey = pubKey;
            account.regid        = regId;

            account.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, pRewardTx->reward_fees,
                                   ReceiptCode::BLOCK_REWARD_TO_MINER, pRewardTx->receipts);


            if (!cw.txReceiptCache.SetTxReceipts(pRewardTx->GetHash(), pRewardTx->receipts))
                return state.DoS(100, ERRORMSG("ConnectBlock() ::ProcessGenesisBlock, set genesis block receipts failed!"),
                                 REJECT_INVALID, "set-tx-receipt-failed");

            assert( cw.accountCache.SaveAccount(account) );
        } else if (block.vptx[i]->nTxType == DELEGATE_VOTE_TX) {

            CDelegateVoteTx *pDelegateTx = (CDelegateVoteTx *)block.vptx[i].get();
            assert(pDelegateTx->txUid.is<CRegID>());  // Vote Tx must use RegId

            CAccount voterAcct;
            assert(cw.accountCache.GetAccount(pDelegateTx->txUid, voterAcct));
            CUserID uid(pDelegateTx->txUid);
            uint64_t maxVotes = 0;
            vector<CCandidateReceivedVote> candidateVotes;
            int32_t j = i;
            for (const auto &vote : pDelegateTx->candidateVotes) {
                assert(vote.GetCandidateVoteType() == ADD_BCOIN);  // it has to be ADD in GensisBlock
                if (vote.GetVotedBcoins() > maxVotes) {
                    maxVotes = vote.GetVotedBcoins();
                }

                CUserID votedUid = vote.GetCandidateUid();

                if (uid == votedUid) {  // vote for self
                    voterAcct.received_votes = vote.GetVotedBcoins();
                    assert(cw.delegateCache.SetDelegateVotes(voterAcct.regid, voterAcct.received_votes));
                } else {  // vote for others
                    CAccount votedAcct;
                    assert(!cw.accountCache.GetAccount(votedUid, votedAcct));

                    CRegID votedRegId(pIndex->height, j++);  // generate RegId in genesis block
                    votedAcct.SetRegId(votedRegId);
                    votedAcct.received_votes = vote.GetVotedBcoins();

                    if (votedUid.is<CPubKey>()) {
                        votedAcct.owner_pubkey = votedUid.get<CPubKey>();
                        votedAcct.keyid  = votedAcct.owner_pubkey.GetKeyId();
                    }

                    assert(cw.accountCache.SaveAccount(votedAcct));
                    assert(cw.delegateCache.SetDelegateVotes(votedAcct.regid, votedAcct.received_votes));
                }

                candidateVotes.push_back( CCandidateReceivedVote(vote) );
                sort(candidateVotes.begin(), candidateVotes.end(),
                     [](const CCandidateReceivedVote &vote1, const CCandidateReceivedVote &vote2) {
                         return vote1.GetVotedBcoins() > vote2.GetVotedBcoins();
                     });
            }

            assert( voterAcct.GetToken(SYMB::WICC).free_amount >= maxVotes );

            voterAcct.OperateBalance(SYMB::WICC, BalanceOpType::VOTE, maxVotes,
                                    ReceiptCode::DELEGATE_ADD_VOTE, pDelegateTx->receipts);

            if (!cw.txReceiptCache.SetTxReceipts(pDelegateTx->GetHash(), pDelegateTx->receipts))
                return state.DoS(100, ERRORMSG("ConnectBlock() ::ProcessGenesisBlock, set genesis block receipts failed!"),
                                 REJECT_INVALID, "set-tx-receipt-failed");

            cw.accountCache.SaveAccount(voterAcct);
            assert( cw.delegateCache.SetCandidateVotes(pDelegateTx->txUid.get<CRegID>(), candidateVotes) );
        }
    }

    if (!cw.delegateCache.SetLastVoteHeight(block.GetHeight())) {
        return state.DoS(100, ERRORMSG("%s(), save last vote height failed! block=%d:%s",
            __FUNCTION__, block.GetHeight(), block.GetHash().ToString()),
            REJECT_INVALID, "save-last-vote-height-failed");
    }

    if (!chain::ProcessBlockDelegates(block, cw, state)) {
        return state.DoS(100, ERRORMSG("ConnectBlock() : process block delegates failed! block=%d:%s",
            block.GetHeight(), block.GetHash().ToString()),
            REJECT_INVALID, "process-block-delegates-failed");
    }

    if(!GenNativeAsset(cw)) {
        return state.DoS(100, ERRORMSG("GenesisBlock::ConnectBlock() : gen WICC, WUSD, WGRT asset error"),
                         REJECT_INVALID, "gen-native-asset-failed");
    }

    if(!GenDefaultFeedCoinPairs(cw)) {
        return state.DoS(100, ERRORMSG("GenesisBlock::ConnectBlock() : gen WICC-WUSD, WGRT-WUSD price coin pair error"),
                         REJECT_INVALID, "gen-default-pricecoinpair-failed");
    }


    return true;
}

bool SaveTxIndex(const uint256 &txid, CCacheWrapper &cw, CValidationState &state, const CDiskTxPos &diskTxPos) {
    if (SysCfg().IsTxIndex()) {
        if (!cw.blockCache.SetTxIndex(txid, diskTxPos))
            return state.Abort(_("Failed to write transaction index"));
    }
    return true;
}

// compute vote staking interest && revoke votes
static bool ComputeVoteStakingInterestAndRevokeVotes(const int32_t currHeight, const uint32_t currBlockTime,
                                                    CCacheWrapper &cw, CValidationState &state) {
    // acquire votes list
    map<CRegIDKey, vector<CCandidateReceivedVote>> regId2ReceivedVotes;
    if (!cw.delegateCache.GetVoterList(regId2ReceivedVotes)) {
        return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : failed to get vote list"),
                         REJECT_INVALID, "bad-get-vote-list");
    }

    // revoke votes if necessary
    map<CRegID, vector<CCandidateVote>> regId2CandidateVotes;
    for (auto &item : regId2ReceivedVotes) {
        const CRegID &regId = item.first.regid;
        auto &candidateReceivedVotes = item.second;
        vector<CCandidateVote> candidateVotes;
        assert(!candidateReceivedVotes.empty());
        // If the voter only votes to one candidate, not bother to revoke votes.
        if (candidateReceivedVotes.size() == 1) {
            regId2CandidateVotes.emplace(regId, candidateVotes /* empty */);
            continue;
        }

        // If the voter votes to more than one candidates, need to revoke votes from the second
        // candidates.
        auto it = candidateReceivedVotes.begin();
        ++it;  // skip the first item
        for (; it < candidateReceivedVotes.end(); ++it) {
            const auto &uid  = it->GetCandidateUid();
            const auto votes = it->GetVotedBcoins();
            candidateVotes.emplace_back(VoteType::MINUS_BCOIN, uid, votes);  // revoke votes
        }

        regId2CandidateVotes.emplace(regId, candidateVotes);
    }

    // compute vote staking interest
    for (const auto &item : regId2CandidateVotes) {
        const auto &regId          = item.first;
        const auto &candidateVotes = item.second;

        vector<CCandidateReceivedVote> candidateVotesInOut;
        cw.delegateCache.GetCandidateVotes(regId, candidateVotesInOut);
        CAccount account;
        cw.accountCache.GetAccount(regId, account);
        vector<CReceipt> receipts;
        if (!account.ProcessCandidateVotes(candidateVotes, candidateVotesInOut, currHeight, currBlockTime,
                                           cw.accountCache, receipts)) {
            return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : operate candidate votes failed, regId=%s",
                            regId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-candidate-votes-failed");
        }
        if (!cw.delegateCache.SetCandidateVotes(regId, candidateVotesInOut)) {
            return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : write candidate votes failed, regId=%s",
                            regId.ToString()), OPERATE_CANDIDATE_VOTES_FAIL, "write-candidate-votes-failed");
        }

        if (!cw.accountCache.SaveAccount(account)) {
            return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : save account id %s info error",
                            account.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
        }

        for (const auto &vote : candidateVotes) {
            CAccount delegate;
            const CUserID &delegateUId = vote.GetCandidateUid();
            if (!cw.accountCache.GetAccount(delegateUId, delegate)) {
                return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : read KeyId(%s) account info error",
                                delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
            }
            uint64_t oldVotes = delegate.received_votes;
            if (!delegate.StakeVoteBcoins(VoteType(vote.GetCandidateVoteType()), vote.GetVotedBcoins())) {
                return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : operate delegate address %s vote fund error",
                                delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-vote-error");
            }

            // Votes: set the new value and erase the old value
            if (!cw.delegateCache.SetDelegateVotes(delegate.regid, delegate.received_votes)) {
                return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : save account id %s vote info error",
                                delegate.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-delegatedb");
            }

            if (!cw.delegateCache.EraseDelegateVotes(delegate.regid, oldVotes)) {
                return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : erase account id %s vote info error",
                                delegate.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-delegatedb");
            }

            if (!cw.accountCache.SaveAccount(delegate)) {
                return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : save account id %s info error",
                                account.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
            }
        }
    }

    if (!cw.delegateCache.SetLastVoteHeight(currHeight)) {
        return state.DoS(100, ERRORMSG("%s(), save last vote height error", __FUNCTION__),
            UPDATE_ACCOUNT_FAIL, "bad-save-last-vote-height");
    }

    return true;
}

bool ConnectBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool fJustCheck) {
    AssertLockHeld(cs_main);

    bool isGensisBlock = (block.GetHeight() == 0) && (block.GetHash() == SysCfg().GetGenesisBlockHash());

    // Check it again in case a previous version let a bad block in
    if (!isGensisBlock && !CheckBlock(block, state, cw, !fJustCheck, !fJustCheck))
        return state.DoS(100, ERRORMSG("ConnectBlock() : check block error"), REJECT_INVALID, "check-block-error");

    if (!fJustCheck) {
        // Verify that the cache's current state corresponds to the previous block
        uint256 hashPrevBlock = pIndex->pprev == nullptr ? uint256() : pIndex->pprev->GetBlockHash();
        if (hashPrevBlock != cw.blockCache.GetBestBlockHash()) {
            LogPrint(BCLog::INFO, "hashPrevBlock=%s, bestblock=%s\n", hashPrevBlock.GetHex(),
                     cw.blockCache.GetBestBlockHash().GetHex());

            assert(hashPrevBlock == cw.blockCache.GetBestBlockHash());
        }
    }

    // Special case for the genesis block, skipping connection of its transactions.
    if (isGensisBlock) {
        if (!ProcessGenesisBlock(block, cw, pIndex, state)) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : process genesis block error"),
                            REJECT_INVALID, "process genesis-block-error");
        }
        return true;
    }

    // In stable coin genesis, need to verify txid for every transaction in block.
    if (block.GetHeight() == SysCfg().GetStableCoinGenesisHeight()) {
        assert(block.vptx.size() == 4);

        vector<string> ScoinTxIDs = IniCfg().GetStableCoinGenesisTxid(SysCfg().NetworkID());
        assert(ScoinTxIDs.size() == 3);
        for (int32_t index = 0; index < 3; ++index) {
            LogPrint(BCLog::INFO, "stable coin genesis block, txid actual: %s, should be: %s, in detail: %s\n",
                     block.vptx[index + 1]->GetHash().GetHex(), ScoinTxIDs[index],
                     block.vptx[index + 1]->ToString(cw.accountCache));

            assert(block.vptx[index + 1]->nTxType == UCOIN_MINT_TX);
            if (SysCfg().NetworkID() == MAIN_NET) {
                assert(block.vptx[index + 1]->GetHash() == uint256S(ScoinTxIDs[index]));
            }
        }
    }

    VoteDelegate curDelegate;
    uint32_t totalDelegateNum;
    if (!VerifyRewardTx(&block, cw, curDelegate, totalDelegateNum))
        return state.DoS(100, ERRORMSG("ConnectBlock() : verify reward tx error"), REJECT_INVALID, "bad-reward-tx");

    CBlockUndo blockUndo;
    int64_t nStart = GetTimeMicros();
    std::vector<pair<uint256, CDiskTxPos> > vPos;
    vPos.reserve(block.vptx.size());

    CDiskTxPos pos(pIndex->GetBlockPos(), GetSizeOfCompactSize(block.vptx.size()));
    CDiskTxPos rewardPos = pos;
    pos.nTxOffset += ::GetSerializeSize(block.vptx[0], SER_DISK, CLIENT_VERSION);

    // Re-compute reward values and total fuel
    uint64_t totalFuel                 = 0;
    map<TokenSymbol, uint64_t> rewards = {{SYMB::WICC, 0}, {SYMB::WUSD, 0}};  // Only allow WICC/WUSD as fees type.

    if (block.vptx.size() > 1) {
        assert(mapBlockIndex.count(cw.blockCache.GetBestBlockHash()));
        int32_t curHeight     = mapBlockIndex[cw.blockCache.GetBestBlockHash()]->height;
        int32_t validHeight   = SysCfg().GetTxCacheHeight();
        uint32_t fuelRate     = block.GetFuelRate();
        uint64_t totalRunStep = 0;

        for (int32_t index = 1; index < (int32_t)block.vptx.size(); ++index) {
            std::shared_ptr<CBaseTx> &pBaseTx = block.vptx[index];
            if (cw.txCache.HasTx((pBaseTx->GetHash())))
                return state.DoS(100, ERRORMSG("ConnectBlock() : txid=%s duplicated", pBaseTx->GetHash().GetHex()),
                                 REJECT_INVALID, "tx-duplicated");

            if (!pBaseTx->IsValidHeight(curHeight, validHeight))
                return state.DoS(100, ERRORMSG("ConnectBlock() : txid=%s beyond the scope of valid height",
                                 pBaseTx->GetHash().GetHex()), REJECT_INVALID, "tx-invalid-height");

            pBaseTx->nFuelRate = fuelRate;
            CTxUndoOpLogger opLogger(cw, pBaseTx->GetHash(), blockUndo);

            uint32_t prevBlockTime = pIndex->pprev != nullptr ? pIndex->pprev->GetBlockTime() : pIndex->GetBlockTime();
            CTxExecuteContext context(pIndex->height, index, fuelRate, pIndex->nTime, prevBlockTime, &cw, &state);
            if (!pBaseTx->CheckAndExecuteTx(context)) {
                pCdMan->pLogCache->SetExecuteFail(pIndex->height, pBaseTx->GetHash(), state.GetRejectCode(), state.GetRejectReason());
                return state.DoS(100, ERRORMSG("ConnectBlock() : txid=%s check/execute failed, in detail: %s",
                                 pBaseTx->GetHash().GetHex(), pBaseTx->ToString(cw.accountCache)), REJECT_INVALID, "tx-execute-failed");
            }

            vPos.push_back(make_pair(pBaseTx->GetHash(), pos));

            totalRunStep += pBaseTx->nRunStep;
            if (totalRunStep > MAX_BLOCK_RUN_STEP)
                return state.DoS(100, ERRORMSG("ConnectBlock() : total steps(%llu) exceed max steps(%llu)", totalRunStep,
                                 MAX_BLOCK_RUN_STEP), REJECT_INVALID, "exceed-max-fuel");

            auto fuel = pBaseTx->GetFuel(block.GetHeight(), block.GetFuelRate());
            totalFuel += fuel;

            auto fees_symbol = std::get<0>(pBaseTx->GetFees());
            assert(fees_symbol == SYMB::WICC || fees_symbol == SYMB::WUSD);  // Only allow WICC/WUSD as fees type.
            auto fees = std::get<1>(pBaseTx->GetFees());
            assert(fees >= fuel);
            rewards[fees_symbol] += (fees - fuel);

            pos.nTxOffset += ::GetSerializeSize(pBaseTx, SER_DISK, CLIENT_VERSION);

            LogPrint(BCLog::DEBUG, "total fuel fee:%d, tx fuel fee:%d runStep:%d fuelRate:%d txid:%s\n", totalFuel,
                     fuel, pBaseTx->nRunStep, fuelRate, pBaseTx->GetHash().GetHex());
        }
    }

    // Verify total fuel
    if (totalFuel != block.GetFuel())
        return state.DoS(100, ERRORMSG("ConnectBlock() : fuel fee value at block header calculate error(actual fuel "
                                       "fee=%lld vs block fuel fee=%lld)", totalFuel, block.GetFuel()));

    // Verify miner account
    CAccount delegateAccount;
    if (!cw.accountCache.GetAccount(block.vptx[0]->txUid, delegateAccount)) {
        assert(0);
    }

    // Verify reward values
    if (block.vptx[0]->nTxType == BLOCK_REWARD_TX) {
        auto pRewardTx = (CBlockRewardTx *)block.vptx[0].get();
        if (pRewardTx->reward_fees != rewards.at(SYMB::WICC)) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : invalid coinbase reward amount"), REJECT_INVALID,
                             "bad-reward-amount");
        }
    } else if (block.vptx[0]->nTxType == UCOIN_BLOCK_REWARD_TX) {
        auto pRewardTx = (CUCoinBlockRewardTx *)block.vptx[0].get();

        if (SysCfg().NetworkID() == TEST_NET && block.GetHeight() < 200000) {
            // TODO: remove me if reset testnet.
        } else {
            if (pRewardTx->reward_fees != rewards) {
                return state.DoS(100, ERRORMSG("ConnectBlock() : invalid coinbase reward amount"), REJECT_INVALID,
                                 "bad-reward-amount");
            }
        }

        // Verify profits
        uint64_t profits = delegateAccount.ComputeBlockInflateInterest(block.GetHeight(), curDelegate, totalDelegateNum);
        if (pRewardTx->inflated_bcoins != profits) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : invalid coinbase profits amount(actual=%d vs valid=%d)",
                             pRewardTx->inflated_bcoins, profits), REJECT_INVALID, "bad-reward-amount");
        }
    }

    {
        // Execute block reward transaction
        uint32_t prevBlockTime = pIndex->pprev != nullptr ? pIndex->pprev->GetBlockTime() : pIndex->GetBlockTime();
        CTxExecuteContext context(pIndex->height, 0, pIndex->nFuelRate, pIndex->nTime, prevBlockTime, &cw, &state);
        CTxUndoOpLogger rewardOpLogger(cw, block.vptx[0]->GetHash(), blockUndo);

        if (!block.vptx[0]->ExecuteFullTx(context)) {
            pCdMan->pLogCache->SetExecuteFail(pIndex->height, block.vptx[0]->GetHash(), state.GetRejectCode(),
                                            state.GetRejectReason());
            return state.DoS(100, ERRORMSG("ConnectBlock() : failed to execute reward transaction"));
        }

        if (pIndex->height + 1 == (int32_t)SysCfg().GetFeatureForkHeight() &&
            !ComputeVoteStakingInterestAndRevokeVotes(pIndex->height, pIndex->nTime, cw, state)) {
            return state.Abort(_("ConnectBlock() : failed to compute vote staking interest"));
        }

        if (!SaveTxIndex(block.vptx[0]->GetHash(), cw, state, rewardPos)) {
            return state.Abort(_("ConnectBlock() : failed to save tx index"));
        }

        for (const auto &item : vPos) {
            if (!SaveTxIndex(item.first, cw, state, item.second)) {
                return state.Abort(_("ConnectBlock() : failed to save tx index"));
            }
        }

        // TODO: move the block delegates undo to block_undo
        if (!chain::ProcessBlockDelegates(block, cw, state)) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : failed to process block delegates! block=%d:%s",
                block.GetHeight(), block.GetHash().ToString()));
        }
    }

    if (pIndex->height - BLOCK_REWARD_MATURITY > 0) {
        // Deal mature block reward transaction
        CBlockIndex *pMatureIndex = pIndex;
        for (int32_t i = 0; i < BLOCK_REWARD_MATURITY; ++i) {
            pMatureIndex = pMatureIndex->pprev;
        }

        if (nullptr != pMatureIndex) {
            CBlock matureBlock;
            if (!ReadBlockFromDisk(pMatureIndex, matureBlock)) {
                return state.Abort(_("ConnectBlock() : read mature block error"));
            }

            uint32_t prevBlockTime = pIndex->pprev != nullptr ? pIndex->pprev->GetBlockTime() : pIndex->GetBlockTime();
            CTxExecuteContext context(pIndex->height, -1, pIndex->nFuelRate, pIndex->nTime, prevBlockTime, &cw, &state);
            CTxUndoOpLogger rewardOpLogger(cw, block.vptx[0]->GetHash(), blockUndo);
            if (!matureBlock.vptx[0]->ExecuteFullTx(context)) {
                pCdMan->pLogCache->SetExecuteFail(pIndex->height, matureBlock.vptx[0]->GetHash(), state.GetRejectCode(),
                                                  state.GetRejectReason());
                return state.DoS(100, ERRORMSG("ConnectBlock() : execute mature block reward tx error"));
            }
        }
    }
    int64_t nTime = GetTimeMicros() - nStart;
    if (SysCfg().IsBenchmark())
        LogPrint(BCLog::INFO, "- Connect %u transactions: %.2fms (%.3fms/tx)\n",
                 (uint32_t)block.vptx.size(), 0.001 * nTime, 0.001 * nTime / block.vptx.size());

    if (fJustCheck)
        return true;

    // Write undo information to disk
    if (pIndex->GetUndoPos().IsNull() || (pIndex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS) {
        if (pIndex->GetUndoPos().IsNull()) {
            CDiskBlockPos pos;
            if (!FindUndoPos(state, pIndex->nFile, pos, ::GetSerializeSize(blockUndo, SER_DISK, CLIENT_VERSION) + 40))
                return state.Abort(_("ConnectBlock() : failed to find undo data's position"));

            // uint256 preHash;
            // if(pIndex->previous != nullptr){
            //     CBlock preBlock;
            //     if (!ReadBlockFromDisk(pDeleteBlockIndex, deleteBlock)) {
            //         return state.Abort(_("ConnectBlock() : failed to read block"));
            //     }
            //     preHash = preBlock.GetMerkleRootHash();
            // }else{
            //     preHash = uint256{};
            // }

            if (!blockUndo.WriteToDisk(pos, pIndex->pprev->GetBlockHash()))
                return state.Abort(_("ConnectBlock() : failed to write undo data"));

            //block.SetMerkleRootHash(blockUndo.CalcStateHash(preHash));

            // Update nUndoPos in block index
            pIndex->nUndoPos = pos.nPos;
            pIndex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pIndex->nStatus = (pIndex->nStatus & ~BLOCK_VALID_MASK) | BLOCK_VALID_SCRIPTS;

        CDiskBlockIndex blockIndex(pIndex);
        if (!pCdMan->pBlockIndexDb->WriteBlockIndex(blockIndex))
            return state.Abort(_("ConnectBlock() : failed to write block index"));
    }

    if (!cw.txCache.AddBlockTx(block)) {
        return state.Abort(_("ConnectBlock() : failed add block into transaction memory cache"));
    }

    if (pIndex->height > SysCfg().GetTxCacheHeight()) {
        CBlockIndex *pDeleteBlockIndex = pIndex;
        int32_t nCacheHeight           = SysCfg().GetTxCacheHeight();
        while (pDeleteBlockIndex && nCacheHeight-- > 0) {
            pDeleteBlockIndex = pDeleteBlockIndex->pprev;
        }

        CBlock deleteBlock;
        if (!ReadBlockFromDisk(pDeleteBlockIndex, deleteBlock)) {
            return state.Abort(_("ConnectBlock() : failed to read block"));
        }

        if (!cw.txCache.RemoveBlockTx(deleteBlock)) {
            return state.Abort(_("ConnectBlock() : failed delete block from transaction memory cache"));
        }
    }

    if (!cw.ppCache.PushBlock(cw.sysParamCache, pIndex))
        return state.Abort(_("ConnectBlock() : push block to price point memory cache failed"));

    // Set best block to current account cache.
    cw.blockCache.SetBestBlock(pIndex->GetBlockHash());

    return true;
}

// Update the on-disk chain state.
bool static WriteChainState(CValidationState &state) {
    static int64_t nLastWrite = 0;
    uint32_t cacheSize        =
        pCdMan->pSysParamCache->GetCacheSize() +
        pCdMan->pAccountCache->GetCacheSize() +
        pCdMan->pAssetCache->GetCacheSize() +
        pCdMan->pContractCache->GetCacheSize() +
        pCdMan->pDelegateCache->GetCacheSize() +
        pCdMan->pCdpCache->GetCacheSize() +
        pCdMan->pClosedCdpCache->GetCacheSize() +
        pCdMan->pDexCache->GetCacheSize() +
        pCdMan->pBlockCache->GetCacheSize() +
        pCdMan->pLogCache->GetCacheSize() +
        pCdMan->pReceiptCache->GetCacheSize();

    if (!IsInitialBlockDownload() || cacheSize > SysCfg().GetCacheSize() ||
        GetTimeMicros() > nLastWrite + 60 * 1000000) {
        // Typical CCoins structures on disk are around 100 bytes in size.
        // Pushing a new one to the database can cause it to be written
        // twice (once in the log, and once in the tables). This is already
        // an overestimation, as most will delete an existing entry or
        // overwrite one. Still, use a conservative safety factor of 2.
        if (!CheckDiskSpace(cacheSize))
            return state.Error("out of disk space");

        FlushBlockFile();
        // pCdMan->pBlockCache->Sync();
        pCdMan->Flush();
        mapForkCache.clear();
        nLastWrite = GetTimeMicros();
    }
    return true;
}

// Update chainActive and related internal data structures.
void static UpdateTip(CBlockIndex *pIndexNew, const CBlock &block) {
    chainActive.SetTip(pIndexNew);

    SyncTransaction(uint256(), nullptr, &block);

    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = IsInitialBlockDownload();
    if ((chainActive.Height() % 20160) == 0 || (!fIsInitialDownload && (chainActive.Height() % 144) == 0))
        g_signals.SetBestChain(chainActive.GetLocator());

    // New best block
    SysCfg().SetBestRecvTime(GetTime());
    LogPrint(BCLog::INFO, "UpdateTip[%d]: %s blkTxCnt=%d chainTxCnt=%lu fuelRate=%d ts=%s\n",
             chainActive.Height(), chainActive.Tip()->GetBlockHash().ToString(),
             block.vptx.size(), chainActive.Tip()->nChainTx, chainActive.Tip()->nFuelRate,
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()));

    // Check the version of the last 100 blocks to see if we need to upgrade:
    if (!fIsInitialDownload) {
        int32_t nUpgraded             = 0;
        const CBlockIndex *pIndex = chainActive.Tip();
        for (int32_t i = 0; i < 100 && pIndex != nullptr; i++) {
            if (pIndex->nVersion > CBlock::CURRENT_VERSION)
                ++nUpgraded;
            pIndex = pIndex->pprev;
        }
        if (nUpgraded > 0)
            LogPrint(BCLog::INFO, "SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int32_t)CBlock::CURRENT_VERSION);
        if (nUpgraded > 100 / 2)
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning = _("Warning: This version is obsolete, upgrade required!");
    }
}

// Disconnect chainActive's tip.
bool static DisconnectTip(CValidationState &state) {
    CBlockIndex *pIndexDelete = chainActive.Tip();
    assert(pIndexDelete);
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(pIndexDelete, block))
        return state.Abort(_("Failed to read blocks from disk."));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        auto spCW = std::make_shared<CCacheWrapper>(pCdMan);

        if (!DisconnectBlock(block, *spCW, pIndexDelete, state))
            return ERRORMSG("DisconnectTip() : DisconnectBlock %s failed", pIndexDelete->GetBlockHash().ToString());

        // Need to re-sync all to global cache layer.
        spCW->Flush();

        // Attention: need to reset the lastest block price median
        CBlockIndex *pPreBlockIndex = pIndexDelete->pprev;
        CBlock preBlock;
        if (pPreBlockIndex) {
            if (!ReadBlockFromDisk(pPreBlockIndex, preBlock))
                return ERRORMSG("DisconnectTip() : failed to read block [%d]: %s", pPreBlockIndex->height,
                                pPreBlockIndex->GetBlockHash().ToString());
        }
    }
    if (SysCfg().IsBenchmark())
        LogPrint(BCLog::INFO, "- Disconnect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;
    // Update chainActive and related variables.
    UpdateTip(pIndexDelete->pprev, block);
    // Resurrect mempool transactions from the disconnected block.
    for (const auto &pTx : block.vptx) {
        list<std::shared_ptr<CBaseTx> > removed;
        CValidationState stateDummy;
        if (!pTx->IsForbidRelay()) {
            if (!AcceptToMemoryPool(mempool, stateDummy, pTx.get(), false)) {
                mempool.Remove(pTx.get(), removed, true);
            }
        } else {
            EraseTransactionFromWallet(pTx->GetHash());
        }
    }

    return true;
}

// Connect a new block to chainActive.
bool static ConnectTip(CValidationState &state, CBlockIndex *pIndexNew) {
    assert(pIndexNew->pprev == chainActive.Tip());
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(pIndexNew, block))
        return state.Abort(strprintf("Failed to read block hash: %s", pIndexNew->GetBlockHash().GetHex()));

    // Apply the block automatically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CInv inv(MSG_BLOCK, pIndexNew->GetBlockHash());

        auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
        if (!ConnectBlock(block, *spCW, pIndexNew, state)) {
            if (state.IsInvalid()) {
                InvalidBlockFound(pIndexNew, state);
            }

            return ERRORMSG("ConnectTip() : ConnectBlock [%d]:%s failed", pIndexNew->height, pIndexNew->GetBlockHash().ToString());
        }
        {
            LOCK(cs_mapNodeState);
            mapBlockSource.erase(inv.hash);
        }

        // Need to re-sync all to global cache layer.
        spCW->Flush();
    }

    if (SysCfg().IsBenchmark())
        LogPrint(BCLog::INFO, "- Connect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);

    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;

    // Update chainActive & related variables.
    UpdateTip(pIndexNew, block);

    for (auto &pTxItem : block.vptx) {
        mempool.memPoolTxs.erase(pTxItem->GetHash());
    }
    return true;
}

// Make chainMostWork correspond to the chain with the most work in it, that isn't
// known to be invalid (it's however far from certain to be valid).
void static FindMostWorkChain() {
    CBlockIndex *pIndexNew = nullptr;

    // In case the current best is invalid, do not consider it.
    while (chainMostWork.Tip() && (chainMostWork.Tip()->nStatus & BLOCK_FAILED_MASK)) {
        setBlockIndexValid.erase(chainMostWork.Tip());
        chainMostWork.SetTip(chainMostWork.Tip()->pprev);
    }

    do {
        // Find the best candidate header.
        {
            set<CBlockIndex *, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexValid.rbegin();
            if (it == setBlockIndexValid.rend())
                return;
            pIndexNew = *it;
        }

        // Check whether all blocks on the path between the currently active chain and the candidate are valid.
        // Just going until the active chain is an optimization, as we know all blocks in it are valid already.
        CBlockIndex *pindexTest = pIndexNew;
        bool fInvalidAncestor   = false;
        while (pindexTest && !chainActive.Contains(pindexTest)) {
            if (pindexTest->nStatus & BLOCK_FAILED_MASK) {
                // Candidate has an invalid ancestor, remove entire chain from the set.
                if (pIndexBestInvalid == nullptr || pIndexNew->nChainWork > pIndexBestInvalid->nChainWork)
                    pIndexBestInvalid = pIndexNew;
                CBlockIndex *pindexFailed = pIndexNew;
                while (pindexTest != pindexFailed) {
                    pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                    setBlockIndexValid.erase(pindexFailed);
                    pindexFailed = pindexFailed->pprev;
                }
                fInvalidAncestor = true;
                break;
            }
            pindexTest = pindexTest->pprev;
        }
        if (fInvalidAncestor)
            continue;

        break;
    } while (true);

    // Check whether it's actually an improvement.
    if (chainMostWork.Tip() && !CBlockIndexWorkComparator()(chainMostWork.Tip(), pIndexNew))
        return;

    // We have a new best.
    chainMostWork.SetTip(pIndexNew);
}

bool ConnectBlockOnFinChain(CBlockIndex* pNewIndex, CValidationState& state) {
    if (pNewIndex && (chainActive.Tip() == pNewIndex->pprev)) {
        if (!ConnectTip(state, pNewIndex)) {
            if (state.IsInvalid()) {
                if (!state.CorruptionPossible()) // The block violates a consensus rule.
                    InvalidChainFound(pNewIndex);

            } else {
                // A system error occurred (disk space, database error, ...).
                return false;
            }
        }

        mempool.ReScanMemPoolTx();
    }
    return true ;

}
// Try to activate to the most-work chain (thereby connecting it).
bool ActivateBestChain(CValidationState &state, CBlockIndex* pNewIndex) {
    LOCK(cs_main);
    CBlockIndex *pIndexOldTip = chainActive.Tip();
    bool fComplete            = false;

    while (!fComplete) {
        FindMostWorkChain();
        fComplete = true;

        // Check whether we have something to do.
        if (chainMostWork.Tip() == nullptr)
            break;

        auto height = chainActive.Height();
        while(height >= 0 ){
            auto chainIndex = chainActive[height] ;
            if( chainIndex &&!chainMostWork.Contains(chainIndex)){
                CBlockIndex* finIndex = pbftMan.GetLocalFinIndex();
                if(finIndex && chainIndex->GetBlockHash() == finIndex->GetBlockHash()){
                    LogPrint(BCLog::INFO, "finality block can't be reverse\n");
                    if (GetTime() - pbftMan.GetLocalFinLastUpdate() > 60) {
                        pbftMan.SetLocalFinTimeout() ;
                    } else{
                        LogPrint(BCLog::INFO, "connect block on fin chain\n");
                        return ConnectBlockOnFinChain(pNewIndex, state);
                    }
                }

                uint256 globalFinIndexHash = pbftMan.GetGlobalFinBlockHash();
                if( chainIndex->GetBlockHash() == globalFinIndexHash){
                    LogPrint(BCLog::INFO, "globalfinality block can't be reverse\n");
                    return ConnectBlockOnFinChain(pNewIndex, state) ;
                }

                height-- ;
            }else if (chainIndex&& chainMostWork.Contains(chainIndex)){
                break ;
            }else if(chainIndex == nullptr)
                return true ;
        }

        // Disconnect active blocks which are no longer in the best chain.
        while (chainActive.Tip() && !chainMostWork.Contains(chainActive.Tip())) {
            if (!DisconnectTip(state))
                return false;

            if (chainActive.Tip() && chainMostWork.Contains(chainActive.Tip()))
                mempool.ReScanMemPoolTx();
        }

        // Connect new blocks.
        while (!chainActive.Contains(chainMostWork.Tip())) {
            CBlockIndex *pIndexConnect = chainMostWork[chainActive.Height() + 1];
            if (!ConnectTip(state, pIndexConnect)) {
                if (state.IsInvalid()) {
                    // The block violates a consensus rule.
                    if (!state.CorruptionPossible())
                        InvalidChainFound(chainMostWork.Tip());
                    fComplete = false;
                    state     = CValidationState();
                    break;
                } else {
                    // A system error occurred (disk space, database error, ...).
                    return false;
                }
            }

            if (chainActive.Contains(chainMostWork.Tip())) {
                mempool.ReScanMemPoolTx();
            }
        }
    }

    if (chainActive.Tip() != pIndexOldTip) {
        string strCmd = SysCfg().GetArg("-blocknotify", "");
        if (!IsInitialBlockDownload() && !strCmd.empty()) {
            boost::replace_all(strCmd, "%s", chainActive.Tip()->GetBlockHash().GetHex());
            boost::thread t(runCommand, strCmd);  // thread runs free
        }
    }

    return true;
}

bool AddToBlockIndex(CBlock &block, CValidationState &state, const CDiskBlockPos &pos) {
    // Check for duplicate
    uint256 hash = block.GetHash();
    if (mapBlockIndex.count(hash))
        return state.Invalid(ERRORMSG("AddToBlockIndex() : %s already exists", hash.ToString()), 0, "duplicate");

    // Construct new block index object
    CBlockIndex *pIndexNew = new CBlockIndex(block);

    assert(pIndexNew);
    {
        LOCK(cs_nBlockSequenceId);
        pIndexNew->nSequenceId = nBlockSequenceId++;
    }
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.insert(make_pair(hash, pIndexNew)).first;
    // LogPrint(BCLog::INFO, "in map hash:%s map size:%d\n", hash.GetHex(), mapBlockIndex.size());
    pIndexNew->pBlockHash                        = &((*mi).first);
    map<uint256, CBlockIndex *>::iterator miPrev = mapBlockIndex.find(block.GetPrevBlockHash());
    if (miPrev != mapBlockIndex.end()) {
        pIndexNew->pprev  = (*miPrev).second;
        pIndexNew->height = pIndexNew->pprev->height + 1;
        pIndexNew->BuildSkip();
    }

    if(block.GetHeight() == 0 )
        pIndexNew->miner = CRegID("0-1");
    else
        pIndexNew->miner = block.vptx[0]->txUid.get<CRegID>();
    pIndexNew->nTx        = block.vptx.size();
    pIndexNew->nChainWork = pIndexNew->height;
    pIndexNew->nChainTx   = (pIndexNew->pprev ? pIndexNew->pprev->nChainTx : 0) + pIndexNew->nTx;
    pIndexNew->nFile      = pos.nFile;
    pIndexNew->nDataPos   = pos.nPos;
    pIndexNew->nUndoPos   = 0;
    pIndexNew->nStatus    = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
    setBlockIndexValid.insert(pIndexNew);

    if (!pCdMan->pBlockIndexDb->WriteBlockIndex(CDiskBlockIndex(pIndexNew)))
        return state.Abort(_("Failed to write block index"));
    int64_t beginTime = GetTimeMillis();
    // New best?
    if (!ActivateBestChain(state, pIndexNew)) {
        LogPrint(BCLog::INFO, "ActivateBestChain() elapse time:%lld ms\n", GetTimeMillis() - beginTime);
        return false;
    }
    // LogPrint(BCLog::INFO, "ActivateBestChain() elapse time:%lld ms\n", GetTimeMillis() - beginTime);
    LOCK(cs_main);
    if (pIndexNew == chainActive.Tip()) {
        // Clear fork warning if its no longer applicable
        CheckForkWarningConditions();
    } else
        CheckForkWarningConditionsOnNewFork(pIndexNew);

    if (!pCdMan->pBlockCache->Flush())
        return state.Abort(_("Failed to sync block index"));

    if (chainActive.Height() > nSyncTipHeight)
        nSyncTipHeight = chainActive.Height();

    return true;
}

bool FindBlockPos(CValidationState &state, CDiskBlockPos &pos, uint32_t nAddSize, uint32_t height, uint64_t nTime,
                  bool fKnown = false) {
    bool fUpdatedLast = false;

    LOCK(cs_LastBlockFile);

    if (fKnown) {
        if (nLastBlockFile != pos.nFile) {
            nLastBlockFile = pos.nFile;
            infoLastBlockFile.SetNull();
            pCdMan->pBlockIndexDb->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);
            fUpdatedLast = true;
        }
    } else {
        while (infoLastBlockFile.nSize + nAddSize >= MAX_BLOCKFILE_SIZE) {
            LogPrint(BCLog::INFO, "Leaving block file %d: %s\n", nLastBlockFile, infoLastBlockFile.ToString());
            FlushBlockFile(true);
            nLastBlockFile++;
            infoLastBlockFile.SetNull();
            pCdMan->pBlockIndexDb->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);  // check whether data for the new file somehow already exist; can fail just fine
            fUpdatedLast = true;
        }
        pos.nFile = nLastBlockFile;
        pos.nPos  = infoLastBlockFile.nSize;
    }

    infoLastBlockFile.nSize += nAddSize;
    infoLastBlockFile.AddBlock(height, nTime);

    if (!fKnown) {
        uint32_t nOldChunks = (pos.nPos + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        uint32_t nNewChunks = (infoLastBlockFile.nSize + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        if (nNewChunks > nOldChunks) {
            if (CheckDiskSpace(nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos)) {
                FILE *file = OpenBlockFile(pos);
                if (file) {
                    LogPrint(BCLog::INFO, "Pre-allocating up to position 0x%x in blk%05u.dat\n", nNewChunks * BLOCKFILE_CHUNK_SIZE, pos.nFile);
                    AllocateFileRange(file, pos.nPos, nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos);
                    fclose(file);
                }
            } else
                return state.Error("out of disk space");
        }
    }

    if (!pCdMan->pBlockIndexDb->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        return state.Abort(_("Failed to write file info"));

    if (fUpdatedLast)
        pCdMan->pBlockCache->WriteLastBlockFile(nLastBlockFile);

    return true;
}

bool ProcessForkedChain(const CBlock &block, CBlockIndex *pPreBlockIndex, CValidationState &state) {
    bool forkChainTipFound = false;
    uint256 forkChainTipBlockHash;
    vector<CBlock> vPreBlocks;
    std::shared_ptr<CCacheWrapper> spCW = nullptr;

    // If the block's previous block is not the active chain's tip, find the forked point.
    while (!chainActive.Contains(pPreBlockIndex)) {
        if (!forkChainTipFound) {
            if (mapForkCache.count(pPreBlockIndex->GetBlockHash())) {
                forkChainTipBlockHash = pPreBlockIndex->GetBlockHash();
                forkChainTipFound     = true;
                LogPrint(BCLog::INFO, "ProcessForkedChain() : fork chain's best block [%d]: %s\n",
                         pPreBlockIndex->height, forkChainTipBlockHash.GetHex());
            } else {
                CBlock block;
                if (!ReadBlockFromDisk(pPreBlockIndex, block))
                    return state.Abort(_("Failed to read block"));

                // Reserve the forked chain's blocks.
                vPreBlocks.push_back(block);
            }
        }

        pPreBlockIndex = pPreBlockIndex->pprev;

        // FIXME: enable it to avoid forked chain attack.
        if (chainActive.Height() - pPreBlockIndex->height > SysCfg().GetMaxForkHeight(block.GetHeight()))
            return state.DoS(100, ERRORMSG(
                    "ProcessForkedChain() : block at fork chain too earlier than tip block hash=%s block height=%d\n",
                    block.GetHash().GetHex(), block.GetHeight()));

        if (mapBlockIndex.find(pPreBlockIndex->GetBlockHash()) == mapBlockIndex.end())
            return state.DoS(10, ERRORMSG("ProcessForkedChain() : prev block not found"), 0, "bad-prevblk");
    }

    // FIXME: enable it to avoid forked chain attack.
    if (chainActive.Height() - pPreBlockIndex->height > SysCfg().GetMaxForkHeight(block.GetHeight()))
        return state.DoS(100, ERRORMSG(
                "ProcessForkedChain() : block at fork chain too earlier than tip block hash=%s block height=%d\n",
                block.GetHash().GetHex(), block.GetHeight()));

    if (forkChainTipFound) {
        spCW = mapForkCache[forkChainTipBlockHash];
    } else if (mapForkCache.count(pPreBlockIndex->GetBlockHash())) {
        forkChainTipBlockHash = pPreBlockIndex->GetBlockHash();
        spCW                  = mapForkCache[forkChainTipBlockHash];
        forkChainTipFound     = true;
        LogPrint(BCLog::INFO, "ProcessForkedChain() : found [%d]: %s in cache\n",
            pPreBlockIndex->height, forkChainTipBlockHash.GetHex());
    } else {
        spCW                     = CCacheWrapper::NewCopyFrom(pCdMan);
        int64_t beginTime        = GetTimeMillis();
        CBlockIndex *pBlockIndex = chainActive.Tip();

        while (pPreBlockIndex != pBlockIndex) {
            LogPrint(BCLog::INFO, "ProcessForkedChain() : disconnect block [%d]: %s\n", pBlockIndex->height,
                     pBlockIndex->GetBlockHash().GetHex());

            CBlock block;
            if (!ReadBlockFromDisk(pBlockIndex, block))
                return state.Abort(_("Failed to read block"));

            bool bfClean = true;
            if (!DisconnectBlock(block, *spCW, pBlockIndex, state, &bfClean)) {
                return ERRORMSG("ProcessForkedChain() : failed to disconnect block [%d]: %s", pBlockIndex->height,
                                pBlockIndex->GetBlockHash().ToString());
            }

            pBlockIndex = pBlockIndex->pprev;
        }  // Rollback the active chain to the forked point.

        mapForkCache[pPreBlockIndex->GetBlockHash()] = spCW;
        forkChainTipBlockHash = pPreBlockIndex->GetBlockHash();
        forkChainTipFound     = true;
        LogPrint(BCLog::INFO, "ProcessForkedChain() : add [%d]: %s to cache\n", pPreBlockIndex->height,
                 pPreBlockIndex->GetBlockHash().GetHex());

        LogPrint(BCLog::INFO, "ProcessForkedChain() : disconnect blocks elapse: %lld ms\n", GetTimeMillis() - beginTime);
    }

    uint256 forkChainBestBlockHash   = spCW->blockCache.GetBestBlockHash();
    int32_t forkChainBestBlockHeight = mapBlockIndex[forkChainBestBlockHash]->height;
    LogPrint(BCLog::INFO, "ProcessForkedChain() : fork chain's best block [%d]: %s\n", forkChainBestBlockHeight,
             forkChainBestBlockHash.GetHex());

    if (!vPreBlocks.empty()) {
        auto spNewForkCW = std::make_shared<CCacheWrapper>(spCW.get());
        // Connect all of the forked chain's blocks.
        for (auto rIter = vPreBlocks.rbegin(); rIter != vPreBlocks.rend(); ++rIter) {
            LogPrint(BCLog::INFO, "ProcessForkedChain() : ConnectBlock block height=%d hash=%s\n", rIter->GetHeight(),
                    rIter->GetHash().GetHex());

            if (!ConnectBlock(*rIter, *spNewForkCW, mapBlockIndex[rIter->GetHash()], state, false)) {
                return ERRORMSG("ProcessForkedChain() : ConnectBlock %s failed", rIter->GetHash().ToString());
            }

            CBlockIndex *pConnBlockIndex = mapBlockIndex[rIter->GetHash()];
            if (pConnBlockIndex->nStatus | BLOCK_FAILED_MASK) {
                pConnBlockIndex->nStatus = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
            }
        }

        spNewForkCW->Flush();  // flush to spNewForkCW

        vector<CBlock>::iterator iterBlock = vPreBlocks.begin();
        if (forkChainTipFound) {
            mapForkCache.erase(forkChainTipBlockHash);
        }

        mapForkCache[iterBlock->GetHash()] = spCW;
    }

    VoteDelegate curDelegate;
    uint32_t totalDelegateNum ;
    if (!VerifyRewardTx(&block, *spCW, curDelegate, totalDelegateNum))
        return state.DoS(100, ERRORMSG("ProcessForkedChain() : block[%u]: %s verify reward tx error",
                        block.GetHeight(), block.GetHash().GetHex()), REJECT_INVALID, "bad-reward-tx");

    return true;
}

bool CheckBlock(const CBlock &block, CValidationState &state, CCacheWrapper &cw, bool fCheckTx, bool fCheckMerkleRoot) {
    if (block.vptx.empty() || block.vptx.size() > MAX_BLOCK_SIZE ||
        ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, ERRORMSG("CheckBlock() : size limits failed"), REJECT_INVALID, "bad-blk-length");

    if ((block.GetHeight() != 0 || block.GetHash() != SysCfg().GetGenesisBlockHash()) &&
        block.GetVersion() != CBlockHeader::CURRENT_VERSION) {
        return state.Invalid(ERRORMSG("CheckBlock() : block version error"), REJECT_INVALID, "block-version-error");
    }

    // Check timestamp `block interval' + 2 seconds limits
    if (block.GetBlockTime() > GetAdjustedTime() + ::GetBlockInterval(block.GetHeight()) + 2)
        return state.Invalid(ERRORMSG("CheckBlock() : block timestamp too far in the future"), REJECT_INVALID,
                             "time-too-new");

    // First transaction must be reward transaction, the rest must not be
    if (block.vptx.empty() || !block.vptx[0]->IsBlockRewardTx())
        return state.DoS(100, ERRORMSG("CheckBlock() : first tx is not coinbase"), REJECT_INVALID, "bad-cb-missing");

    // Build the merkle tree already. We need it anyway later, and it makes the
    // block cache the transaction hashes, which means they don't need to be
    // recalculated many times during this block's validation.
    block.BuildMerkleTree();

    // Check for duplicate txids. This is caught by ConnectInputs(),
    // but catching it earlier avoids a potential DoS attack:
    set<uint256> uniqueTxSet;
    uint32_t priceMedianTxCount = 0;
    for (uint32_t i = 0; i < block.vptx.size(); i++) {
        uniqueTxSet.insert(block.GetTxid(i));
        if (block.GetHeight() != 0 || block.GetHash() != SysCfg().GetGenesisBlockHash()) {
            if (i > 0 && block.vptx[i]->IsBlockRewardTx())
                return state.DoS(100, ERRORMSG("CheckBlock() : more than one block reward tx"), REJECT_INVALID,
                                "bad-block-reward-tx-multiple");
        }

        if (block.vptx[i]->IsPriceMedianTx())
            priceMedianTxCount++;
    }


    // In stable coin release, every block should have one price median tx only.
    if (GetFeatureForkVersion(block.GetHeight()) >= MAJOR_VER_R2 && priceMedianTxCount != 1)
        return state.DoS(100, ERRORMSG("CheckBlock() : price median tx number error"), REJECT_INVALID,
                         "bad-price-median-tx-number");

    if (uniqueTxSet.size() != block.vptx.size())
        return state.DoS(100, ERRORMSG("CheckBlock() : duplicate transaction"), REJECT_INVALID, "bad-tx-duplicated",
                        true);

    // Check merkle root
    if (fCheckMerkleRoot && block.GetMerkleRootHash() != block.vMerkleTree.back())
        return state.DoS(100, ERRORMSG("CheckBlock() : merkleRootHash mismatch, height: %u, merkleRootHash(in block: %s vs calculate: %s)",
                        block.GetHeight(), block.GetMerkleRootHash().ToString(), block.vMerkleTree.back().ToString()),
                        REJECT_INVALID, "bad-merkle-root", true);

    // Check nonce
    static uint64_t maxNonce = SysCfg().GetBlockMaxNonce();
    if (block.GetNonce() > maxNonce)
        return state.Invalid(ERRORMSG("CheckBlock() : Nonce is larger than maxNonce"), REJECT_INVALID, "Nonce-too-large");

    return true;
}

bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp, bool mining) {
    AssertLockHeld(cs_main);

    uint256 blockHash = block.GetHash();
    LogPrint(BCLog::INFO, "AcceptBlock[%d]: %s, miner: %s, ts: %u\n", block.GetHeight(), blockHash.GetHex(),
             block.GetMinerUserID().ToString(), block.GetBlockTime());

    // Check for duplicated block
    if (mapBlockIndex.count(blockHash))
        return state.Invalid(ERRORMSG("AcceptBlock() : block already in mapBlockIndex"), 0, "duplicated");

    assert(block.GetHeight() == 0 || mapBlockIndex.count(block.GetPrevBlockHash()));

    if (block.GetHeight() != 0 && block.GetFuelRate() != GetElementForBurn(mapBlockIndex[block.GetPrevBlockHash()]))
        return state.DoS(100, ERRORMSG("AcceptBlock() : block fuel rate unmatched"), REJECT_INVALID,
                         "fuel-rate-unmatched");

    // Get prev block index
    CBlockIndex *pPrevBlockIndex = nullptr;
    int32_t height = 0;
    if (block.GetHeight() != 0 || blockHash != SysCfg().GetGenesisBlockHash()) {
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(block.GetPrevBlockHash());
        if (mi == mapBlockIndex.end())
            return state.DoS(10, ERRORMSG("AcceptBlock() : prev block not found"), 0, "bad-prevblk");

        pPrevBlockIndex = (*mi).second;
        height          = pPrevBlockIndex->height + 1;

        if (block.GetHeight() != (uint32_t)height) {
            return state.DoS(100, ERRORMSG("AcceptBlock() : height given in block mismatches with its actual height"),
                             REJECT_INVALID, "incorrect-height");
        }

        // Check timestamp against prev
        if (block.GetBlockTime() <= pPrevBlockIndex->GetBlockTime() ||
            (block.GetBlockTime() - pPrevBlockIndex->GetBlockTime()) < GetBlockInterval(block.GetHeight())) {
            return state.Invalid(ERRORMSG("AcceptBlock() : the new block came in too early"),
                                REJECT_INVALID, "time-too-early");
        }

        if (pPrevBlockIndex->GetBlockHash() != chainActive.Tip()->GetBlockHash()) {
            if (!ProcessForkedChain(block, pPrevBlockIndex, state)) {
                return state.DoS(100, ERRORMSG("AcceptBlock() : failed to process forked chain"), REJECT_INVALID,
                                "failed-to-process-forked-chain");
            }
        }

        // Reject block.nVersion=1 blocks when 95% (75% on testnet) of the network has been upgraded:
        if (block.GetVersion() < 2) {
            if ((!TestNet() && CBlockIndex::IsSuperMajority(2, pPrevBlockIndex, 950, 1000)) ||
                (TestNet() && CBlockIndex::IsSuperMajority(2, pPrevBlockIndex, 75, 100))) {
                return state.Invalid(ERRORMSG("AcceptBlock() : rejected nVersion=1 block"), REJECT_OBSOLETE, "bad-version");
            }
        }
    }

    // Write block to history file
    try {
        uint32_t nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
        CDiskBlockPos blockPos;
        if (dbp != nullptr)
            blockPos = *dbp;

        if (!FindBlockPos(state, blockPos, nBlockSize + 8, height, block.GetTime(), dbp != nullptr))
            return ERRORMSG("AcceptBlock() : FindBlockPos failed");

        if (dbp == nullptr && !WriteBlockToDisk(block, blockPos))
            return state.Abort(_("Failed to write block"));

        if (!AddToBlockIndex(block, state, blockPos))
            return ERRORMSG("AcceptBlock() : AddToBlockIndex failed");

    } catch (std::runtime_error &e) {
        ERRORMSG("AcceptBlock() : catch exception: %s", e.what());
        return state.Abort(_("System error: ") + e.what());
    }

    // Relay inventory, but don't relay old inventory during initial block download
    CBlockIndex* pTip = chainActive.Tip() ;
    if (pTip->GetBlockHash() == blockHash) {
        {
            LOCK(cs_vNodes);
            for (auto pNode : vNodes) {
                //p2p_xiaoyu_20191116
                if (mining) {
                    pNode->PushMessage(NetMsgType::BLOCK, block);
                    continue;
                }
                if (chainActive.Height() > (pNode->nStartingHeight != -1 ? pNode->nStartingHeight - 2000 : 0))
                    pNode->PushInventory(CInv(MSG_BLOCK, blockHash));
            }
        }

        VoteDelegateVector delegates;
        if (pCdMan->pDelegateCache->GetActiveDelegates(delegates)) {
            pbftContext.SaveMinersByHash(blockHash, delegates);
        }

        BroadcastBlockConfirm(pTip) ;
        if(pbftMan.UpdateLocalFinBlock(pTip)){
            BroadcastBlockFinality(pTip);
            pbftMan.UpdateGlobalFinBlock(pTip);
        }
    }

    return true;
}

bool CBlockIndex::IsSuperMajority(int32_t minVersion, const CBlockIndex *pstart, uint32_t nRequired, uint32_t nToCheck) {
    uint32_t nFound = 0;
    for (uint32_t i = 0; i < nToCheck && nFound < nRequired && pstart != nullptr; i++) {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }

    return (nFound >= nRequired);
}

int64_t CBlockIndex::GetMedianTime() const {
    AssertLockHeld(cs_main);
    const CBlockIndex *pIndex = this;
    for (int32_t i = 0; i < nMedianTimeSpan / 2; i++) {
        if (!chainActive.Next(pIndex))
            return GetBlockTime();

        pIndex = chainActive.Next(pIndex);
    }

    return pIndex->GetMedianTimePast();
}

/** Turn the lowest '1' bit in the binary representation of a number into a '0'. */
int32_t static inline InvertLowestOne(int32_t n) { return n & (n - 1); }

/** Compute what height to jump back to with the CBlockIndex::pskip pointer. */
int32_t static inline GetSkipHeight(int32_t height) {
    if (height < 2)
        return 0;

    // Determine which height to jump back to. Any number strictly lower than height is acceptable,
    // but the following expression seems to perform well in simulations (max 110 steps to go back
    // up to 2**18 blocks).
    return (height & 1) ? InvertLowestOne(InvertLowestOne(height - 1)) + 1 : InvertLowestOne(height);
}

CBlockIndex *CBlockIndex::GetAncestor(int32_t heightIn) {
    if (heightIn > height || heightIn < 0)
        return nullptr;

    CBlockIndex *pindexWalk = this;
    int32_t heightWalk          = height;
    while (heightWalk > heightIn) {
        int32_t heightSkip     = GetSkipHeight(heightWalk);
        int32_t heightSkipPrev = GetSkipHeight(heightWalk - 1);
        if (heightSkip == heightIn ||
            (heightSkip > heightIn && !(heightSkipPrev < heightSkip - 2 && heightSkipPrev >= heightIn))) {
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            pindexWalk = pindexWalk->pskip;
            heightWalk = heightSkip;
        } else {
            pindexWalk = pindexWalk->pprev;
            heightWalk--;
        }
    }
    return pindexWalk;
}

const CBlockIndex *CBlockIndex::GetAncestor(int32_t heightIn) const {
    return const_cast<CBlockIndex *>(this)->GetAncestor(heightIn);
}

void CBlockIndex::BuildSkip() {
    if (pprev)
        pskip = pprev->GetAncestor(GetSkipHeight(height));
}

void PushGetBlocks(CNode *pNode, CBlockIndex *pIndexBegin, uint256 hashEnd) {
    // Ask this guy to fill in what we're missing
    AssertLockHeld(cs_main);
    // Filter out duplicate requests
    if (pIndexBegin == pNode->pIndexLastGetBlocksBegin && hashEnd == pNode->hashLastGetBlocksEnd) {
        LogPrint(BCLog::NET, "filter the same GetLocator from peer %s\n", pNode->addr.ToString());
        return;
    }
    pNode->pIndexLastGetBlocksBegin = pIndexBegin;
    pNode->hashLastGetBlocksEnd     = hashEnd;
    CBlockLocator blockLocator      = chainActive.GetLocator(pIndexBegin);
    pNode->PushMessage(NetMsgType::GETBLOCKS, blockLocator, hashEnd);
    LogPrint(BCLog::NET, "getblocks from peer %s, hashEnd:%s\n", pNode->addr.ToString(), hashEnd.GetHex());
}

void PushGetBlocksOnCondition(CNode *pNode, CBlockIndex *pIndexBegin, uint256 hashEnd) {
    // Ask this guy to fill in what we're missing
    AssertLockHeld(cs_main);
    // Filter out duplicate requests
    if (pIndexBegin == pNode->pIndexLastGetBlocksBegin && hashEnd == pNode->hashLastGetBlocksEnd) {
        LogPrint(BCLog::NET, "filter the same GetLocator from peer %s\n", pNode->addr.ToString());
        static CBloomFilter filter(5000, 0.0001, 0, BLOOM_UPDATE_NONE);
        static uint32_t count = 0;
        string key            = to_string(pNode->id) + ":" + to_string((GetTime() / 2));
        if (!filter.contains(vector<uint8_t>(key.begin(), key.end()))) {
            filter.insert(vector<uint8_t>(key.begin(), key.end()));
            ++count;
            pNode->pIndexLastGetBlocksBegin = pIndexBegin;
            pNode->hashLastGetBlocksEnd     = hashEnd;
            CBlockLocator blockLocator      = chainActive.GetLocator(pIndexBegin);
            pNode->PushMessage(NetMsgType::GETBLOCKS, blockLocator, hashEnd);
            LogPrint(BCLog::NET, "getblocks from peer %s, hashEnd:%s\n", pNode->addr.ToString(), hashEnd.GetHex());
        } else {
            if (count >= 5000) {
                count = 0;
                filter.Clear();
            }
        }
    } else {
        pNode->pIndexLastGetBlocksBegin = pIndexBegin;
        pNode->hashLastGetBlocksEnd     = hashEnd;
        CBlockLocator blockLocator      = chainActive.GetLocator(pIndexBegin);
        pNode->PushMessage(NetMsgType::GETBLOCKS, blockLocator, hashEnd);
        LogPrint(BCLog::NET, "getblocks from peer %s, hashEnd:%s\n", pNode->addr.ToString(), hashEnd.GetHex());
    }
}

bool ProcessBlock(CValidationState &state, CNode *pFrom, CBlock *pBlock, CDiskBlockPos *dbp) {
    int64_t llBeginTime = GetTimeMillis();
    // LogPrint(BCLog::INFO, "ProcessBlock() enter:%lld\n", llBeginTime);
    AssertLockHeld(cs_main);
    // Check for duplicate
    uint256 blockHash    = pBlock->GetHash();
    uint32_t blockHeight = pBlock->GetHeight();
    if (mapBlockIndex.count(blockHash))
        return state.Invalid(ERRORMSG("ProcessBlock() : block [%u]: %s exists", blockHeight, blockHash.ToString()), 0,
                             "duplicate");


    if (mapOrphanBlocks.count(blockHash))
        return state.Invalid(
            ERRORMSG("ProcessBlock() : block (orphan) [%u]: %s exists", blockHeight, blockHash.ToString()), 0,
            "duplicate");

    int64_t llBeginCheckBlockTime = GetTimeMillis();
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);

    // Preliminary checks
    if (!CheckBlock(*pBlock, state, *spCW, false)) {
        LogPrint(BCLog::INFO, "CheckBlock() height: %d elapse time:%lld ms\n", chainActive.Height(),
                 GetTimeMillis() - llBeginCheckBlockTime);

        return ERRORMSG("ProcessBlock() : block hash:%s CheckBlock FAILED", pBlock->GetHash().GetHex());
    }

    // If we don't already have its previous block, shunt it off to holding area until we get it
    if (!pBlock->GetPrevBlockHash().IsNull() && !mapBlockIndex.count(pBlock->GetPrevBlockHash())) {
        if (pBlock->GetHeight() > (uint32_t)nSyncTipHeight) {
            LogPrint(BCLog::DEBUG, "blockHeight=%d syncTipHeight=%d\n", pBlock->GetHeight(), nSyncTipHeight );
            nSyncTipHeight = pBlock->GetHeight();
        }

        // Accept orphans as long as there is a node to request its parents from
        if (pFrom) {
            bool success = PruneOrphanBlocks(pBlock->GetHeight());
            if (success) {
                COrphanBlock *pblock2 = new COrphanBlock();
                CDataStream ss(SER_DISK, CLIENT_VERSION);
                ss << *pBlock;
                pblock2->vchBlock      = vector<uint8_t>(ss.begin(), ss.end());
                pblock2->blockHash     = blockHash;
                pblock2->prevBlockHash = pBlock->GetPrevBlockHash();
                pblock2->height        = pBlock->GetHeight();
                mapOrphanBlocks.insert(make_pair(blockHash, pblock2));
                mapOrphanBlocksByPrev.insert(make_pair(pblock2->prevBlockHash, pblock2));
                setOrphanBlock.insert(pblock2);
            }

            // Ask this guy to fill in what we're missing
            LogPrint(BCLog::NET,
                     "receive an orphan block height=%d hash=%s, %s it, leading to getblocks (current block height=%d, "
                     "current block hash=%s, orphan blocks=%d)\n",
                     pBlock->GetHeight(), pBlock->GetHash().GetHex(), success ? "keep" : "abandon",
                     chainActive.Height(), chainActive.Tip()->GetBlockHash().GetHex(), mapOrphanBlocksByPrev.size());

            PushGetBlocksOnCondition(pFrom, chainActive.Tip(), GetOrphanRoot(blockHash));
        }
        return true;
    }

    int64_t llAcceptBlockTime = GetTimeMillis();

    bool mining = (pFrom)?false:true;
    // Store to disk
    if (!AcceptBlock(*pBlock, state, dbp, mining)) {
        LogPrint(BCLog::INFO, "AcceptBlock() elapse time: %lld ms\n", GetTimeMillis() - llAcceptBlockTime);
        return ERRORMSG("ProcessBlock() : AcceptBlock FAILED");
    }
    // LogPrint(BCLog::INFO, "AcceptBlock() elapse time:%lld ms\n", GetTimeMillis() - llAcceptBlockTime);

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(blockHash);
    for (uint32_t i = 0; i < vWorkQueue.size(); i++) {
        uint256 prevBlockHash = vWorkQueue[i];
        for (multimap<uint256, COrphanBlock *>::iterator mi = mapOrphanBlocksByPrev.lower_bound(prevBlockHash);
             mi != mapOrphanBlocksByPrev.upper_bound(prevBlockHash); ++mi) {
            CBlock block;
            {
                CDataStream ss(mi->second->vchBlock, SER_DISK, CLIENT_VERSION);
                ss >> block;
            }
            block.BuildMerkleTree();
            /**
             * Use a dummy CValidationState so someone can't setup nodes to counter-DoS based on orphan resolution
             * (that is, feeding people an invalid block based on LegitBlockX in order to get anyone relaying LegitBlockX banned)
             */
            CValidationState stateDummy;
            if (AcceptBlock(block, stateDummy)) {
                vWorkQueue.push_back(mi->second->blockHash);
            }
            setOrphanBlock.erase(mi->second);
            mapOrphanBlocks.erase(mi->second->blockHash);
            delete mi->second;
        }
        mapOrphanBlocksByPrev.erase(prevBlockHash);
    }

    LogPrint(BCLog::INFO, "ProcessBlock[%d] elapse time:%lld ms\n", pBlock->GetHeight(), GetTimeMillis() - llBeginTime);
    return true;
}

bool AbortNode(const string &strMessage) {
    strMiscWarning = strMessage;
    LogPrint(BCLog::ERROR, "Detect abort ERROR! *** %s\n", strMessage);
    StartShutdown();

    return false;
}

bool CheckDiskSpace(uint64_t nAdditionalBytes) {
    uint64_t nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

    // Check for mininum disk space bytes (currently 50MB)
    if (nFreeBytesAvailable < MIN_DISK_SPACE + nAdditionalBytes)
        return AbortNode(_("Error: Disk space is low!"));

    return true;
}

bool static LoadBlockIndexDB() {
    if (!pCdMan->pBlockIndexDb->LoadBlockIndexes())
        return ERRORMSG("%s(), LoadBlockIndexes from db failed", __FUNCTION__);

    boost::this_thread::interruption_point();

    // Calculate nChainWork
    vector<pair<int32_t, CBlockIndex *> > vSortedByHeight;
    vSortedByHeight.reserve(mapBlockIndex.size());
    for (const auto &item : mapBlockIndex) {
        CBlockIndex *pIndex = item.second;
        vSortedByHeight.push_back(make_pair(pIndex->height, pIndex));
    }
    sort(vSortedByHeight.begin(), vSortedByHeight.end());
    for (const auto &item : vSortedByHeight) {
        CBlockIndex *pIndex = item.second;
        pIndex->nChainWork  = pIndex->height;
        pIndex->nChainTx    = (pIndex->pprev ? pIndex->pprev->nChainTx : 0) + pIndex->nTx;
        if ((pIndex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS && !(pIndex->nStatus & BLOCK_FAILED_MASK))
            setBlockIndexValid.insert(pIndex);
        if (pIndex->nStatus & BLOCK_FAILED_MASK &&
            (!pIndexBestInvalid || pIndex->nChainWork > pIndexBestInvalid->nChainWork))
            pIndexBestInvalid = pIndex;
        if (pIndex->pprev)
            pIndex->BuildSkip();
    }

    // Load block file info
    pCdMan->pBlockCache->ReadLastBlockFile(nLastBlockFile);
    LogPrint(BCLog::INFO, "LoadBlockIndexDB(): last block file = %i\n", nLastBlockFile);
    if (pCdMan->pBlockIndexDb->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile))
    LogPrint(BCLog::INFO, "LoadBlockIndexDB(): last block file info: %s\n", infoLastBlockFile.ToString());

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    pCdMan->pBlockCache->ReadReindexing(fReindexing);

    bool fCurReindex = SysCfg().IsReindex();
    SysCfg().SetReIndex(fCurReindex |= fReindexing);

    // Check whether we have a transaction index
    bool bTxIndex = SysCfg().IsTxIndex();
    pCdMan->pBlockCache->ReadFlag("txindex", bTxIndex);
    SysCfg().SetTxIndex(bTxIndex);
    LogPrint(BCLog::INFO, "LoadBlockIndexDB(): transaction index %s\n", bTxIndex ? "enabled" : "disabled");

    // Load pointer to end of best chain
    uint256 bestBlockHash = pCdMan->pBlockCache->GetBestBlockHash();
    const auto &it = mapBlockIndex.find(bestBlockHash);
    if (it == mapBlockIndex.end()) {
        return ERRORMSG("%s(), the best block hash in db not found in block index! hash=%s\n",
            __FUNCTION__, bestBlockHash.ToString());
    }

    chainActive.SetTip(it->second);
  //  chainActive.UpdateFinalityBlock();
    LogPrint(BCLog::INFO, "LoadBlockIndexDB(): hashBestChain=%s height=%d date=%s\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()));

    return true;
}

bool VerifyDB(int32_t nCheckLevel, int32_t nCheckDepth) {
    LOCK(cs_main);
    if (chainActive.Tip() == nullptr || chainActive.Tip()->pprev == nullptr)
        return true;

    // Verify blocks in the best chain
    if (nCheckDepth <= 0)
        nCheckDepth = 1000000000;  // suffices until the year 19000

    if (nCheckDepth > chainActive.Height())
        nCheckDepth = chainActive.Height();

    nCheckLevel = max(0, min(4, nCheckLevel));
    LogPrint(BCLog::INFO, "Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);

    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);

    CBlockIndex *pIndexState   = chainActive.Tip();
    CBlockIndex *pIndexFailure = nullptr;
    int32_t nGoodTransactions  = 0;
    CValidationState state;

    for (CBlockIndex *pIndex = chainActive.Tip(); pIndex && pIndex->pprev; pIndex = pIndex->pprev) {
        boost::this_thread::interruption_point();
        if (pIndex->height < chainActive.Height() - nCheckDepth)
            break;

        CBlock block;
        // check level 0: read from disk
        if (!ReadBlockFromDisk(pIndex, block))
            return ERRORMSG("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s",
                            pIndex->height, pIndex->GetBlockHash().ToString());

        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !CheckBlock(block, state, *spCW, false))
            return ERRORMSG("VerifyDB() : *** found bad block at %d, hash=%s\n",
                            pIndex->height, pIndex->GetBlockHash().ToString());

        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pIndex) {
            CBlockUndo undo;
            CDiskBlockPos pos = pIndex->GetUndoPos();
            if (!pos.IsNull()) {
                if (!undo.ReadFromDisk(pos, pIndex->pprev->GetBlockHash()))
                    return ERRORMSG("VerifyDB() : *** found bad undo data at %d, hash=%s\n",
                                    pIndex->height, pIndex->GetBlockHash().ToString());
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        if (nCheckLevel >= 3 && pIndex == pIndexState) {
            bool fClean = true;
            if (!DisconnectBlock(block, *spCW, pIndex, state, &fClean))
                return ERRORMSG("VerifyDB() : *** irrecoverable inconsistency in block data at %d, hash=%s",
                                pIndex->height, pIndex->GetBlockHash().ToString());

            pIndexState = pIndex->pprev;
            if (!fClean) {
                nGoodTransactions = 0;
                pIndexFailure     = pIndex;
            } else {
                nGoodTransactions += block.vptx.size();
            }
        }
    }
    if (pIndexFailure)
        return ERRORMSG("VerifyDB() : *** coin database inconsistencies found (last %i blocks, %i good transactions before that)\n",
                        chainActive.Height() - pIndexFailure->height + 1, nGoodTransactions);

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4) {
        CBlockIndex *pIndex = pIndexState;
        while (pIndex != chainActive.Tip()) {
            boost::this_thread::interruption_point();
            pIndex = chainActive.Next(pIndex);
            CBlock block;
            if (!ReadBlockFromDisk(pIndex, block))
                return ERRORMSG("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s",
                                pIndex->height, pIndex->GetBlockHash().ToString());

            if (!ConnectBlock(block, *spCW, pIndex, state, false))
                return ERRORMSG("VerifyDB() : *** found un-connectable block at %d, hash=%s",
                                pIndex->height, pIndex->GetBlockHash().ToString());
        }
    }

    LogPrint(BCLog::INFO, "No coin database inconsistencies in last %i blocks (%i transactions)\n",
            chainActive.Height() - pIndexState->height, nGoodTransactions);

    return true;
}

void UnloadBlockIndex() {
    mapBlockIndex.clear();
    setBlockIndexValid.clear();
    chainActive.SetTip(nullptr);
    pIndexBestInvalid = nullptr;
}

bool LoadBlockIndex() {
    // Load block index from databases
    if (!SysCfg().IsReindex() && !LoadBlockIndexDB())
        return false;

    return true;
}

bool InitBlockIndex() {
    LOCK(cs_main);
    // Check whether we're already initialized
    if (chainActive.Genesis() != nullptr)
        return true;

    // Use the provided setting for -txindex in the new database
    SysCfg().SetTxIndex(SysCfg().GetBoolArg("-txindex", true));
    pCdMan->pBlockCache->WriteFlag("txindex", SysCfg().IsTxIndex());
    LogPrint(BCLog::INFO, "Initializing databases...\n");

    // Only add the genesis block if not reindexing (in which case we reuse the one already on disk)
    if (!SysCfg().IsReindex()) {
        try {
            CBlock &block = const_cast<CBlock &>(SysCfg().GenesisBlock());
            // Start new block file
            uint32_t nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
            CDiskBlockPos blockPos;
            CValidationState state;
            if (!FindBlockPos(state, blockPos, nBlockSize + 8, 0, block.GetTime()))
                return ERRORMSG("InitBlockIndex() : FindBlockPos failed");

            if (!WriteBlockToDisk(block, blockPos))
                return ERRORMSG("InitBlockIndex() : writing genesis block to disk failed");

            if (!AddToBlockIndex(block, state, blockPos))
                return ERRORMSG("InitBlockIndex() : genesis block not accepted");

        } catch (runtime_error &e) {
            return ERRORMSG("InitBlockIndex() : failed to initialize block database: %s", e.what());
        }
    }

    return true;
}

void PrintBlockTree() {
    AssertLockHeld(cs_main);
    // pre-compute tree structure
    map<CBlockIndex *, vector<CBlockIndex *> > mapNext;
    for (map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi) {
        CBlockIndex *pIndex = (*mi).second;
        mapNext[pIndex->pprev].push_back(pIndex);
    }

    vector<pair<int32_t, CBlockIndex *> > vStack;
    vStack.push_back(make_pair(0, chainActive.Genesis()));

    int32_t nPrevCol = 0;
    while (!vStack.empty()) {
        int32_t nCol            = vStack.back().first;
        CBlockIndex *pIndex = vStack.back().second;
        vStack.pop_back();

        // print split or gap
        if (nCol > nPrevCol) {
            for (int32_t i = 0; i < nCol - 1; i++)
                LogPrint(BCLog::INFO, "| ");
            LogPrint(BCLog::INFO, "|\\\n");
        } else if (nCol < nPrevCol) {
            for (int32_t i = 0; i < nCol; i++)
                LogPrint(BCLog::INFO, "| ");
            LogPrint(BCLog::INFO, "|\n");
        }
        nPrevCol = nCol;

        // print columns
        for (int32_t i = 0; i < nCol; i++)
            LogPrint(BCLog::INFO, "| ");

        // print item
        CBlock block;
        ReadBlockFromDisk(pIndex, block);
        LogPrint(BCLog::INFO, "%d (blk%05u.dat:0x%x)  %s  tx %u\n",
                 pIndex->height,
                 pIndex->GetBlockPos().nFile, pIndex->GetBlockPos().nPos,
                 DateTimeStrFormat("%Y-%m-%d %H:%M:%S", block.GetBlockTime()),
                 block.vptx.size());

        // put the main time-chain first
        vector<CBlockIndex *> &vNext = mapNext[pIndex];
        for (uint32_t i = 0; i < vNext.size(); i++) {
            if (chainActive.Next(vNext[i])) {
                swap(vNext[0], vNext[i]);
                break;
            }
        }

        // iterate children
        for (uint32_t i = 0; i < vNext.size(); i++)
            vStack.push_back(make_pair(nCol + i, vNext[i]));
    }
}

bool LoadExternalBlockFile(FILE *fileIn, CDiskBlockPos *dbp) {
    int64_t nStart = GetTimeMillis();
    int32_t nLoaded    = 0;
    try {
        CBufferedFile blkdat(fileIn, 2 * MAX_BLOCK_SIZE, MAX_BLOCK_SIZE + 8, SER_DISK, CLIENT_VERSION);
        uint64_t nStartByte = 0;
        if (dbp) {
            // (try to) skip already indexed part
            CBlockFileInfo info;
            if (pCdMan->pBlockIndexDb->ReadBlockFileInfo(dbp->nFile, info)) {
                nStartByte = info.nSize;
                blkdat.Seek(info.nSize);
            }
        }
        uint64_t nRewind = blkdat.GetPos();
        while (blkdat.good() && !blkdat.eof()) {
            boost::this_thread::interruption_point();

            blkdat.SetPos(nRewind);
            nRewind++;          // start one byte further next time, in case of failure
            blkdat.SetLimit();  // remove former limit
            uint32_t nSize = 0;
            try {
                // locate a header
                uint8_t buf[MESSAGE_START_SIZE];
                blkdat.FindByte(SysCfg().MessageStart()[0]);
                nRewind = blkdat.GetPos() + 1;
                blkdat >> FLATDATA(buf);
                if (memcmp(buf, SysCfg().MessageStart(), MESSAGE_START_SIZE))
                    continue;
                // read size
                blkdat >> nSize;
                if (nSize < 80 || nSize > MAX_BLOCK_SIZE)
                    continue;
            } catch (std::exception &e) {
                // no valid block header found; don't complain
                break;
            }
            try {
                // read block
                uint64_t nBlockPos = blkdat.GetPos();
                blkdat.SetLimit(nBlockPos + nSize);
                CBlock block;
                blkdat >> block;
                nRewind = blkdat.GetPos();

                // process block
                if (nBlockPos >= nStartByte) {
                    LOCK(cs_main);
                    if (dbp)
                        dbp->nPos = nBlockPos;
                    CValidationState state;
                    if (ProcessBlock(state, nullptr, &block, dbp))
                        nLoaded++;
                    if (state.IsError())
                        break;
                }
            } catch (std::exception &e) {
                LogPrint(BCLog::INFO, "%s : Deserialize or I/O error - %s\n", __func__, e.what());
            }
        }
        fclose(fileIn);
    } catch (runtime_error &e) {
        AbortNode(_("Error: system error: ") + e.what());
    }
    if (nLoaded > 0)
        LogPrint(BCLog::INFO, "Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
    return nLoaded > 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// CAlert
//

string GetWarnings(string strFor) {
    int32_t nPriority = 0;
    string strStatusBar;
    string strRPC;

    if (SysCfg().GetBoolArg("-testsafemode", false))
        strRPC = "test";

    if (!CLIENT_VERSION_IS_RELEASE)
        strStatusBar = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "") {
        nPriority    = 1000;
        strStatusBar = strMiscWarning;
    }

    if (fLargeWorkForkFound) {
        nPriority    = 2000;
        strStatusBar = strRPC = _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    } else if (fLargeWorkInvalidChainFound) {
        nPriority    = 2000;
        strStatusBar = strRPC = _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.");
    }

    // Alerts
    {
        LOCK(cs_mapAlerts);
        for (auto &item : mapAlerts) {
            const CAlert &alert = item.second;
            if (alert.AppliesToMe() && alert.nPriority > nPriority) {
                nPriority    = alert.nPriority;
                strStatusBar = alert.strStatusBar;
            }
        }
    }

    if (strFor == "statusbar")
        return strStatusBar;
    else if (strFor == "rpc")
        return strRPC;
    assert(!"GetWarnings() : invalid parameter");
    return "error";
}


class CMainCleanup {
   public:
    CMainCleanup() {}
    ~CMainCleanup() {
        // block headers
        map<uint256, CBlockIndex *>::iterator it1 = mapBlockIndex.begin();
        for (; it1 != mapBlockIndex.end(); it1++)
            delete (*it1).second;
        mapBlockIndex.clear();

        // orphan blocks
        map<uint256, COrphanBlock *>::iterator it2 = mapOrphanBlocks.begin();
        for (; it2 != mapOrphanBlocks.end(); it2++)
            delete (*it2).second;
        mapOrphanBlocks.clear();
        mapOrphanBlocksByPrev.clear();
        setOrphanBlock.clear();
    }
} instance_of_cmaincleanup;

bool DisconnectBlockFromTip(CValidationState &state) {
    return DisconnectTip(state);
}

bool EraseBlockIndexFromSet(CBlockIndex *pIndex) {
    AssertLockHeld(cs_main);
    return setBlockIndexValid.erase(pIndex) > 0;
}

bool IsInitialBlockDownload() {
    LOCK(cs_main);
    if (SysCfg().IsImporting() ||
        SysCfg().IsReindex())
        return true;

    static int64_t nLastUpdate;
    static CBlockIndex *pIndexLastBest;
    if (chainActive.Tip() != pIndexLastBest) {
        pIndexLastBest = chainActive.Tip();
        nLastUpdate    = GetTime();
    }

    return (GetTime() - nLastUpdate < 10 && chainActive.Tip()->GetBlockTime() < GetTime() - 24 * 60 * 60);
}
