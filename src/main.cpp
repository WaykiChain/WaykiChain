// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "main.h"

#include "entities/id.h"
#include "addrman.h"
#include "alert.h"
#include "config/chainparams.h"
#include "config/configuration.h"
#include "config/scoin.h"
#include "init.h"
#include "miner/miner.h"
#include "net.h"
#include "tx/merkletx.h"
#include "commons/util.h"
#include "vm/luavm/vmrunenv.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "p2p/chainmessage.h"

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
map<uint256, std::shared_ptr<CCacheWrapper> > mapForkCache;
CSignatureCache signatureCache;
CChain chainActive;
CChain chainMostWork;

/** Fees smaller than this (in sawi) are considered zero fee (for transaction creation) */
uint64_t CBaseTx::nMinTxFee = 10000;  // Override with -mintxfee
/** Fees smaller than this (in sawi) are considered zero fee (for relaying and mining) */
uint64_t CBaseTx::nMinRelayTxFee = 1000;
/** Amount smaller than this (in sawi) is considered dust amount */
uint64_t CBaseTx::nDustAmountThreshold = 10000;
/** Amount of blocks that other nodes claim to have */



map<uint256, COrphanBlock *> mapOrphanBlocks;
multimap<uint256, COrphanBlock *> mapOrphanBlocksByPrev;

map<uint256, std::shared_ptr<CBaseTx> > mapOrphanTransactions;

const string strMessageMagic = "Coin Signed Message:\n";

// Internal stuff
namespace {


    void InitializeNode(NodeId nodeid, const CNode *pNode) {
        LOCK(cs_main);
        CNodeState &state = mapNodeState.insert(make_pair(nodeid, CNodeState())).first->second;
        state.name        = pNode->addrName;
    }

    int32_t GetHeight() {
        LOCK(cs_main);
        return chainActive.Height();
    }

    void FinalizeNode(NodeId nodeid) {
        LOCK(cs_main);
        CNodeState *state = State(nodeid);

        for (const auto &entry : state->vBlocksInFlight)
            mapBlocksInFlight.erase(entry.hash);
        for (const auto &hash : state->vBlocksToDownload)
            mapBlocksToDownload.erase(hash);

        mapNodeState.erase(nodeid);
    }

    // Requires cs_main.
    void MarkBlockAsInFlight(NodeId nodeid, const uint256 &hash) {
        CNodeState *state = State(nodeid);
        assert(state != nullptr);

        // Make sure it's not listed somewhere already.
        MarkBlockAsReceived(hash);

        QueuedBlock newentry = {hash, GetTimeMicros(), state->nBlocksInFlight};
        if (state->nBlocksInFlight == 0)
            state->nLastBlockReceive = newentry.nTime;  // Reset when a first request is sent.
        list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(), newentry);
        state->nBlocksInFlight++;
        mapBlocksInFlight[hash] = make_pair(nodeid, it);
    }

    struct CBlockIndexWorkComparator {
    bool operator()(CBlockIndex *pa, CBlockIndex *pb) {

        // First sort by most total work, ...
        if(pa->nChainWork != pb->nChainWork){
            return (pa->nChainWork < pb->nChainWork) ;
        }


        // ... then by earliest time received, ...
        if(pa->nSequenceId != pb->nSequenceId){
            return (pa->nSequenceId > pb->nSequenceId) ;
        }

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those all have id 0).

        return pa > pb ;

    }
};

CBlockIndex *pindexBestInvalid;
// may contain all CBlockIndex*'s that have validness >=BLOCK_VALID_TRANSACTIONS, and must contain those who aren't
// failed
set<CBlockIndex *, CBlockIndexWorkComparator> setBlockIndexValid;  //根据高度排序的有序集合

struct COrphanBlockComparator {
    bool operator()(COrphanBlock *pa, COrphanBlock *pb) {
        if (pa->height > pb->height)
            return false;
        if (pa->height < pb->height)
            return true;
        return false;
    }
};
set<COrphanBlock *, COrphanBlockComparator> setOrphanBlock;  //存的孤立块

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

namespace {
struct CMainSignals {
    // Notifies listeners of updated transaction data (passing hash, transaction, and optionally the block it is found
    // in.
    boost::signals2::signal<void(const uint256 &, CBaseTx *, const CBlock *)> SyncTransaction;
    // Notifies listeners of an erased transaction (currently disabled, requires transaction replacement).
    boost::signals2::signal<void(const uint256 &)> EraseTransaction;
    // Notifies listeners of a new active block chain.
    boost::signals2::signal<void(const CBlockLocator &)> SetBestChain;
    // Notifies listeners about an inventory item being seen on the network.
    // boost::signals2::signal<void (const uint256 &)> Inventory;
    // Tells listeners to broadcast their data.
    boost::signals2::signal<void()> Broadcast;
} g_signals;
}  // namespace

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

void EraseTransaction(const uint256 &hash) { g_signals.EraseTransaction(hash); }

//////////////////////////////////////////////////////////////////////////////
//
// Registration of network node signals.
//


bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats) {
    LOCK(cs_main);
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

//////////////////////////////////////////////////////////////////////////////
//
// CChain implementation
//

CBlockIndex *CChain::SetTip(CBlockIndex *pIndex) {
    if (pIndex == nullptr) {
        vChain.clear();
        return nullptr;
    }
    vChain.resize(pIndex->height + 1);
    while (pIndex && vChain[pIndex->height] != pIndex) {
        vChain[pIndex->height] = pIndex;
        pIndex                  = pIndex->pprev;
    }
    return pIndex;
}

CBlockLocator CChain::GetLocator(const CBlockIndex *pIndex) const {
    int32_t nStep = 1;
    vector<uint256> vHave;
    vHave.reserve(32);

    if (!pIndex)
        pIndex = Tip();
    while (pIndex) {
        vHave.push_back(pIndex->GetBlockHash());
        // Stop when we have added the genesis block.
        if (pIndex->height == 0)
            break;
        // Exponentially larger steps back, plus the genesis block.
        int32_t height = max(pIndex->height - nStep, 0);
        // Jump back quickly to the same height as the chain.
        if (pIndex->height > height)
            pIndex = pIndex->GetAncestor(height);
        // In case pIndex is not in this chain, iterate pIndex->pprev to find blocks.
        while (!Contains(pIndex))
            pIndex = pIndex->pprev;
        // If pIndex is in this chain, use direct height-based access.
        if (pIndex->height > height)
            pIndex = (*this)[height];
        if (vHave.size() > 10)
            nStep *= 2;
    }

    return CBlockLocator(vHave);
}

CBlockIndex *CChain::FindFork(const CBlockLocator &locator) const {
    // Find the first block the caller has in the main chain
    for (const auto &hash : locator.vHave) {
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hash);
        if (mi != mapBlockIndex.end()) {
            CBlockIndex *pIndex = (*mi).second;
            if (pIndex && Contains(pIndex))
                return pIndex;
        }
    }

    return Genesis();
}

uint32_t LimitOrphanTxSize(uint32_t nMaxOrphans) {
    uint32_t nEvicted = 0;
    while (mapOrphanTransactions.size() > nMaxOrphans) {
        // Evict a random orphan:
        uint256 randomhash                                   = GetRandHash();
        map<uint256, std::shared_ptr<CBaseTx> >::iterator it = mapOrphanTransactions.lower_bound(randomhash);
        if (it == mapOrphanTransactions.end())
            it = mapOrphanTransactions.begin();
        mapOrphanTransactions.erase(it->first);
        ++nEvicted;
    }
    return nEvicted;
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

int64_t GetMinRelayFee(const CBaseTx *pBaseTx, uint32_t nBytes, bool fAllowFree) {
    uint64_t nBaseFee = pBaseTx->nMinRelayTxFee;
    int64_t nMinFee   = (1 + (int64_t)nBytes / 1000) * nBaseFee;

    if (fAllowFree) {
        // There is a free transaction area in blocks created by most miners,
        // * If we are relaying we allow transactions up to DEFAULT_BLOCK_PRIORITY_SIZE - 1000
        //   to be considered to fall into this category. We don't want to encourage sending
        //   multiple transactions instead of one big transaction to avoid fees.
        if (nBytes < (DEFAULT_BLOCK_PRIORITY_SIZE - 1000))
            nMinFee = 0;
    }

    if (!CheckBaseCoinRange(nMinFee))
        nMinFee = GetBaseCoinMaxMoney();

    return nMinFee;
}

bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx, bool fLimitFree,
                        bool fRejectInsaneFee) {
    AssertLockHeld(cs_main);

    // is it already in the memory pool?
    uint256 hash = pBaseTx->GetHash();
    if (pool.Exists(hash))
        return state.Invalid(ERRORMSG("AcceptToMemoryPool() : txid: %s already in mempool", hash.GetHex()),
            REJECT_INVALID, "tx-already-in-mempool");

    // is it a miner reward tx or median price tx?
    if (pBaseTx->IsBlockRewardTx() || pBaseTx->IsMedianPriceTx())
        return state.Invalid(ERRORMSG("AcceptToMemoryPool() : txid: %s is a miner reward transaction, can't put into mempool",
            hash.GetHex()), REJECT_INVALID, "tx-coinbase-to-mempool");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    string reason;
    if (SysCfg().NetworkID() == MAIN_NET && !IsStandardTx(pBaseTx, reason))
        return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : txid: %s is nonstandard transaction due to %s",
            hash.GetHex(), reason), REJECT_NONSTANDARD, reason);

    auto spCW = std::make_shared<CCacheWrapper>(*mempool.cw);

    if (!pBaseTx->CheckTx(chainActive.Height(), *spCW, state))
        return ERRORMSG("AcceptToMemoryPool() : CheckTx failed, txid: %s", hash.GetHex());

    CTxMemPoolEntry entry(pBaseTx, GetTime(), chainActive.Height());
    // TODO: consider different coin types.
    auto nFees = std::get<1>(entry.GetFees());
    auto nSize = entry.GetTxSize();

    if (pBaseTx->nTxType == BCOIN_TRANSFER_TX) {
        CBaseCoinTransferTx *pTx = static_cast<CBaseCoinTransferTx *>(pBaseTx);
        if (pTx->coin_amount < CBaseTx::nDustAmountThreshold)
            return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : txid: %s transfer dust amount, %d < %d",
                 hash.GetHex(), pTx->coin_amount, CBaseTx::nDustAmountThreshold), REJECT_DUST, "dust amount");
    } else if (pBaseTx->nTxType == UCOIN_TRANSFER_TX) {
        CCoinTransferTx *pTx = static_cast<CCoinTransferTx *>(pBaseTx);
        if (pTx->coin_amount < CBaseTx::nDustAmountThreshold)
            return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : txid: %s transfer dust amount, %d < %d",
                 hash.GetHex(), pTx->coin_amount, CBaseTx::nDustAmountThreshold), REJECT_DUST, "dust amount");
    }

    uint64_t minFees = GetMinRelayFee(pBaseTx, nSize, true);
    if (fLimitFree && nFees < minFees)
        return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : txid: %s not pay enough fees, %d < %d",
            hash.GetHex(), nFees, minFees), REJECT_INSUFFICIENTFEE, "insufficient fee");

    // Continuously rate-limit free transactions
    // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
    // be annoying or make others' transactions take longer to confirm.
    if (fLimitFree && nFees < CBaseTx::nMinRelayTxFee) {
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

        LogPrint("INFO", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount + nSize);
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

int32_t GetTxConfirmHeight(const uint256 &hash, CContractDBCache &scriptDBCache) {
    if (SysCfg().IsTxIndex()) {
        CDiskTxPos diskTxPos;
        if (scriptDBCache.ReadTxIndex(hash, diskTxPos)) {
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
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CContractDBCache &scriptDBCache,
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
            if (scriptDBCache.ReadTxIndex(hash, diskTxPos)) {
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

//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//

bool WriteBlockToDisk(CBlock &block, CDiskBlockPos &pos) {
    // Open history file to append
    CAutoFile fileout = CAutoFile(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return ERRORMSG("WriteBlockToDisk : OpenBlockFile failed");

    // Write index header
    uint32_t nSize = fileout.GetSerializeSize(block);
    fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

    // Write block
    int32_t fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return ERRORMSG("WriteBlockToDisk : ftell failed");
    pos.nPos = (uint32_t)fileOutPos;
    fileout << block;

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool ReadBlockFromDisk(const CDiskBlockPos &pos, CBlock &block) {
    block.SetNull();

    // Open history file to read
    CAutoFile filein = CAutoFile(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (!filein)
        return ERRORMSG("ReadBlockFromDisk : OpenBlockFile failed");

    // Read block
    try {
        filein >> block;
    } catch (std::exception &e) {
        return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
    }

    // Check the header
    // if (!CheckProofOfWork(block.GetHash(), block.GetBits()))
    //     return ERRORMSG("ReadBlockFromDisk : Errors in block header");

    return true;
}

bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block) {
    if (!ReadBlockFromDisk(pIndex->GetBlockPos(), block))
        return false;

    if (block.GetHash() != pIndex->GetBlockHash())
        return ERRORMSG("ReadBlockFromDisk(CBlock&, CBlockIndex*) : GetHash() doesn't match");

    return true;
}

bool ReadBaseTxFromDisk(const CTxCord txCord, std::shared_ptr<CBaseTx> &pTx) {
    auto pBlock = std::make_shared<CBlock>();
    const CBlockIndex* pBlockIndex = chainActive[txCord.GetHeight()];
    if (pBlockIndex == nullptr) {
        return ERRORMSG("ReadBaseTxFromDisk error, the height(%d) is exceed current best block height", txCord.GetHeight());
    }
    if (!ReadBlockFromDisk(pBlockIndex, *pBlock)) {
        return ERRORMSG("ReadBaseTxFromDisk error, read the block at height(%d) failed!", txCord.GetHeight());
    }
    if (txCord.GetIndex() >= pBlock->vptx.size()) {
        return ERRORMSG("ReadBaseTxFromDisk error, the tx(%s) index exceed the tx count of block", txCord.ToString());
    }
    pTx = pBlock->vptx.at(txCord.GetIndex())->GetNewInstance();
    return true;
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
        (pindexBestInvalid &&
         pindexBestInvalid->nChainWork > chainActive.Tip()->nChainWork + (GetBlockProof(*chainActive.Tip()) * 6))) {
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
            LogPrint("INFO",
                     "CheckForkWarningConditions: Warning: Large valid fork found\n  forking the chain at height %d "
                     "(%s)\n  lasting to height %d (%s).\nChain state database corruption likely.\n",
                     pIndexBestForkBase->height, pIndexBestForkBase->pBlockHash->ToString(),
                     pIndexBestForkTip->height, pIndexBestForkTip->pBlockHash->ToString());
            fLargeWorkForkFound = true;
        } else {
            LogPrint("INFO",
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

// Requires cs_main.
void Misbehaving(NodeId pNode, int32_t howmuch) {
    if (howmuch == 0)
        return;

    CNodeState *state = State(pNode);
    if (state == nullptr)
        return;

    state->nMisbehavior += howmuch;
    if (state->nMisbehavior >= SysCfg().GetArg("-banscore", 100)) {
        LogPrint("INFO", "Misbehaving: %s (%d -> %d) BAN THRESHOLD EXCEEDED\n", state->name,
                 state->nMisbehavior - howmuch, state->nMisbehavior);
        state->fShouldBan = true;
    } else {
        LogPrint("INFO", "Misbehaving: %s (%d -> %d)\n", state->name, state->nMisbehavior - howmuch,
                 state->nMisbehavior);
    }
}

void static InvalidChainFound(CBlockIndex *pIndexNew) {
    if (!pindexBestInvalid || pIndexNew->nChainWork > pindexBestInvalid->nChainWork) {
        pindexBestInvalid = pIndexNew;
        // The current code doesn't actually read the BestInvalidWork entry in
        // the block database anymore, as it is derived from the flags in block
        // index entry. We only write it for backward compatibility.
        // TODO: need to remove the indexBestInvalid
        //pCdMan->pBlockTreeDb->WriteBestInvalidWork(ArithToUint256(pindexBestInvalid->nChainWork));
    }
    LogPrint("INFO", "InvalidChainFound: invalid block=%s  height=%d  log2_work=%.8g  date=%s\n",
             pIndexNew->GetBlockHash().ToString(), pIndexNew->height,
             log(pIndexNew->nChainWork.getdouble()) / log(2.0),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pIndexNew->GetBlockTime()));
    LogPrint("INFO", "InvalidChainFound:  current best=%s  height=%d  log2_work=%.8g  date=%s\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
             log(chainActive.Tip()->nChainWork.getdouble()) / log(2.0),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()));
    CheckForkWarningConditions();
}

void static InvalidBlockFound(CBlockIndex *pIndex, const CValidationState &state) {
    int32_t nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        map<uint256, NodeId>::iterator it = mapBlockSource.find(pIndex->GetBlockHash());
        if (it != mapBlockSource.end() && State(it->second)) {
            CBlockReject reject = {state.GetRejectCode(), state.GetRejectReason(), pIndex->GetBlockHash()};
            State(it->second)->rejects.push_back(reject);
            if (nDoS > 0) {
                LogPrint("INFO", "Misebehaving: found invalid block, hash:%s, Misbehavior add %d\n", it->first.GetHex(),
                         nDoS);
                Misbehaving(it->second, nDoS);
            }
        }
    }

    if (!state.CorruptionPossible()) {
        pIndex->nStatus |= BLOCK_FAILED_VALID;
        pCdMan->pBlockTreeDb->WriteBlockIndex(CDiskBlockIndex(pIndex));
        setBlockIndexValid.erase(pIndex);
        InvalidChainFound(pIndex);
    }
}

bool InvalidateBlock(CValidationState &state, CBlockIndex *pIndex) {
    AssertLockHeld(cs_main);

    // Mark the block itself as invalid.
    pIndex->nStatus |= BLOCK_FAILED_VALID;
    pCdMan->pBlockTreeDb->WriteBlockIndex(CDiskBlockIndex(pIndex));
    setBlockIndexValid.erase(pIndex);

    LogPrint("INFO", "Invalidate block[%d]: %s BLOCK_FAILED_VALID\n", pIndex->height,
             pIndex->GetBlockHash().ToString());

    while (chainActive.Contains(pIndex)) {
        CBlockIndex *pindexWalk = chainActive.Tip();
        pindexWalk->nStatus |= BLOCK_FAILED_CHILD;
        pCdMan->pBlockTreeDb->WriteBlockIndex(CDiskBlockIndex(pindexWalk));
        setBlockIndexValid.erase(pindexWalk);

        LogPrint("INFO", "Invalidate block[%d]: %s BLOCK_FAILED_CHILD\n", pindexWalk->height,
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
            pCdMan->pBlockTreeDb->WriteBlockIndex(CDiskBlockIndex(it->second));
            setBlockIndexValid.insert(it->second);
            if (it->second == pindexBestInvalid) {
                // Reset invalid block marker if it was pointing to one of those.
                pindexBestInvalid = nullptr;
            }
        }
        it++;
    }

    // Remove the invalidity flag from all ancestors too.
    while (pIndex != nullptr) {
        if (pIndex->nStatus & BLOCK_FAILED_MASK) {
            pIndex->nStatus &= ~BLOCK_FAILED_MASK;
            setBlockIndexValid.insert(pIndex);
            pCdMan->pBlockTreeDb->WriteBlockIndex(CDiskBlockIndex(pIndex));
        }
        pIndex = pIndex->pprev;
    }

    return true;
}

void UpdateTime(CBlockHeader &block, const CBlockIndex *pIndexPrev) {
    block.SetTime(max(pIndexPrev->GetMedianTimePast() + 1, GetAdjustedTime()));
}

bool DisconnectBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool *pfClean) {
    assert(pIndex->GetBlockHash() == cw.accountCache.GetBestBlock());

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
    if (!cw.UndoDatas(blockUndo)) {
        return ERRORMSG("DisconnectBlock() : Undo tx datas in block failed");
    }

    // Set previous block as the best block
    cw.accountCache.SetBestBlock(pIndex->pprev->GetBlockHash());

    // Delete the disconnected block's transactions from transaction memory cache.
    if (!cw.txCache.DeleteBlockFromCache(block)) {
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

        if (!cw.txCache.AddBlockToCache(reLoadblock)) {
            return state.Abort(_("DisconnectBlock() : failed to add block into transaction memory cache"));
        }
    }

    // Delete the disconnected block's pricefeed items from price point memory cache.
    if (!cw.ppCache.DeleteBlockFromCache(block)) {
        return state.Abort(_("DisconnectBlock() : failed to delete block from price point memory cache"));
    }

    // Load price points into price point memory cache.
    // TODO: parameterize 11.
    if (pIndex->height > 11) {
        CBlockIndex *pReLoadBlockIndex = pIndex;
        int32_t nCacheHeight           = 11;
        while (pReLoadBlockIndex && nCacheHeight-- > 0) {
            pReLoadBlockIndex = pReLoadBlockIndex->pprev;
        }

        CBlock reLoadblock;
        if (!ReadBlockFromDisk(pReLoadBlockIndex, reLoadblock)) {
            return state.Abort(_("DisconnectBlock() : failed to read block"));
        }

        if (!cw.ppCache.AddBlockToCache(reLoadblock)) {
            return state.Abort(_("DisconnectBlock() : failed to add block into price point memory cache"));
        }
    }

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
        if (!pCdMan->pBlockTreeDb->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
            return state.Abort(_("Failed to write block info"));
    } else {
        CBlockFileInfo info;
        if (!pCdMan->pBlockTreeDb->ReadBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to read block info"));
        pos.nPos = info.nUndoSize;
        nNewSize = (info.nUndoSize += nAddSize);
        if (!pCdMan->pBlockTreeDb->WriteBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to write block info"));
    }

    uint32_t nOldChunks = (pos.nPos + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    uint32_t nNewChunks = (nNewSize + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks) {
        if (CheckDiskSpace(nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos)) {
            FILE *file = OpenUndoFile(pos);
            if (file) {
                LogPrint("INFO", "Pre-allocating up to position 0x%x in rev%05u.dat\n", nNewChunks * UNDOFILE_CHUNK_SIZE, pos.nFile);
                AllocateFileRange(file, pos.nPos, nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos);
                fclose(file);
            }
        } else
            return state.Error("out of disk space");
    }

    return true;
}

static bool ProcessGenesisBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex) {
    cw.accountCache.SetBestBlock(pIndex->GetBlockHash());
    for (uint32_t i = 1; i < block.vptx.size(); i++) {
        if (block.vptx[i]->nTxType == BLOCK_REWARD_TX) {
            assert(i <= 1);
            CBlockRewardTx *pRewardTx = (CBlockRewardTx *)block.vptx[i].get();
            CAccount account;
            CRegID regId(pIndex->height, i);
            CPubKey pubKey       = pRewardTx->txUid.get<CPubKey>();
            CKeyID keyId         = pubKey.GetKeyId();
            account.nickid       = CNickID();
            account.keyid        = keyId;
            account.owner_pubkey = pubKey;
            account.regid        = regId;

            account.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, pRewardTx->reward);

            assert(cw.accountCache.SaveAccount(account));
        } else if (block.vptx[i]->nTxType == DELEGATE_VOTE_TX) {
            CDelegateVoteTx *pDelegateTx = (CDelegateVoteTx *)block.vptx[i].get();
            assert(pDelegateTx->txUid.type() == typeid(CRegID));  // Vote Tx must use RegId

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

                    if (votedUid.type() == typeid(CPubKey)) {
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
            assert(voterAcct.GetToken(SYMB::WICC).free_amount >= maxVotes);
            voterAcct.OperateBalance(SYMB::WICC, BalanceOpType::STAKE, maxVotes);
            cw.accountCache.SaveAccount(voterAcct);
            assert(cw.delegateCache.SetCandidateVotes(pDelegateTx->txUid.get<CRegID>(),
                                                      candidateVotes));
        }
    }

    return true;
}

bool SaveTxIndex(CBaseTx &baseTx, CCacheWrapper &cw, CValidationState &state, CDiskTxPos &diskTxPos) {
    if (SysCfg().IsTxIndex()) {
        // TODO: should move to blockTxCache?
        if (!cw.contractCache.SetTxIndex(baseTx.GetHash(), diskTxPos))
            return state.Abort(_("Failed to write transaction index"));
    }
    return true;
}

// compute vote staking interest && revoke votes
bool ComputeVoteStakingInterestAndRevokeVotes(const int32_t currHeight, CCacheWrapper &cw, CValidationState &state) {
    // acquire votes list
    map<string /* CRegID */, vector<CCandidateReceivedVote>> regId2ReceivedVotes;
    if (!cw.delegateCache.GetVoterList(regId2ReceivedVotes)) {
        return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : failed to get vote list"),
                         REJECT_INVALID, "bad-get-vote-list");
    }

    // revoke votes if necessary
    map<CRegID, vector<CCandidateVote>> regId2CandidateVotes;
    for (auto &item : regId2ReceivedVotes) {
        CRegID regId(UnsignedCharArray(item.first.begin(), item.first.end()));
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
        if (!account.ProcessDelegateVotes(candidateVotes, candidateVotesInOut, currHeight, cw.accountCache, receipts)) {
            return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : operate delegate vote failed, regId=%s",
                            regId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
        }
        if (!cw.delegateCache.SetCandidateVotes(regId, candidateVotesInOut)) {
            return state.DoS(100, ERRORMSG("ComputeVoteStakingInterestAndRevokeVotes() : write candidate votes failed, regId=%s",
                            regId.ToString()), WRITE_CANDIDATE_VOTES_FAIL, "write-candidate-votes-failed");
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

    return true;
}

bool ConnectBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool fJustCheck) {
    AssertLockHeld(cs_main);

    bool isGensisBlock = block.GetHeight() == 0 && block.GetHash() == SysCfg().GetGenesisBlockHash();

    // Check it again in case a previous version let a bad block in
    if (!isGensisBlock && !CheckBlock(block, state, cw, !fJustCheck, !fJustCheck))
        return false;

    if (!fJustCheck) {
        // Verify that the view's current state corresponds to the previous block
        uint256 hashPrevBlock = pIndex->pprev == nullptr ? uint256() : pIndex->pprev->GetBlockHash();
        if (hashPrevBlock != cw.accountCache.GetBestBlock()) {
            LogPrint("INFO", "hashPrevBlock=%s, bestblock=%s\n",
                    hashPrevBlock.GetHex(), cw.accountCache.GetBestBlock().GetHex());

            assert(hashPrevBlock == cw.accountCache.GetBestBlock());
        }
    }

    // Special case for the genesis block, skipping connection of its transactions.
    if (isGensisBlock) {
        return ProcessGenesisBlock(block, cw, pIndex);
    }

    // In stable coin genesis, need to verify txid for every transaction in block.
    if (block.GetHeight() == SysCfg().GetStableCoinGenesisHeight()) {
        assert(block.vptx.size() == 4);

        vector<string> txids = IniCfg().GetStableCoinGenesisTxid(SysCfg().NetworkID());
        assert(txids.size() == 3);
        for (int32_t index = 0; index < 3; ++ index) {
            LogPrint("INFO", "stable coin genesis block, txid actual: %s, should be: %s, in detail: %s",
                     block.vptx[index + 1]->GetHash().GetHex(), txids[index],
                     block.vptx[index + 1]->ToString(cw.accountCache));
            assert(block.vptx[index + 1]->nTxType == UCOIN_REWARD_TX);
            if (SysCfg().NetworkID() == MAIN_NET) {
                assert(block.vptx[index + 1]->GetHash() == uint256S(txids[index]));
            }
        }
    }

    if (!VerifyPosTx(&block, cw, false))
        return state.DoS(100, ERRORMSG("ConnectBlock() : the block hash=%s check pos tx error",
                        block.GetHash().GetHex()), REJECT_INVALID, "bad-pos-tx");

    CBlockUndo blockUndo;
    int64_t nStart = GetTimeMicros();
    CDiskTxPos pos(pIndex->GetBlockPos(), GetSizeOfCompactSize(block.vptx.size()));
    CDiskTxPos rewardPos = pos;
    pos.nTxOffset += ::GetSerializeSize(block.vptx[0], SER_DISK, CLIENT_VERSION);

    // Re-compute reward values and total fuel
    uint64_t totalFuel                 = 0;
    map<TokenSymbol, uint64_t> rewards = {{SYMB::WICC, 0}, {SYMB::WUSD, 0}};  // Only allow WICC/WUSD as fees type.

    if (block.vptx.size() > 1) {
        assert(mapBlockIndex.count(cw.accountCache.GetBestBlock()));
        int32_t curHeight     = mapBlockIndex[cw.accountCache.GetBestBlock()]->height;
        int32_t validHeight   = SysCfg().GetTxCacheHeight();
        uint32_t fuelRate     = block.GetFuelRate();
        uint64_t totalRunStep = 0;

        bool haveMedianPriceTx = block.vptx[1]->IsMedianPriceTx();
        bool havePriceFeedTx   = (haveMedianPriceTx && (block.vptx.size() > 2) && (block.vptx[2]->IsPriceFeedTx()));
        int32_t lastPriceFeedTxIndex   = 2;      // 0: block reward tx; 1: median price tx
        bool needExecuteTx             = false;  // Execute it after all price feed tx(s) executed.
        uint32_t medianPriceTxOffset   = 0;
        uint32_t lastPriceFeedTxOffset = 0;
        if (havePriceFeedTx) {
            while (block.vptx.size() > (uint32_t)lastPriceFeedTxIndex + 1 &&
                   block.vptx[lastPriceFeedTxIndex + 1]->IsPriceFeedTx()) {
                ++lastPriceFeedTxIndex;
            }
        }

        LogPrint("INFO", "Connect() : haveMedianPriceTx: %d, havePriceFeedTx: %d, lastPriceFeedTxIndex: %d, tx: %llu\n",
                 haveMedianPriceTx, havePriceFeedTx, lastPriceFeedTxIndex, block.vptx.size());

        for (int32_t index = 1; index < (int32_t)block.vptx.size(); ++index) {
            if (index == 1 && haveMedianPriceTx) {
                if (havePriceFeedTx && !needExecuteTx) {
                    needExecuteTx = true;
                    medianPriceTxOffset = pos.nTxOffset;
                    pos.nTxOffset += ::GetSerializeSize(block.vptx[1], SER_DISK, CLIENT_VERSION);
                    continue;
                } else {
                    // Execute median price tx normally.
                }
            }

            std::shared_ptr<CBaseTx> &pBaseTx = block.vptx[index];
            if (cw.txCache.HaveTx((pBaseTx->GetHash())))
                return state.DoS(100, ERRORMSG("ConnectBlock() : txid=%s duplicated", pBaseTx->GetHash().GetHex()),
                    REJECT_INVALID, "tx-duplicated");

            if (!pBaseTx->IsValidHeight(curHeight, validHeight))
                return state.DoS(100, ERRORMSG("ConnectBlock() : txid=%s beyond the scope of valid height",
                    pBaseTx->GetHash().GetHex()), REJECT_INVALID, "tx-invalid-height");

            pBaseTx->nFuelRate = fuelRate;
            cw.EnableTxUndoLog(pBaseTx->GetHash());
            if (!pBaseTx->ExecuteTx(pIndex->height, index, cw, state)) {
                if (SysCfg().IsLogFailures()) {
                    pCdMan->pLogCache->SetExecuteFail(pIndex->height, pBaseTx->GetHash(), state.GetRejectCode(),
                                                      state.GetRejectReason());
                }
                cw.DisableTxUndoLog();
                return false;
            }

            if (!SaveTxIndex(*pBaseTx, cw, state, pos)) {
                cw.DisableTxUndoLog();
                return false;
            }

            blockUndo.vtxundo.push_back(cw.txUndo);
            cw.DisableTxUndoLog();

            totalRunStep += pBaseTx->nRunStep;
            if (totalRunStep > MAX_BLOCK_RUN_STEP)
                return state.DoS(100, ERRORMSG("block hash=%s total run steps exceed max run step", block.GetHash().GetHex()),
                    REJECT_INVALID, "exceed-max-run-step");

            auto fuel = pBaseTx->GetFuel(block.GetFuelRate());
            totalFuel += fuel;

            auto fees_symbol = std::get<0>(pBaseTx->GetFees());
            assert(fees_symbol == SYMB::WICC || fees_symbol == SYMB::WUSD);  // Only allow WICC/WUSD as fees type.
            auto fees = std::get<1>(pBaseTx->GetFees());
            assert(fees >= fuel);
            rewards[fees_symbol] += (fees - fuel);

            pos.nTxOffset += ::GetSerializeSize(pBaseTx, SER_DISK, CLIENT_VERSION);

            if (index == 1 && haveMedianPriceTx && needExecuteTx) {
                // Jump to the tx referred to index = lastPriceFeedTxIndex + 1
                index         = lastPriceFeedTxIndex;  // autoincrement in loop for(;;)
                pos.nTxOffset = lastPriceFeedTxOffset;
            } else if (havePriceFeedTx && index == lastPriceFeedTxIndex) {
                // Jump to the median price tx referred to index = 1
                assert(block.vptx[1]->IsMedianPriceTx());
                index                 = 0;  // autoincrement in loop for(;;)
                lastPriceFeedTxOffset = pos.nTxOffset;
                pos.nTxOffset         = medianPriceTxOffset;
            }

            LogPrint("fuel", "connect block total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txid:%s \n", totalFuel,
                        fuel, pBaseTx->nRunStep, fuelRate, pBaseTx->GetHash().GetHex());
        }
    }

    // Verify total fuel
    if (totalFuel != block.GetFuel())
        return ERRORMSG("fuel value at block header calculate error(actual fuel:%lld vs block fuel:%lld)", totalFuel,
                        block.GetFuel());

    // Verify miner account
    CAccount delegateAccount;
    if (!cw.accountCache.GetAccount(block.vptx[0]->txUid, delegateAccount)) {
        assert(0);
    }

    // Verify reward values
    if (block.vptx[0]->nTxType == BLOCK_REWARD_TX) {
        auto pRewardTx        = (CBlockRewardTx *)block.vptx[0].get();
        if (pRewardTx->reward != rewards.at(SYMB::WICC)) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : invalid coinbase reward amount"), REJECT_INVALID,
                             "bad-reward-amount");
        }
    } else if (block.vptx[0]->nTxType == UCOIN_BLOCK_REWARD_TX) {
        auto pRewardTx = (CUCoinBlockRewardTx *)block.vptx[0].get();
        if (pRewardTx->rewards != rewards) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : invalid coinbase reward amount"), REJECT_INVALID,
                             "bad-reward-amount");
        }

        // Verify profits
        uint64_t profits = delegateAccount.ComputeBlockInflateInterest(block.GetHeight());
        if (pRewardTx->profits != profits) {
            return state.DoS(100, ERRORMSG("ConnectBlock() : invalid coinbase profits amount(actual=%d vs valid=%d)",
                             pRewardTx->profits, profits), REJECT_INVALID, "bad-reward-amount");
        }
    }

    // Execute block reward transaction
    cw.EnableTxUndoLog(block.vptx[0]->GetHash());
    if (!block.vptx[0]->ExecuteTx(pIndex->height, 0, cw, state)) {
        if (SysCfg().IsLogFailures()) {
            pCdMan->pLogCache->SetExecuteFail(pIndex->height, block.vptx[0]->GetHash(), state.GetRejectCode(),
                                              state.GetRejectReason());
        }
        cw.DisableTxUndoLog();
        return ERRORMSG("ConnectBlock() : failed to execute reward transaction");
    }

    if (pIndex->height + 1 == (int32_t)SysCfg().GetFeatureForkHeight() &&
        !ComputeVoteStakingInterestAndRevokeVotes(pIndex->height, cw, state)) {
        return false;
    }

    if (!SaveTxIndex(*block.vptx[0], cw, state, rewardPos)) {
        cw.DisableTxUndoLog();
        return false;
    }

    blockUndo.vtxundo.push_back(cw.txUndo);
    cw.DisableTxUndoLog();

    if (pIndex->height - BLOCK_REWARD_MATURITY > 0) {
        // Deal mature block reward transaction
        CBlockIndex *pMatureIndex = pIndex;
        for (int32_t i = 0; i < BLOCK_REWARD_MATURITY; ++i) {
            pMatureIndex = pMatureIndex->pprev;
        }

        if (nullptr != pMatureIndex) {
            CBlock matureBlock;
            if (!ReadBlockFromDisk(pMatureIndex, matureBlock)) {
                return state.DoS(100, ERRORMSG("ConnectBlock() : read mature block error"), REJECT_INVALID,
                                 "bad-read-block");
            }

            cw.EnableTxUndoLog(block.vptx[0]->GetHash());
            if (!matureBlock.vptx[0]->ExecuteTx(pIndex->height, -1, cw, state)) {
                if (SysCfg().IsLogFailures()) {
                    pCdMan->pLogCache->SetExecuteFail(pIndex->height, matureBlock.vptx[0]->GetHash(),
                                                      state.GetRejectCode(), state.GetRejectReason());
                }
                cw.DisableTxUndoLog();
                return ERRORMSG("ConnectBlock() : execute mature block reward tx error!");
            }
        }
        blockUndo.vtxundo.push_back(cw.txUndo);
        cw.DisableTxUndoLog();
    }
    int64_t nTime = GetTimeMicros() - nStart;
    if (SysCfg().IsBenchmark())
        LogPrint("INFO", "- Connect %u transactions: %.2fms (%.3fms/tx)\n",
                 (uint32_t)block.vptx.size(), 0.001 * nTime, 0.001 * nTime / block.vptx.size());

    if (fJustCheck)
        return true;

    // Write undo information to disk
    if (pIndex->GetUndoPos().IsNull() || (pIndex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS) {
        if (pIndex->GetUndoPos().IsNull()) {
            CDiskBlockPos pos;
            if (!FindUndoPos(state, pIndex->nFile, pos, ::GetSerializeSize(blockUndo, SER_DISK, CLIENT_VERSION) + 40))
                return ERRORMSG("ConnectBlock() : failed to find undo data's position");

            if (!blockUndo.WriteToDisk(pos, pIndex->pprev->GetBlockHash()))
                return state.Abort(_("ConnectBlock() : failed to write undo data"));

            // Update nUndoPos in block index
            pIndex->nUndoPos = pos.nPos;
            pIndex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pIndex->nStatus = (pIndex->nStatus & ~BLOCK_VALID_MASK) | BLOCK_VALID_SCRIPTS;

        CDiskBlockIndex blockIndex(pIndex);
        if (!pCdMan->pBlockTreeDb->WriteBlockIndex(blockIndex))
            return state.Abort(_("ConnectBlock() : failed to write block index"));
    }

    if (!cw.txCache.AddBlockToCache(block)) {
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

        if (!cw.txCache.DeleteBlockFromCache(deleteBlock)) {
            return state.Abort(_("ConnectBlock() : failed delete block from transaction memory cache"));
        }
    }

    // Attention: should NOT to call AddBlockToCache() for price point memory cache, as everything
    // is ready when executing transactions.

    // TODO: parameterize 11.
    if (pIndex->height > 11) {
        CBlockIndex *pDeleteBlockIndex = pIndex;
        int32_t nCacheHeight           = 11;
        while (pDeleteBlockIndex && nCacheHeight-- > 0) {
            pDeleteBlockIndex = pDeleteBlockIndex->pprev;
        }

        CBlock deleteBlock;
        if (!ReadBlockFromDisk(pDeleteBlockIndex, deleteBlock)) {
            return state.Abort(_("ConnectBlock() : failed to read block"));
        }

        if (!cw.ppCache.DeleteBlockFromCache(deleteBlock)) {
            return state.Abort(_("ConnectBlock() : failed delete block from price point memory cache"));
        }
    }

    // Set best block to current account cache.
    assert(cw.accountCache.SetBestBlock(pIndex->GetBlockHash()));
    return true;
}

// Update the on-disk chain state.
bool static WriteChainState(CValidationState &state) {
    static int64_t nLastWrite = 0;
    uint32_t cachesize    = pCdMan->pAccountCache->GetCacheSize() + pCdMan->pContractCache->GetCacheSize() +
                             pCdMan->pDelegateCache->GetCacheSize() + pCdMan->pCdpCache->GetCacheSize();

    if (!IsInitialBlockDownload()
        || cachesize > SysCfg().GetViewCacheSize()
        || GetTimeMicros() > nLastWrite + 60 * 1000000) {
        // Typical CCoins structures on disk are around 100 bytes in size.
        // Pushing a new one to the database can cause it to be written
        // twice (once in the log, and once in the tables). This is already
        // an overestimation, as most will delete an existing entry or
        // overwrite one. Still, use a conservative safety factor of 2.
        if (!CheckDiskSpace(cachesize))
            return state.Error("out of disk space");

        FlushBlockFile();
        pCdMan->pBlockTreeDb->Sync();
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
    mempool.AddUpdatedTransactionNum(1);
    LogPrint("INFO", "UpdateTip: new best=%s  height=%d  tx=%lu  date=%s txnumber=%d nFuelRate=%d\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(), chainActive.Tip()->nChainTx,
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
             block.vptx.size(), chainActive.Tip()->nFuelRate);

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
            LogPrint("INFO", "SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int32_t)CBlock::CURRENT_VERSION);
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
            return ERRORMSG("DisconnectTip() : DisconnectBlock %s failed",
                            pIndexDelete->GetBlockHash().ToString());

        // Need to re-sync all to global cache layer.
        spCW->Flush();
        // Attention: need to reload top N delegates.
        pCdMan->pDelegateCache->LoadTopDelegateList();
    }
    if (SysCfg().IsBenchmark())
        LogPrint("INFO", "- Disconnect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;
    // Update chainActive and related variables.
    UpdateTip(pIndexDelete->pprev, block);
    // Resurrect mempool transactions from the disconnected block.
    for (const auto &ptx : block.vptx) {
        list<std::shared_ptr<CBaseTx> > removed;
        CValidationState stateDummy;
        if (!ptx->IsBlockRewardTx()) {
            if (!AcceptToMemoryPool(mempool, stateDummy, ptx.get(), false)) {
                mempool.Remove(ptx.get(), removed, true);
            }
        } else {
            EraseTransaction(ptx->GetHash());
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
        return state.Abort(strprintf("Failed to read block hash:%s\n", pIndexNew->GetBlockHash().GetHex()));

    // Apply the block automatically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CInv inv(MSG_BLOCK, pIndexNew->GetBlockHash());

        auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
        if (!ConnectBlock(block, *spCW, pIndexNew, state)) {
            if (state.IsInvalid()) {
                InvalidBlockFound(pIndexNew, state);
            }
            return ERRORMSG("ConnectTip() : ConnectBlock %s failed", pIndexNew->GetBlockHash().ToString());
        }
        mapBlockSource.erase(inv.hash);

        // Need to re-sync all to global cache layer.
        spCW->Flush();
        // Attention: need to reload top N delegates.
        pCdMan->pDelegateCache->LoadTopDelegateList();

        uint256 uBestblockHash = pCdMan->pAccountCache->GetBestBlock();
        LogPrint("INFO", "uBestBlockHash[%d]: %s\n", nSyncTipHeight, uBestblockHash.GetHex());
    }

    if (SysCfg().IsBenchmark())
        LogPrint("INFO", "- Connect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);

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
                if (pindexBestInvalid == nullptr || pIndexNew->nChainWork > pindexBestInvalid->nChainWork)
                    pindexBestInvalid = pIndexNew;
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

// Try to activate to the most-work chain (thereby connecting it).
bool ActivateBestChain(CValidationState &state) {
    LOCK(cs_main);
    CBlockIndex *pIndexOldTip = chainActive.Tip();
    bool fComplete            = false;
    while (!fComplete) {
        FindMostWorkChain();
        fComplete = true;

        // Check whether we have something to do.
        if (chainMostWork.Tip() == nullptr) break;

        // Disconnect active blocks which are no longer in the best chain.
        while (chainActive.Tip() && !chainMostWork.Contains(chainActive.Tip())) {
            if (!DisconnectTip(state))
                return false;

            if (chainActive.Tip() && chainMostWork.Contains(chainActive.Tip()))
                mempool.ReScanMemPoolTx(pCdMan);
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
                mempool.ReScanMemPoolTx(pCdMan);
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
    // LogPrint("INFO", "in map hash:%s map size:%d\n", hash.GetHex(), mapBlockIndex.size());
    pIndexNew->pBlockHash                        = &((*mi).first);
    map<uint256, CBlockIndex *>::iterator miPrev = mapBlockIndex.find(block.GetPrevBlockHash());
    if (miPrev != mapBlockIndex.end()) {
        pIndexNew->pprev  = (*miPrev).second;
        pIndexNew->height = pIndexNew->pprev->height + 1;
        pIndexNew->BuildSkip();
    }
    pIndexNew->nTx        = block.vptx.size();
    pIndexNew->nChainWork = pIndexNew->height;
    pIndexNew->nChainTx   = (pIndexNew->pprev ? pIndexNew->pprev->nChainTx : 0) + pIndexNew->nTx;
    pIndexNew->nFile      = pos.nFile;
    pIndexNew->nDataPos   = pos.nPos;
    pIndexNew->nUndoPos   = 0;
    pIndexNew->nStatus    = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
    setBlockIndexValid.insert(pIndexNew);

    if (!pCdMan->pBlockTreeDb->WriteBlockIndex(CDiskBlockIndex(pIndexNew)))
        return state.Abort(_("Failed to write block index"));
    int64_t beginTime = GetTimeMillis();
    // New best?
    if (!ActivateBestChain(state)) {
        LogPrint("INFO", "ActivateBestChain() elapse time:%lld ms\n", GetTimeMillis() - beginTime);
        return false;
    }
    // LogPrint("INFO", "ActivateBestChain() elapse time:%lld ms\n", GetTimeMillis() - beginTime);
    LOCK(cs_main);
    if (pIndexNew == chainActive.Tip()) {
        // Clear fork warning if its no longer applicable
        CheckForkWarningConditions();
    } else
        CheckForkWarningConditionsOnNewFork(pIndexNew);

    if (!pCdMan->pBlockTreeDb->Flush())
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
            pCdMan->pBlockTreeDb->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);
            fUpdatedLast = true;
        }
    } else {
        while (infoLastBlockFile.nSize + nAddSize >= MAX_BLOCKFILE_SIZE) {
            LogPrint("INFO", "Leaving block file %i: %s\n", nLastBlockFile, infoLastBlockFile.ToString());
            FlushBlockFile(true);
            nLastBlockFile++;
            infoLastBlockFile.SetNull();
            pCdMan->pBlockTreeDb->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);  // check whether data for the new file somehow already exist; can fail just fine
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
                    LogPrint("INFO", "Pre-allocating up to position 0x%x in blk%05u.dat\n", nNewChunks * BLOCKFILE_CHUNK_SIZE, pos.nFile);
                    AllocateFileRange(file, pos.nPos, nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos);
                    fclose(file);
                }
            } else
                return state.Error("out of disk space");
        }
    }

    if (!pCdMan->pBlockTreeDb->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        return state.Abort(_("Failed to write file info"));
    if (fUpdatedLast)
        pCdMan->pBlockTreeDb->WriteLastBlockFile(nLastBlockFile);

    return true;
}

bool ProcessForkedChain(const CBlock &block, CBlockIndex *pPreBlockIndex, CValidationState &state) {
    if (pPreBlockIndex->GetBlockHash() == chainActive.Tip()->GetBlockHash())
        return true;  // No fork, return immediately.

    auto spForkCW       = std::make_shared<CCacheWrapper>();
    auto spCW           = std::make_shared<CCacheWrapper>(pCdMan);
    bool forkChainTipFound = false;
    uint256 forkChainTipBlockHash;
    vector<CBlock> vPreBlocks;

    // If the block's previous block is not the active chain's tip, find the forked point.
    while (!chainActive.Contains(pPreBlockIndex)) {
        if (!forkChainTipFound) {
            if (mapForkCache.count(pPreBlockIndex->GetBlockHash())) {
                forkChainTipBlockHash = pPreBlockIndex->GetBlockHash();
                forkChainTipFound     = true;
                LogPrint("INFO", "ProcessForkedChain() : fork chain's best block [%d]: %s\n",
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
        // if (chainActive.Height() - pPreBlockIndex->height > SysCfg().GetMaxForkHeight())
        //     return state.DoS(100, ERRORMSG(
        //         "ProcessForkedChain() : block at fork chain too earlier than tip block hash=%s block height=%d\n",
        //         block.GetHash().GetHex(), block.GetHeight()));

        if (mapBlockIndex.find(pPreBlockIndex->GetBlockHash()) == mapBlockIndex.end())
            return state.DoS(10, ERRORMSG("ProcessForkedChain() : prev block not found"), 0, "bad-prevblk");
    }

    if (mapForkCache.count(pPreBlockIndex->GetBlockHash())) {
        uint256 blockHash   = pPreBlockIndex->GetBlockHash();
        int32_t blockHeight = pPreBlockIndex->height;

        spCW->sysParamCache = mapForkCache[blockHash]->sysParamCache;
        spCW->accountCache  = mapForkCache[blockHash]->accountCache;
        spCW->contractCache = mapForkCache[blockHash]->contractCache;
        spCW->delegateCache = mapForkCache[blockHash]->delegateCache;
        spCW->cdpCache      = mapForkCache[blockHash]->cdpCache;
        spCW->dexCache      = mapForkCache[blockHash]->dexCache;
        spCW->txReceiptCache= mapForkCache[blockHash]->txReceiptCache;
        spCW->txCache       = mapForkCache[blockHash]->txCache;
        spCW->ppCache       = mapForkCache[blockHash]->ppCache;

        LogPrint("INFO", "ProcessForkedChain() : found [%d]: %s in cache\n",
            blockHeight, blockHash.GetHex());
    } else {
        int64_t beginTime        = GetTimeMillis();
        CBlockIndex *pBlockIndex = chainActive.Tip();

        while (pPreBlockIndex != pBlockIndex) {
            LogPrint("INFO", "ProcessForkedChain() : disconnect block [%d]: %s\n",
                pBlockIndex->height, pBlockIndex->GetBlockHash().GetHex());

            CBlock block;
            if (!ReadBlockFromDisk(pBlockIndex, block))
                return state.Abort(_("Failed to read block"));

            bool bfClean = true;
            if (!DisconnectBlock(block, *spCW, pBlockIndex, state, &bfClean)) {
                return ERRORMSG("ProcessForkedChain() : failed to disconnect block [%d]: %s",
                                pBlockIndex->height, pBlockIndex->GetBlockHash().ToString());
            }

            pBlockIndex = pBlockIndex->pprev;
        }  // Rollback the active chain to the forked point.

        mapForkCache[pPreBlockIndex->GetBlockHash()] = spCW;
        LogPrint("INFO", "ProcessForkedChain() : add [%d]: %s to cache\n", pPreBlockIndex->height,
                 pPreBlockIndex->GetBlockHash().GetHex());

        LogPrint("INFO", "ProcessForkedChain() : disconnect blocks elapse: %lld ms\n", GetTimeMillis() - beginTime);
    }

    if (forkChainTipFound) {
        spForkCW->sysParamCache  = mapForkCache[forkChainTipBlockHash]->sysParamCache;
        spForkCW->accountCache   = mapForkCache[forkChainTipBlockHash]->accountCache;
        spForkCW->contractCache  = mapForkCache[forkChainTipBlockHash]->contractCache;
        spForkCW->delegateCache  = mapForkCache[forkChainTipBlockHash]->delegateCache;
        spForkCW->cdpCache       = mapForkCache[forkChainTipBlockHash]->cdpCache;
        spForkCW->dexCache       = mapForkCache[forkChainTipBlockHash]->dexCache;
        spForkCW->txReceiptCache = mapForkCache[forkChainTipBlockHash]->txReceiptCache;
        spForkCW->txCache        = mapForkCache[forkChainTipBlockHash]->txCache;
        spForkCW->ppCache        = mapForkCache[forkChainTipBlockHash]->ppCache;
    } else {
        spForkCW->sysParamCache  = spCW->sysParamCache;
        spForkCW->accountCache   = spCW->accountCache;
        spForkCW->contractCache  = spCW->contractCache;
        spForkCW->delegateCache  = spCW->delegateCache;
        spForkCW->cdpCache       = spCW->cdpCache;
        spForkCW->dexCache       = spCW->dexCache;
        spForkCW->txReceiptCache = spCW->txReceiptCache;
        spForkCW->txCache        = spCW->txCache;
        spForkCW->ppCache        = spCW->ppCache;
    }

    uint256 forkChainBestBlockHash = spForkCW->accountCache.GetBestBlock();
    int32_t forkChainBestBlockHeight   = mapBlockIndex[forkChainBestBlockHash]->height;
    LogPrint("INFO", "ProcessForkedChain() : fork chain's best block [%d]: %s\n", forkChainBestBlockHeight,
             forkChainBestBlockHash.GetHex());

    // Connect all of the forked chain's blocks.
    vector<CBlock>::reverse_iterator rIter = vPreBlocks.rbegin();
    for (; rIter != vPreBlocks.rend(); ++rIter) {
        LogPrint("INFO", "ProcessForkedChain() : ConnectBlock block height=%d hash=%s\n", rIter->GetHeight(),
                 rIter->GetHash().GetHex());

        if (!ConnectBlock(*rIter, *spForkCW, mapBlockIndex[rIter->GetHash()], state, false)) {
            return ERRORMSG("ProcessForkedChain() : ConnectBlock %s failed", rIter->GetHash().ToString());
        }

        CBlockIndex *pConnBlockIndex = mapBlockIndex[rIter->GetHash()];
        if (pConnBlockIndex->nStatus | BLOCK_FAILED_MASK) {
            pConnBlockIndex->nStatus = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
        }
    }

    // Verify reward transaction
    if (!VerifyPosTx(&block, *spForkCW, true)) {
        return state.DoS(100, ERRORMSG("ProcessForkedChain() : failed to verify pos transaction, block hash=%s",
                        block.GetHash().GetHex()), REJECT_INVALID, "bad-pos-tx");
    }

    // Verify reward value
    CAccount delegateAccount;
    if (!spForkCW->accountCache.GetAccount(block.vptx[0]->txUid, delegateAccount)) {
        assert(0);
    }

    // TODO: Fees
    // if (block.vptx[0]->nTxType == BLOCK_REWARD_TX) {
    //     auto pRewardTx = (CBlockRewardTx *)block.vptx[0].get();
    //     uint64_t llValidReward = block.GetFees() - block.GetFuel();
    //     if (pRewardTx->reward != llValidReward) {
    //         LogPrint("ERROR", "ProcessForkedChain() : block height:%u, block fee:%lld, block fuel:%u\n",
    //                  block.GetHeight(), block.GetFees(), block.GetFuel());
    //         return state.DoS(100, ERRORMSG("ProcessForkedChain() : invalid coinbase reward amount(actual=%d vs valid=%d)",
    //                          pRewardTx->reward, llValidReward), REJECT_INVALID, "bad-reward-amount");
    //     }
    // } else if (block.vptx[0]->nTxType == UCOIN_BLOCK_REWARD_TX) {
    //     auto pRewardTx = (CUCoinBlockRewardTx *)block.vptx[0].get();
    //     uint64_t llValidReward = block.GetFees() - block.GetFuel();
    //     if (pRewardTx->reward != llValidReward) {
    //         return state.DoS(100, ERRORMSG("ProcessForkedChain() : invalid coinbase reward amount(actual=%d vs valid=%d)",
    //                          pRewardTx->reward, llValidReward), REJECT_INVALID, "bad-reward-amount");
    //     }

    //     uint64_t profits = delegateAccount.ComputeBlockInflateInterest(block.GetHeight());
    //     if (pRewardTx->profits != profits) {
    //         return state.DoS(100, ERRORMSG("ProcessForkedChain() : invalid coinbase profits amount(actual=%d vs valid=%d)",
    //                          pRewardTx->profits, profits), REJECT_INVALID, "bad-reward-amount");
    //     }
    // }

    // forkChainBestBlockHeight = mapBlockIndex[spForkCW->accountCache.GetBestBlock()]->height;
    // for (auto &item : block.vptx) {
    //     // Verify height
    //     if (!item->IsValidHeight(forkChainBestBlockHeight, SysCfg().GetTxCacheHeight())) {
    //         return state.DoS(100, ERRORMSG("ProcessForkedChain() : txid=%s beyond the scope of valid height\n ",
    //                          item->GetHash().GetHex()), REJECT_INVALID, "tx-invalid-height");
    //     }
    //     // Verify duplicated transaction
    //     if (spForkCW->txCache.HaveTx(item->GetHash()))
    //         return state.DoS(100, ERRORMSG("ProcessForkedChain() : txid=%s has been confirmed",
    //                          item->GetHash().GetHex()), REJECT_INVALID, "duplicated-txid");
    // }

    if (!vPreBlocks.empty()) {
        vector<CBlock>::iterator iterBlock = vPreBlocks.begin();
        if (forkChainTipFound) {
            mapForkCache.erase(forkChainTipBlockHash);
        }

        mapForkCache[iterBlock->GetHash()] = spForkCW;
    }

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
    if (block.GetBlockTime() > GetAdjustedTime() + ::GetBlockInterval(block.GetHeight()) + 2) {
        return state.Invalid(ERRORMSG("CheckBlock() : block timestamp too far in the future"), REJECT_INVALID,
                             "time-too-new");
    }

    // First transaction must be reward transaction, the rest must not be
    if (block.vptx.empty() || !block.vptx[0]->IsBlockRewardTx())
        return state.DoS(100, ERRORMSG("CheckBlock() : first tx is not coinbase"), REJECT_INVALID, "bad-cb-missing");

    // Build the merkle tree already. We need it anyway later, and it makes the
    // block cache the transaction hashes, which means they don't need to be
    // recalculated many times during this block's validation.
    block.BuildMerkleTree();

    // Check for duplicate txids. This is caught by ConnectInputs(),
    // but catching it earlier avoids a potential DoS attack:
    set<uint256> uniqueTx;
    for (uint32_t i = 0; i < block.vptx.size(); i++) {
        uniqueTx.insert(block.GetTxid(i));

        if (fCheckTx && !block.vptx[i]->CheckTx(block.GetHeight(), cw, state))
            return ERRORMSG("CheckBlock() : CheckTx failed, txid: %s", block.vptx[i]->GetHash().GetHex());

        if (block.GetHeight() != 0 || block.GetHash() != SysCfg().GetGenesisBlockHash()) {
            if (0 != i && block.vptx[i]->IsBlockRewardTx())
                return state.DoS(100, ERRORMSG("CheckBlock() : more than one coinbase"), REJECT_INVALID, "bad-coinbase-multiple");

            // Second transaction must be median price transaction if existed.
            if (1 != i && block.vptx[i]->IsMedianPriceTx())
                return state.DoS(100, ERRORMSG("CheckBlock() : more than one median price tx"), REJECT_INSUFFICIENTFEE,
                                 "bad-median-price-multiple");
        }
    }

    if (uniqueTx.size() != block.vptx.size())
        return state.DoS(100, ERRORMSG("CheckBlock() : duplicate transaction"), REJECT_INVALID, "bad-tx-duplicated",
                         true);

    // Check merkle root
    if (fCheckMerkleRoot && block.GetMerkleRootHash() != block.vMerkleTree.back())
        return state.DoS(100, ERRORMSG("CheckBlock() : merkleRootHash mismatch, height: %u, merkleRootHash(in block: %s vs calculate: %s)",
                        block.GetHeight(), block.GetMerkleRootHash().ToString(), block.vMerkleTree.back().ToString()),
                        REJECT_INVALID, "bad-merkle-root", true);

    // Check nonce
    static uint64_t maxNonce = SysCfg().GetBlockMaxNonce();
    if (block.GetNonce() > maxNonce) {
        return state.Invalid(ERRORMSG("CheckBlock() : Nonce is larger than maxNonce"), REJECT_INVALID, "Nonce-too-large");
    }

    return true;
}

bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp) {
    AssertLockHeld(cs_main);

    uint256 blockHash = block.GetHash();
    LogPrint("INFO", "AcceptBlock[%d]: %s\n", block.GetHeight(), blockHash.GetHex());
    // Check for duplicated block
    if (mapBlockIndex.count(blockHash))
        return state.Invalid(ERRORMSG("AcceptBlock() : block already in mapBlockIndex"), 0, "duplicated");

    assert(block.GetHeight() == 0 || mapBlockIndex.count(block.GetPrevBlockHash()));

    if (block.GetHeight() != 0 && block.GetFuelRate() != GetElementForBurn(mapBlockIndex[block.GetPrevBlockHash()]))
        return state.DoS(100, ERRORMSG("CheckBlock() : block fuel rate unmatched"), REJECT_INVALID,
                         "fuel-rate-unmatched");

    // Get prev block index
    CBlockIndex *pBlockIndexPrev = nullptr;
    int32_t height = 0;
    if (block.GetHeight() != 0 || blockHash != SysCfg().GetGenesisBlockHash()) {
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(block.GetPrevBlockHash());
        if (mi == mapBlockIndex.end())
            return state.DoS(10, ERRORMSG("AcceptBlock() : prev block not found"), 0, "bad-prevblk");

        pBlockIndexPrev = (*mi).second;
        height          = pBlockIndexPrev->height + 1;

        if (block.GetHeight() != (uint32_t)height) {
            return state.DoS(100, ERRORMSG("AcceptBlock() : height given in block mismatches with its actual height"),
                             REJECT_INVALID, "incorrect-height");
        }

        int64_t beginTime = GetTimeMillis();

        // Check timestamp against prev
        if (block.GetBlockTime() <= pBlockIndexPrev->GetBlockTime() ||
            (block.GetBlockTime() - pBlockIndexPrev->GetBlockTime()) < GetBlockInterval(block.GetHeight())) {
            return state.Invalid(ERRORMSG("AcceptBlock() : the new block came in too early"),
                                REJECT_INVALID, "time-too-early");
        }

        // Process forked branch
        if (!ProcessForkedChain(block, pBlockIndexPrev, state)) {
            LogPrint("INFO", "ProcessForkedChain() end: %lld ms\n", GetTimeMillis() - beginTime);
            return state.DoS(100, ERRORMSG("AcceptBlock() : check proof of pos tx"), REJECT_INVALID, "bad-pos-tx");
        }

        // Reject block.nVersion=1 blocks when 95% (75% on testnet) of the network has been upgraded:
        if (block.GetVersion() < 2) {
            if ((!TestNet() && CBlockIndex::IsSuperMajority(2, pBlockIndexPrev, 950, 1000)) ||
                (TestNet() && CBlockIndex::IsSuperMajority(2, pBlockIndexPrev, 75, 100))) {
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
        return state.Abort(_("System error: ") + e.what());
    }

    // Relay inventory, but don't relay old inventory during initial block download
    if (chainActive.Tip()->GetBlockHash() == blockHash) {
        LOCK(cs_vNodes);
        for (auto pNode : vNodes) {
            if (chainActive.Height() > (pNode->nStartingHeight != -1 ? pNode->nStartingHeight - 2000 : 0))
                pNode->PushInventory(CInv(MSG_BLOCK, blockHash));
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

void PushGetBlocks(CNode *pNode, CBlockIndex *pindexBegin, uint256 hashEnd) {
    // Ask this guy to fill in what we're missing
    AssertLockHeld(cs_main);
    // Filter out duplicate requests
    if (pindexBegin == pNode->pindexLastGetBlocksBegin && hashEnd == pNode->hashLastGetBlocksEnd) {
        LogPrint("net", "filter the same GetLocator\n");
        return;
    }
    pNode->pindexLastGetBlocksBegin = pindexBegin;
    pNode->hashLastGetBlocksEnd     = hashEnd;
    CBlockLocator blockLocator      = chainActive.GetLocator(pindexBegin);
    pNode->PushMessage("getblocks", blockLocator, hashEnd);
    LogPrint("net", "getblocks from peer %s, hashEnd:%s\n", pNode->addr.ToString(), hashEnd.GetHex());
}

void PushGetBlocksOnCondition(CNode *pNode, CBlockIndex *pindexBegin, uint256 hashEnd) {
    // Ask this guy to fill in what we're missing
    AssertLockHeld(cs_main);
    // Filter out duplicate requests
    if (pindexBegin == pNode->pindexLastGetBlocksBegin && hashEnd == pNode->hashLastGetBlocksEnd) {
        LogPrint("net", "filter the same GetLocator\n");
        static CBloomFilter filter(5000, 0.0001, 0, BLOOM_UPDATE_NONE);
        static uint32_t count = 0;
        string key                = to_string(pNode->id) + ":" + to_string((GetTime() / 2));
        if (!filter.contains(vector<uint8_t>(key.begin(), key.end()))) {
            filter.insert(vector<uint8_t>(key.begin(), key.end()));
            ++count;
            pNode->pindexLastGetBlocksBegin = pindexBegin;
            pNode->hashLastGetBlocksEnd     = hashEnd;
            CBlockLocator blockLocator      = chainActive.GetLocator(pindexBegin);
            pNode->PushMessage("getblocks", blockLocator, hashEnd);
            LogPrint("net", "getblocks from peer %s, hashEnd:%s\n", pNode->addr.ToString(), hashEnd.GetHex());
        } else {
            if (count >= 5000) {
                count = 0;
                filter.Clear();
            }
        }
    } else {
        pNode->pindexLastGetBlocksBegin = pindexBegin;
        pNode->hashLastGetBlocksEnd     = hashEnd;
        CBlockLocator blockLocator      = chainActive.GetLocator(pindexBegin);
        pNode->PushMessage("getblocks", blockLocator, hashEnd);
        LogPrint("net", "getblocks from peer %s, hashEnd:%s\n", pNode->addr.ToString(), hashEnd.GetHex());
    }
}

bool ProcessBlock(CValidationState &state, CNode *pFrom, CBlock *pBlock, CDiskBlockPos *dbp) {
    int64_t llBeginTime = GetTimeMillis();
    //  LogPrint("INFO", "ProcessBlock() enter:%lld\n", llBeginTime);
    AssertLockHeld(cs_main);
    // Check for duplicate
    uint256 blockHash = pBlock->GetHash();
    if (mapBlockIndex.count(blockHash))
        return state.Invalid(ERRORMSG("ProcessBlock() : block exists: %d %s",
                            mapBlockIndex[blockHash]->height, blockHash.ToString()), 0, "duplicate");

    if (mapOrphanBlocks.count(blockHash))
        return state.Invalid(ERRORMSG("ProcessBlock() : block (orphan) exists %s", blockHash.ToString()), 0, "duplicate");

    int64_t llBeginCheckBlockTime = GetTimeMillis();
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);

    // Preliminary checks
    if (!CheckBlock(*pBlock, state, *spCW, false)) {
        LogPrint("INFO", "CheckBlock() height: %d elapse time:%lld ms\n",
                chainActive.Height(), GetTimeMillis() - llBeginCheckBlockTime);

        return ERRORMSG("ProcessBlock() : block hash:%s CheckBlock FAILED", pBlock->GetHash().GetHex());
    }

    // If we don't already have its previous block, shunt it off to holding area until we get it
    if (!pBlock->GetPrevBlockHash().IsNull() && !mapBlockIndex.count(pBlock->GetPrevBlockHash())) {
        if (pBlock->GetHeight() > (uint32_t)nSyncTipHeight) {
            LogPrint("DEBUG", "blockHeight=%d syncTipHeight=%d\n", pBlock->GetHeight(), nSyncTipHeight );
            nSyncTipHeight = pBlock->GetHeight();
        }

        // Accept orphans as long as there is a node to request its parents from
        if (pFrom) {
            bool success = PruneOrphanBlocks(pBlock->GetHeight());
            if (success) {
                COrphanBlock *pblock2 = new COrphanBlock();
                {
                    CDataStream ss(SER_DISK, CLIENT_VERSION);
                    ss << *pBlock;
                    pblock2->vchBlock = vector<uint8_t>(ss.begin(), ss.end());
                }
                pblock2->blockHash = blockHash;
                pblock2->prevBlockHash = pBlock->GetPrevBlockHash();
                pblock2->height = pBlock->GetHeight();
                mapOrphanBlocks.insert(make_pair(blockHash, pblock2));
                mapOrphanBlocksByPrev.insert(make_pair(pblock2->prevBlockHash, pblock2));
                setOrphanBlock.insert(pblock2);
            }

            // Ask this guy to fill in what we're missing
            LogPrint("net", "receive an orphan block height=%d hash=%s, %s it, leading to getblocks (current height=%d & orphan blocks=%d)\n",
                    pBlock->GetHeight(), pBlock->GetHash().GetHex(), success ? "keep" : "abandon",
                    chainActive.Height(), mapOrphanBlocksByPrev.size());

            PushGetBlocksOnCondition(pFrom, chainActive.Tip(), GetOrphanRoot(blockHash));
        }
        return true;
    }

    int64_t llAcceptBlockTime = GetTimeMillis();
    // Store to disk
    if (!AcceptBlock(*pBlock, state, dbp)) {
        LogPrint("INFO", "AcceptBlock() elapse time: %lld ms\n", GetTimeMillis() - llAcceptBlockTime);
        return ERRORMSG("ProcessBlock() : AcceptBlock FAILED");
    }
    // LogPrint("INFO", "AcceptBlock() elapse time:%lld ms\n", GetTimeMillis() - llAcceptBlockTime);

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

    LogPrint("INFO", "ProcessBlock() elapse time:%lld ms\n", GetTimeMillis() - llBeginTime);
    return true;
}

CMerkleBlock::CMerkleBlock(const CBlock &block, CBloomFilter &filter) {
    header = block.GetBlockHeader();

    vector<bool> vMatch;
    vector<uint256> vHashes;

    vMatch.reserve(block.vptx.size());
    vHashes.reserve(block.vptx.size());

    for (uint32_t i = 0; i < block.vptx.size(); i++) {
        uint256 hash = block.vptx[i]->GetHash();
        if (filter.contains(block.vptx[i]->GetHash())) {
            vMatch.push_back(true);
            vMatchedTxn.push_back(make_pair(i, hash));
        } else
            vMatch.push_back(false);
        vHashes.push_back(hash);
    }

    txn = CPartialMerkleTree(vHashes, vMatch);
}

uint256 CPartialMerkleTree::CalcHash(int32_t height, uint32_t pos, const vector<uint256> &vTxid) {
    if (height == 0) {
        // hash at height 0 is the txids themself
        return vTxid[pos];
    } else {
        // calculate left hash
        uint256 left = CalcHash(height - 1, pos * 2, vTxid), right;
        // calculate right hash if not beyond the end of the array - copy left hash otherwise1
        if (pos * 2 + 1 < CalcTreeWidth(height - 1))
            right = CalcHash(height - 1, pos * 2 + 1, vTxid);
        else
            right = left;
        // combine subhashes
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    }
}

void CPartialMerkleTree::TraverseAndBuild(int32_t height, uint32_t pos, const vector<uint256> &vTxid, const vector<bool> &vMatch) {
    // determine whether this node is the parent of at least one matched txid
    bool fParentOfMatch = false;
    for (uint32_t p = pos << height; p < (pos + 1) << height && p < nTransactions; p++)
        fParentOfMatch |= vMatch[p];
    // store as flag bit
    vBits.push_back(fParentOfMatch);
    if (height == 0 || !fParentOfMatch) {
        // if at height 0, or nothing interesting below, store hash and stop
        vHash.push_back(CalcHash(height, pos, vTxid));
    } else {
        // otherwise, don't store any hash, but descend into the subtrees
        TraverseAndBuild(height - 1, pos * 2, vTxid, vMatch);
        if (pos * 2 + 1 < CalcTreeWidth(height - 1))
            TraverseAndBuild(height - 1, pos * 2 + 1, vTxid, vMatch);
    }
}

uint256 CPartialMerkleTree::TraverseAndExtract(int32_t height, uint32_t pos,
                                               uint32_t &nBitsUsed, uint32_t &nHashUsed, vector<uint256> &vMatch) {
    if (nBitsUsed >= vBits.size()) {
        // overflowed the bits array - failure
        fBad = true;
        return uint256();
    }
    bool fParentOfMatch = vBits[nBitsUsed++];
    if (height == 0 || !fParentOfMatch) {
        // if at height 0, or nothing interesting below, use stored hash and do not descend
        if (nHashUsed >= vHash.size()) {
            // overflowed the hash array - failure
            fBad = true;
            return uint256();
        }
        const uint256 &hash = vHash[nHashUsed++];
        if (height == 0 && fParentOfMatch)  // in case of height 0, we have a matched txid
            vMatch.push_back(hash);
        return hash;
    } else {
        // otherwise, descend into the subtrees to extract matched txids and hashes
        uint256 left = TraverseAndExtract(height - 1, pos * 2, nBitsUsed, nHashUsed, vMatch), right;
        if (pos * 2 + 1 < CalcTreeWidth(height - 1))
            right = TraverseAndExtract(height - 1, pos * 2 + 1, nBitsUsed, nHashUsed, vMatch);
        else
            right = left;
        // and combine them before returning
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    }
}

CPartialMerkleTree::CPartialMerkleTree(const vector<uint256> &vTxid, const vector<bool> &vMatch)
    : nTransactions(vTxid.size()), fBad(false) {
    // reset state
    vBits.clear();
    vHash.clear();

    // calculate height of tree
    int32_t height = 0;
    while (CalcTreeWidth(height) > 1)
        height++;

    // traverse the partial tree
    TraverseAndBuild(height, 0, vTxid, vMatch);
}

CPartialMerkleTree::CPartialMerkleTree() : nTransactions(0), fBad(true) {}

uint256 CPartialMerkleTree::ExtractMatches(vector<uint256> &vMatch) {
    vMatch.clear();
    // An empty set will not work
    if (nTransactions == 0)
        return uint256();
    // check for excessively high numbers of transactions
    if (nTransactions > MAX_BLOCK_SIZE / 60)  // 60 is the lower bound for the size of a serialized Transaction
        return uint256();
    // there can never be more hashes provided than one for every txid
    if (vHash.size() > nTransactions)
        return uint256();
    // there must be at least one bit per node in the partial tree, and at least one node per hash
    if (vBits.size() < vHash.size())
        return uint256();
    // calculate height of tree
    int32_t height = 0;
    while (CalcTreeWidth(height) > 1)
        height++;
    // traverse the partial tree
    uint32_t nBitsUsed = 0, nHashUsed = 0;
    uint256 merkleRootHash = TraverseAndExtract(height, 0, nBitsUsed, nHashUsed, vMatch);
    // verify that no problems occurred during the tree traversal
    if (fBad)
        return uint256();
    // verify that all bits were consumed (except for the padding caused by serializing it as a byte sequence)
    if ((nBitsUsed + 7) / 8 != (vBits.size() + 7) / 8)
        return uint256();
    // verify that all hashes were consumed
    if (nHashUsed != vHash.size())
        return uint256();
    return merkleRootHash;
}

bool AbortNode(const string &strMessage) {
    strMiscWarning = strMessage;
    LogPrint("INFO", "*** %s\n", strMessage);
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
    if (!pCdMan->pBlockTreeDb->LoadBlockIndexGuts())
        return false;

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
            (!pindexBestInvalid || pIndex->nChainWork > pindexBestInvalid->nChainWork))
            pindexBestInvalid = pIndex;
        if (pIndex->pprev)
            pIndex->BuildSkip();
    }

    // Load block file info
    pCdMan->pBlockTreeDb->ReadLastBlockFile(nLastBlockFile);
    LogPrint("INFO", "LoadBlockIndexDB(): last block file = %i\n", nLastBlockFile);
    if (pCdMan->pBlockTreeDb->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        LogPrint("INFO", "LoadBlockIndexDB(): last block file info: %s\n", infoLastBlockFile.ToString());

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    pCdMan->pBlockTreeDb->ReadReindexing(fReindexing);

    bool fCurReindex = SysCfg().IsReindex();
    SysCfg().SetReIndex(fCurReindex |= fReindexing);

    // Check whether we have a transaction index
    bool bTxIndex = SysCfg().IsTxIndex();
    pCdMan->pBlockTreeDb->ReadFlag("txindex", bTxIndex);
    SysCfg().SetTxIndex(bTxIndex);
    LogPrint("INFO", "LoadBlockIndexDB(): transaction index %s\n", bTxIndex ? "enabled" : "disabled");

    // Load pointer to end of best chain
    uint256 bestBlockHash = pCdMan->pAccountCache->GetBestBlock();
    const auto &it = mapBlockIndex.find(bestBlockHash);
    if (it == mapBlockIndex.end()) {
        return true;
    }

    chainActive.SetTip(it->second);
    LogPrint("INFO", "LoadBlockIndexDB(): hashBestChain=%s height=%d date=%s\n",
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
    LogPrint("INFO", "Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);

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

    LogPrint("INFO", "No coin database inconsistencies in last %i blocks (%i transactions)\n",
            chainActive.Height() - pIndexState->height, nGoodTransactions);

    return true;
}

void UnloadBlockIndex() {
    mapBlockIndex.clear();
    setBlockIndexValid.clear();
    chainActive.SetTip(nullptr);
    pindexBestInvalid = nullptr;
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
    pCdMan->pBlockTreeDb->WriteFlag("txindex", SysCfg().IsTxIndex());
    LogPrint("INFO", "Initializing databases...\n");

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
                LogPrint("INFO", "| ");
            LogPrint("INFO", "|\\\n");
        } else if (nCol < nPrevCol) {
            for (int32_t i = 0; i < nCol; i++)
                LogPrint("INFO", "| ");
            LogPrint("INFO", "|\n");
        }
        nPrevCol = nCol;

        // print columns
        for (int32_t i = 0; i < nCol; i++)
            LogPrint("INFO", "| ");

        // print item
        CBlock block;
        ReadBlockFromDisk(pIndex, block);
        LogPrint("INFO", "%d (blk%05u.dat:0x%x)  %s  tx %u\n",
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
            if (pCdMan->pBlockTreeDb->ReadBlockFileInfo(dbp->nFile, info)) {
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
                LogPrint("INFO", "%s : Deserialize or I/O error - %s", __func__, e.what());
            }
        }
        fclose(fileIn);
    } catch (runtime_error &e) {
        AbortNode(_("Error: system error: ") + e.what());
    }
    if (nLoaded > 0)
        LogPrint("INFO", "Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
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



bool static ProcessMessage(CNode *pFrom, string strCommand, CDataStream &vRecv)
{
    RandAddSeedPerfmon();
    LogPrint("net", "received: %s (%u bytes)\n", strCommand, vRecv.size());
    // if (GetRand(atoi(SysCfg().GetArg("-dropmessagestest", "0"))) == 0) {
    //     LogPrint("INFO", "dropmessagestest DROPPING RECV MESSAGE\n");
    //     return true;
    // }

    {
        LOCK(cs_main);
        State(pFrom->GetId())->nLastBlockProcess = GetTimeMicros();
    }

    if (strCommand == "version") {
        int32_t res = ProcessVersionMessage(pFrom,strCommand, vRecv);
        if(res !=-1)
            return res == 1 ;
    }

    else if (pFrom->nVersion == 0) {
        // Must have a version message before anything else
        Misbehaving(pFrom->GetId(), 1);
        return false;
    }

    else if (strCommand == "verack") {
        pFrom->SetRecvVersion(min(pFrom->nVersion, PROTOCOL_VERSION));
    }

    else if (strCommand == "addr") {
       if(!ProcessAddrMessage(pFrom, vRecv))
           return false ;
    }

    else if (strCommand == "inv") {
        if(!ProcessInvMessage(pFrom, vRecv))
            return false ;
    }

    else if (strCommand == "getdata") {
        if(!ProcessGetDataMessage(pFrom, vRecv))
            return false ;
    }

    else if (strCommand == "getblocks") {
        ProcessGetBlocksMessage(pFrom,vRecv) ;
    }

    else if (strCommand == "getheaders") {
       if(ProcessGetHeadersMessage(pFrom, vRecv))
           return true;
    }

    else if (strCommand == "tx") {
        if(!ProcessTxMessage(pFrom, strCommand , vRecv))
            return false ;
    }

    else if (strCommand == "block" && !SysCfg().IsImporting() && !SysCfg().IsReindex())  // Ignore blocks received while importing
    {
        ProcessBlockMessage(pFrom, vRecv);
    }

    else if (strCommand == "getaddr") {
        pFrom->vAddrToSend.clear();
        vector<CAddress> vAddr = addrman.GetAddr();
        for (const auto &addr : vAddr)
            pFrom->PushAddress(addr);
    }

    else if (strCommand == "mempool") {
        ProcessMempoolMessage(pFrom, vRecv);
    }

    else if (strCommand == "ping") {
        // Echo the message back with the nonce. This allows for two useful features:
        //
        // 1) A remote node can quickly check if the connection is operational
        // 2) Remote nodes can measure the latency of the network thread. If this node
        //    is overloaded it won't respond to pings quickly and the remote node can
        //    avoid sending us more work, like chain download requests.
        //
        // The nonce stops the remote getting confused between different pings: without
        // it, if the remote node sends a ping once per second and this node takes 5
        // seconds to respond to each, the 5th ping the remote sends would appear to
        // return very quickly.

        uint64_t nonce = 0;
        vRecv >> nonce;

        pFrom->PushMessage("pong", nonce);

    }

    else if (strCommand == "pong") {
       ProcessPongMessage(pFrom, vRecv) ;
    }

    else if (strCommand == "alert") {
        ProcessAlertMessage(pFrom, vRecv) ;
    }

    else if (strCommand == "filterload") {
        ProcessFilterLoadMessage(pFrom, vRecv);
    }

    else if (strCommand == "filteradd") {
        ProcessFilterAddMessage(pFrom, vRecv);
    }

    else if (strCommand == "filterclear") {
        LOCK(pFrom->cs_filter);
        delete pFrom->pfilter;
        pFrom->pfilter    = new CBloomFilter();
        pFrom->fRelayTxes = true;
    }

    else if (strCommand == "reject") {
        ProcessRejectMessage(pFrom, vRecv);
    } else {
        // Ignore unknown commands for extensibility
    }

    // Update the last seen time for this node's address
    if (pFrom->fNetworkNode)
        if (strCommand == "version" || strCommand == "addr" || strCommand == "inv" || strCommand == "getdata" || strCommand == "ping")
            AddressCurrentlyConnected(pFrom->addr);

    return true;
}

// requires LOCK(cs_vRecvMsg)
bool ProcessMessages(CNode *pFrom) {
    //if (fDebug)
    //    LogPrint("INFO","ProcessMessages(%u messages)\n", pFrom->vRecvMsg.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fOk = true;

    if (!pFrom->vRecvGetData.empty())
        ProcessGetData(pFrom);

    // this maintains the order of responses
    if (!pFrom->vRecvGetData.empty())
        return fOk;

    deque<CNetMessage>::iterator it = pFrom->vRecvMsg.begin();
    while (!pFrom->fDisconnect && it != pFrom->vRecvMsg.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pFrom->nSendSize >= SendBufferSize())
            break;

        // get next message
        CNetMessage &msg = *it;

        //if (fDebug)
        //    LogPrint("INFO","ProcessMessages(message %u msgsz, %u bytes, complete:%s)\n",
        //            msg.hdr.nMessageSize, msg.vRecv.size(),
        //            msg.complete() ? "Y" : "N");

        // end, if an incomplete message is found
        if (!msg.complete())
            break;

        // at this point, any failure means we can delete the current message
        it++;

        // Scan for message start
        if (memcmp(msg.hdr.pchMessageStart, SysCfg().MessageStart(), MESSAGE_START_SIZE) != 0) {
            LogPrint("INFO", "\n\nPROCESSMESSAGE: INVALID MESSAGESTART\n\n");
            fOk = false;
            break;
        }

        // Read header
        CMessageHeader &hdr = msg.hdr;
        if (!hdr.IsValid()) {
            LogPrint("INFO", "\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand());
            continue;
        }
        string strCommand = hdr.GetCommand();

        // Message size
        uint32_t nMessageSize = hdr.nMessageSize;

        // Checksum
        CDataStream &vRecv = msg.vRecv;
        uint256 hash       = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        uint32_t nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));
        if (nChecksum != hdr.nChecksum) {
            LogPrint("INFO", "ProcessMessages(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
                     strCommand, nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Process message
        bool fRet = false;
        try {
            fRet = ProcessMessage(pFrom, strCommand, vRecv);
            boost::this_thread::interruption_point();
        } catch (std::ios_base::failure &e) {
            pFrom->PushMessage("reject", strCommand, REJECT_MALFORMED, string("error parsing message"));
            if (strstr(e.what(), "end of data")) {
                // Allow exceptions from under-length message on vRecv
                LogPrint("INFO", "ProcessMessages(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand, nMessageSize, e.what());
                LogPrint("INFO", "ProcessMessages(%s, %u bytes) : %s\n", strCommand, nMessageSize, HexStr(vRecv.begin(), vRecv.end()).c_str());
            } else if (strstr(e.what(), "size too large")) {
                // Allow exceptions from over-long size
                LogPrint("INFO", "ProcessMessages(%s, %u bytes) : Exception '%s' caught\n", strCommand, nMessageSize, e.what());
            } else {
                PrintExceptionContinue(&e, "ProcessMessages()");
            }
        } catch (boost::thread_interrupted) {
            throw;
        } catch (std::exception &e) {
            PrintExceptionContinue(&e, "ProcessMessages()");
        } catch (...) {
            PrintExceptionContinue(nullptr, "ProcessMessages()");
        }

        if (!fRet)
            LogPrint("INFO", "ProcessMessage(%s, %u bytes) FAILED\n", strCommand, nMessageSize);

        break;
    }

    // In case the connection got shut down, its receive buffer was wiped
    if (!pFrom->fDisconnect)
        pFrom->vRecvMsg.erase(pFrom->vRecvMsg.begin(), it);

    return fOk;
}

bool SendMessages(CNode *pTo, bool fSendTrickle) {
    {
        // Don't send anything until we get their version message
        if (pTo->nVersion == 0)
            return true;

        //
        // Message: ping
        //
        bool pingSend = false;
        if (pTo->fPingQueued) {
            // RPC ping request by user
            pingSend = true;
        }
        if (pTo->nLastSend && GetTime() - pTo->nLastSend > 30 * 60 && pTo->vSendMsg.empty()) {
            // Ping automatically sent as a keepalive
            pingSend = true;
        }
        if (pingSend) {
            uint64_t nonce = 0;
            while (nonce == 0) {
                RAND_bytes((uint8_t *)&nonce, sizeof(nonce));
            }
            pTo->nPingNonceSent = nonce;
            pTo->fPingQueued    = false;
            //            if (pTo->nVersion > BIP0031_VERSION) {
            // Take timestamp as close as possible before transmitting ping
            pTo->nPingUsecStart = GetTimeMicros();
            pTo->PushMessage("ping", nonce);
            //            } else {
            //                // Peer is too old to support ping command with nonce, pong will never arrive, disable timing
            //                pTo->nPingUsecStart = 0;
            //                pTo->PushMessage("ping");
            //            }
        }

        TRY_LOCK(cs_main, lockMain);  // Acquire cs_main for IsInitialBlockDownload() and CNodeState()
        if (!lockMain)
            return true;

        // Address refresh broadcast
        static int64_t nLastRebroadcast;
        if (!IsInitialBlockDownload() && (GetTime() - nLastRebroadcast > 24 * 60 * 60)) {
            {
                LOCK(cs_vNodes);
                for (auto pNode : vNodes) {
                    // Periodically clear setAddrKnown to allow refresh broadcasts
                    if (nLastRebroadcast)
                        pNode->setAddrKnown.clear();

                    // Rebroadcast our address
                    if (!fNoListen) {
                        CAddress addr = GetLocalAddress(&pNode->addr);
                        if (addr.IsRoutable())
                            pNode->PushAddress(addr);
                    }
                }
            }
            nLastRebroadcast = GetTime();
        }

        //
        // Message: addr
        //
        if (fSendTrickle) {
            vector<CAddress> vAddr;
            vAddr.reserve(pTo->vAddrToSend.size());
            for (const auto &addr : pTo->vAddrToSend) {
                // returns true if wasn't already contained in the set
                if (pTo->setAddrKnown.insert(addr).second) {
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than 1000
                    if (vAddr.size() >= 1000) {
                        pTo->PushMessage("addr", vAddr);
                        vAddr.clear();
                    }
                }
            }
            pTo->vAddrToSend.clear();
            if (!vAddr.empty())
                pTo->PushMessage("addr", vAddr);
        }

        CNodeState &state = *State(pTo->GetId());
        if (state.fShouldBan) {
            if (pTo->addr.IsLocal()) {
                LogPrint("INFO", "Warning: not banning local node %s!\n", pTo->addr.ToString());
            }
            else {
                LogPrint("INFO", "Warning: banned a remote node %s!\n", pTo->addr.ToString());
                pTo->fDisconnect = true;
                CNode::Ban(pTo->addr);
            }
            state.fShouldBan = false;
        }

        for (const auto &reject : state.rejects)
            pTo->PushMessage("reject", (string) "block", reject.chRejectCode, reject.strRejectReason, reject.blockHash);
        state.rejects.clear();

        // Start block sync
        if (pTo->fStartSync && !SysCfg().IsImporting() && !SysCfg().IsReindex()) {
            pTo->fStartSync = false;
            nSyncTipHeight  = pTo->nStartingHeight;
            LogPrint("net", "start block sync lead to getblocks\n");
            PushGetBlocks(pTo, chainActive.Tip(), uint256());
        }

        // Resend wallet transactions that haven't gotten in a block yet
        // Except during reindex, importing and IBD, when old wallet
        // transactions become unconfirmed and spams other nodes.
        if (!SysCfg().IsReindex() && !SysCfg().IsImporting() && !IsInitialBlockDownload()) {
            g_signals.Broadcast();
        }

        //
        // Message: inventory
        //
        vector<CInv> vInv;
        vector<CInv> vInvWait;
        {
            LOCK(pTo->cs_inventory);
            vInv.reserve(pTo->vInventoryToSend.size());
            vInvWait.reserve(pTo->vInventoryToSend.size());
            for (const auto &inv : pTo->vInventoryToSend) {
                if (pTo->setInventoryKnown.count(inv))
                    continue;

                // trickle out tx inv to protect privacy
                if (inv.type == MSG_TX && !fSendTrickle) {
                    // 1/4 of tx invs blast to all immediately
                    static uint256 hashSalt;
                    if (hashSalt.IsNull())
                        hashSalt = GetRandHash();
                    uint256 hashRand  = ArithToUint256(UintToArith256(inv.hash) ^ UintToArith256(hashSalt));
                    hashRand          = Hash(BEGIN(hashRand), END(hashRand));
                    bool fTrickleWait = ((UintToArith256(hashRand) & 3) != 0);

                    if (fTrickleWait) {
                        vInvWait.push_back(inv);
                        continue;
                    }
                }

                // returns true if wasn't already contained in the set
                if (pTo->setInventoryKnown.insert(inv).second) {
                    vInv.push_back(inv);
                    if (vInv.size() >= 1000) {
                        pTo->PushMessage("inv", vInv);
                        vInv.clear();
                    }
                }
            }
            pTo->vInventoryToSend = vInvWait;
        }
        if (!vInv.empty())
            pTo->PushMessage("inv", vInv);

        // Detect stalled peers. Require that blocks are in flight, we haven't
        // received a (requested) block in one minute, and that all blocks are
        // in flight for over two minutes, since we first had a chance to
        // process an incoming block.
        int64_t nNow = GetTimeMicros();
        if (!pTo->fDisconnect && state.nBlocksInFlight &&
            state.nLastBlockReceive < state.nLastBlockProcess - BLOCK_DOWNLOAD_TIMEOUT * 1000000 &&
            state.vBlocksInFlight.front().nTime < state.nLastBlockProcess - 2 * BLOCK_DOWNLOAD_TIMEOUT * 1000000) {
            LogPrint("INFO", "Peer %s is stalling block download, disconnecting\n", state.name.c_str());
            pTo->fDisconnect = true;
        }

        //
        // Message: getdata (blocks)
        //
        vector<CInv> vGetData;
        int32_t index(0);
        while (!pTo->fDisconnect && state.nBlocksToDownload && state.nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            uint256 hash = state.vBlocksToDownload.front();
            vGetData.push_back(CInv(MSG_BLOCK, hash));
            MarkBlockAsInFlight(pTo->GetId(), hash);
            LogPrint("net", "Requesting block [%d] %s from %s, nBlocksInFlight=%d\n",
                     ++index, hash.ToString().c_str(), state.name.c_str(), state.nBlocksInFlight);
            if (vGetData.size() >= 1000) {
                pTo->PushMessage("getdata", vGetData);
                vGetData.clear();
                index = 0;
            }
        }

        //
        // Message: getdata (non-blocks)
        //
        while (!pTo->fDisconnect && !pTo->mapAskFor.empty() && (*pTo->mapAskFor.begin()).first <= nNow) {
            const CInv &inv = (*pTo->mapAskFor.begin()).second;
            if (!AlreadyHave(inv)) {
                LogPrint("net", "sending getdata: %s\n", inv.ToString());
                vGetData.push_back(inv);
                if (vGetData.size() >= 1000) {
                    pTo->PushMessage("getdata", vGetData);
                    vGetData.clear();
                }
            }
            pTo->mapAskFor.erase(pTo->mapAskFor.begin());
        }
        if (!vGetData.empty())
            pTo->PushMessage("getdata", vGetData);
    }
    return true;
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

        // orphan transactions
        mapOrphanTransactions.clear();
    }
} instance_of_cmaincleanup;

std::shared_ptr<CBaseTx> CreateNewEmptyTransaction(uint8_t txType) {
    switch (txType) {
        case BLOCK_REWARD_TX:
            return std::make_shared<CBlockRewardTx>();
        case ACCOUNT_REGISTER_TX:
            return std::make_shared<CAccountRegisterTx>();
        case BCOIN_TRANSFER_TX:
            return std::make_shared<CBaseCoinTransferTx>();
        case LCONTRACT_INVOKE_TX:
            return std::make_shared<CLuaContractInvokeTx>();
        case LCONTRACT_DEPLOY_TX:
            return std::make_shared<CLuaContractDeployTx>();
        case DELEGATE_VOTE_TX:
            return std::make_shared<CDelegateVoteTx>();

        case BCOIN_TRANSFER_MTX:
            return std::make_shared<CMulsigTx>();
        case FCOIN_STAKE_TX:
            return std::make_shared<CFcoinStakeTx>();

        case UCOIN_TRANSFER_TX:
            return std::make_shared<CCoinTransferTx>();
        case UCOIN_REWARD_TX:
            return std::make_shared<CCoinRewardTx>();
        case UCOIN_BLOCK_REWARD_TX:
            return std::make_shared<CUCoinBlockRewardTx>();
        case PRICE_FEED_TX:
            return std::make_shared<CPriceFeedTx>();
        case PRICE_MEDIAN_TX:
            return std::make_shared<CBlockPriceMedianTx>();

        case CDP_STAKE_TX:
            return std::make_shared<CCDPStakeTx>();
        case CDP_REDEEM_TX:
            return std::make_shared<CCDPRedeemTx>();
        case CDP_LIQUIDATE_TX:
            return std::make_shared<CCDPLiquidateTx>();

        case DEX_TRADE_SETTLE_TX:
            return std::make_shared<CDEXSettleTx>();
        case DEX_CANCEL_ORDER_TX:
            return std::make_shared<CDEXCancelOrderTx>();
        case DEX_LIMIT_BUY_ORDER_TX:
            return std::make_shared<CDEXBuyLimitOrderTx>();
        case DEX_LIMIT_SELL_ORDER_TX:
            return std::make_shared<CDEXSellLimitOrderTx>();
        case DEX_MARKET_BUY_ORDER_TX:
            return std::make_shared<CDEXBuyMarketOrderTx>();
        case DEX_MARKET_SELL_ORDER_TX:
            return std::make_shared<CDEXSellMarketOrderTx>();

        default:
            ERRORMSG("CreateNewEmptyTransaction type error");
            break;
    }
    return nullptr;
}

string CBlockUndo::ToString() const {
    string str;
    vector<CTxUndo>::const_iterator iterUndo = vtxundo.begin();
    for (; iterUndo != vtxundo.end(); ++iterUndo) {
        str += iterUndo->ToString();
    }
    return str;
}

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

FILE *OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly) {
    if (pos.IsNull())
        return nullptr;
    boost::filesystem::path path = GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
    boost::filesystem::create_directories(path.parent_path());
    FILE *file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file) {
        LogPrint("INFO", "Unable to open file %s\n", path.string());
        return nullptr;
    }
    if (pos.nPos) {
        if (fseek(file, pos.nPos, SEEK_SET)) {
            LogPrint("INFO", "Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return nullptr;
        }
    }
    return file;
}

FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "blk", fReadOnly);
}

FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "rev", fReadOnly);
}
