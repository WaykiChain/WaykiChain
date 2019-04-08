// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain Core developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include "configuration.h"

#include "addrman.h"
#include "alert.h"
#include "chainparams.h"

#include <sstream>
#include "init.h"
#include "miner.h"
#include "net.h"
#include "syncdatadb.h"
#include "tx.h"
#include "txdb.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "util.h"
#include "vm/vmrunenv.h"

#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <memory>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

using namespace json_spirit;
using namespace std;
using namespace boost;

#if defined(NDEBUG)
#error "Coin cannot be compiled without assertions."
#endif

#define LOG_CATEGORY_BENCH      "BENCH"     // log category: BENCH

#define MILLI                   0.001       // conversation rate: milli

//
// Global state
//

CCriticalSection cs_main;

CTxMemPool mempool;  //存放收到的未被执行,未收集到块的交易

map<uint256, CBlockIndex *> mapBlockIndex;
CChain chainActive;
CChain chainMostWork;
int nSyncTipHeight(0);  //同步时 ,chainActive.Tip()->nHeight

map<uint256, std::tuple<std::shared_ptr<CAccountViewCache>, std::shared_ptr<CTransactionDBCache>, std::shared_ptr<CScriptDBViewCache> > > mapCache;

CSignatureCache signatureCache;

/** Fees smaller than this (in sawi) are considered zero fee (for transaction creation) */
uint64_t CBaseTx::nMinTxFee = 10000;  // Override with -mintxfee
/** Fees smaller than this (in sawi) are considered zero fee (for relaying and mining) */
int64_t CBaseTx::nMinRelayTxFee = 1000;
/** Amout smaller than this (in sawi) is considered dust amount */
uint64_t CBaseTx::nDustAmountThreshold = 10000;
/** Amount of blocks that other nodes claim to have */
static CMedianFilter<int> cPeerBlockCounts(8, 0);

struct COrphanBlock {
    uint256 hashBlock;
    uint256 hashPrev;
    int height;
    vector<unsigned char> vchBlock;
};
map<uint256, COrphanBlock *> mapOrphanBlocks;             //存放因网络延迟等原因，收到的孤儿块
multimap<uint256, COrphanBlock *> mapOrphanBlocksByPrev;  //存放孤儿块的上一个块Hash及块

map<uint256, std::shared_ptr<CBaseTx> > mapOrphanTransactions;

const string strMessageMagic = "Coin Signed Message:\n";

// Internal stuff
namespace {
struct CBlockIndexWorkComparator {
    bool operator()(CBlockIndex *pa, CBlockIndex *pb) {
        // First sort by most total work, ...
        if (pa->nChainWork > pb->nChainWork) return false;
        if (pa->nChainWork < pb->nChainWork) return true;

        // ... then by earliest time received, ...
        if (pa->nSequenceId < pb->nSequenceId) return false;
        if (pa->nSequenceId > pb->nSequenceId) return true;

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those all have id 0).
        if (pa < pb) return false;
        if (pa > pb) return true;

        // Identical blocks.
        return false;
    }
};

CBlockIndex *pindexBestInvalid;
// may contain all CBlockIndex*'s that have validness >=BLOCK_VALID_TRANSACTIONS, and must contain those who aren't failed
set<CBlockIndex *, CBlockIndexWorkComparator> setBlockIndexValid;  //根据高度排序的有序集合

struct COrphanBlockComparator {
    bool operator()(COrphanBlock *pa, COrphanBlock *pb) {
        if (pa->height > pb->height) return false;
        if (pa->height < pb->height) return true;
        return false;
    }
};
set<COrphanBlock *, COrphanBlockComparator> setOrphanBlock;  //存的孤立块

CCriticalSection cs_LastBlockFile;
CBlockFileInfo infoLastBlockFile;
int nLastBlockFile = 0;

// Every received block is assigned a unique and increasing identifier, so we
// know which one to give priority in case of a fork.
CCriticalSection cs_nBlockSequenceId;
// Blocks loaded from disk are assigned id 0, so start the counter at 1.
uint32_t nBlockSequenceId = 1;

// Sources of received blocks, to be able to send them reject messages or ban
// them, if processing happens afterwards. Protected by cs_main.
map<uint256, NodeId> mapBlockSource;  // Remember who we got this block from.

// Blocks that are in flight, and that are in the queue to be downloaded.
// Protected by cs_main.
struct QueuedBlock {
    uint256 hash;
    int64_t nTime;      // Time of "getdata" request in microseconds.
    int nQueuedBefore;  // Number of blocks in flight at the time of request.
};
map<uint256, pair<NodeId, list<QueuedBlock>::iterator> > mapBlocksInFlight;
map<uint256, pair<NodeId, list<uint256>::iterator> > mapBlocksToDownload;  //存放待下载到的块，下载后执行erase

}  // namespace

//////////////////////////////////////////////////////////////////////////////
//
// dispatching functions
//

// These functions dispatch to one or all registered wallets

namespace {
struct CMainSignals {
    // Notifies listeners of updated transaction data (passing hash, transaction, and optionally the block it is found in.
    boost::signals2::signal<void(const uint256 &, CBaseTx *, const CBlock *)> SyncTransaction;
    // Notifies listeners of an erased transaction (currently disabled, requires transaction replacement).
    boost::signals2::signal<void(const uint256 &)> EraseTransaction;
    // Notifies listeners of an updated transaction without new data (for now: a coinbase potentially becoming visible).
    boost::signals2::signal<void(const uint256 &)> UpdatedTransaction;
    // Notifies listeners of a new active block chain.
    boost::signals2::signal<void(const CBlockLocator &)> SetBestChain;
    // Notifies listeners about an inventory item being seen on the network.
    // boost::signals2::signal<void (const uint256 &)> Inventory;
    // Tells listeners to broadcast their data.
    boost::signals2::signal<void()> Broadcast;
} g_signals;
}  // namespace

bool WriteBlockLog(bool falg, string suffix) {
    if (NULL == chainActive.Tip()) {
        return false;
    }
    char splitChar;
#ifdef WIN32
    splitChar = '\\';
#else
    splitChar = '/';
#endif

    boost::filesystem::path LogDirpath = GetDataDir() / "BlockLog";
    if (!falg) {
        LogDirpath = GetDataDir() / "BlockLog1";
    }
    if (!boost::filesystem::exists(LogDirpath)) {
        boost::filesystem::create_directory(LogDirpath);
    }

    ofstream file;
    int high              = chainActive.Height();
    string strLogFilePath = LogDirpath.string();
    strLogFilePath += splitChar + strprintf("%d_", high) + chainActive.Tip()->GetBlockHash().ToString();

    string strScriptLog = strLogFilePath + "_scriptDB_" + suffix + ".txt";
    file.open(strScriptLog);
    if (!file.is_open())
        return false;
    file << write_string(Value(pScriptDBTip->ToJsonObj()), true);
    file.close();

    string strAccountViewLog = strLogFilePath + "_AccountView_" + suffix + ".txt";
    file.open(strAccountViewLog);
    if (!file.is_open())
        return false;
    file << write_string(Value(pAccountViewTip->ToJsonObj()), true);
    file.close();

    string strCacheLog = strLogFilePath + "_Cache_" + suffix + ".txt";
    file.open(strCacheLog);
    if (!file.is_open())
        return false;
    file << write_string(Value(pTxCacheTip->ToJsonObj()), true);
    file.close();

    string strundoLog = strLogFilePath + "_undo.txt";
    file.open(strundoLog);
    if (!file.is_open())
        return false;
    CBlockUndo blockUndo;
    CDiskBlockPos pos = chainActive.Tip()->GetUndoPos();
    if (!pos.IsNull()) {
        if (blockUndo.ReadFromDisk(pos, chainActive.Tip()->pprev->GetBlockHash()))
            file << blockUndo.ToString();
    }

    file.close();
    return true;
}

void RegisterWallet(CWalletInterface *pwalletIn) {
    g_signals.SyncTransaction.connect(boost::bind(&CWalletInterface::SyncTransaction, pwalletIn, _1, _2, _3));
    g_signals.EraseTransaction.connect(boost::bind(&CWalletInterface::EraseFromWallet, pwalletIn, _1));
    g_signals.UpdatedTransaction.connect(boost::bind(&CWalletInterface::UpdatedTransaction, pwalletIn, _1));
    g_signals.SetBestChain.connect(boost::bind(&CWalletInterface::SetBestChain, pwalletIn, _1));
    //    g_signals.Inventory.connect(boost::bind(&CWalletInterface::Inventory, pwalletIn, _1));
    g_signals.Broadcast.connect(boost::bind(&CWalletInterface::ResendWalletTransactions, pwalletIn));
}

void UnregisterWallet(CWalletInterface *pwalletIn) {
    g_signals.Broadcast.disconnect(boost::bind(&CWalletInterface::ResendWalletTransactions, pwalletIn));
    //    g_signals.Inventory.disconnect(boost::bind(&CWalletInterface::Inventory, pwalletIn, _1));
    g_signals.SetBestChain.disconnect(boost::bind(&CWalletInterface::SetBestChain, pwalletIn, _1));
    g_signals.UpdatedTransaction.disconnect(boost::bind(&CWalletInterface::UpdatedTransaction, pwalletIn, _1));
    g_signals.EraseTransaction.disconnect(boost::bind(&CWalletInterface::EraseFromWallet, pwalletIn, _1));
    g_signals.SyncTransaction.disconnect(boost::bind(&CWalletInterface::SyncTransaction, pwalletIn, _1, _2, _3));
}

void UnregisterAllWallets() {
    g_signals.Broadcast.disconnect_all_slots();
    // g_signals.Inventory.disconnect_all_slots();
    g_signals.SetBestChain.disconnect_all_slots();
    g_signals.UpdatedTransaction.disconnect_all_slots();
    g_signals.EraseTransaction.disconnect_all_slots();
    g_signals.SyncTransaction.disconnect_all_slots();
}

void SyncWithWallets(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pblock) {
    g_signals.SyncTransaction(hash, pBaseTx, pblock);
}

void EraseTransaction(const uint256 &hash) {
    g_signals.EraseTransaction(hash);
}

//////////////////////////////////////////////////////////////////////////////
//
// Registration of network node signals.
//

namespace {

struct CBlockReject {
    unsigned char chRejectCode;
    string strRejectReason;
    uint256 hashBlock;
};

// Maintain validation-specific state about nodes, protected by cs_main, instead
// by CNode's own locks. This simplifies asynchronous operation, where
// processing of incoming data is done after the ProcessMessage call returns,
// and we're no longer holding the node's locks.
struct CNodeState {
    // Accumulated misbehaviour score for this peer.
    int nMisbehavior;
    // Whether this peer should be disconnected and banned.
    bool fShouldBan;
    // String name of this peer (debugging/logging purposes).
    string name;
    // List of asynchronously-determined block rejections to notify this peer about.
    vector<CBlockReject> rejects;
    list<QueuedBlock> vBlocksInFlight;
    int nBlocksInFlight;              //每个节点,单独能下载的最大块数量   MAX_BLOCKS_IN_TRANSIT_PER_PEER
    list<uint256> vBlocksToDownload;  //待下载的块
    int nBlocksToDownload;            //待下载的块个数
    int64_t nLastBlockReceive;        //上一次收到块的时间
    int64_t nLastBlockProcess;        //收到块，处理消息时的时间

    CNodeState() {
        nMisbehavior      = 0;
        fShouldBan        = false;
        nBlocksToDownload = 0;
        nBlocksInFlight   = 0;
        nLastBlockReceive = 0;
        nLastBlockProcess = 0;
    }
};

// Map maintaining per-node state. Requires cs_main.
map<NodeId, CNodeState> mapNodeState;

// Requires cs_main.
CNodeState *State(NodeId pnode) {
    map<NodeId, CNodeState>::iterator it = mapNodeState.find(pnode);
    if (it == mapNodeState.end())
        return NULL;
    return &it->second;
}

int GetHeight() {
    LOCK(cs_main);
    return chainActive.Height();
}

void InitializeNode(NodeId nodeid, const CNode *pnode) {
    LOCK(cs_main);
    CNodeState &state = mapNodeState.insert(make_pair(nodeid, CNodeState())).first->second;
    state.name        = pnode->addrName;
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
void MarkBlockAsReceived(const uint256 &hash, NodeId nodeFrom = -1) {
    map<uint256, pair<NodeId, list<uint256>::iterator> >::iterator itToDownload =
        mapBlocksToDownload.find(hash);
    if (itToDownload != mapBlocksToDownload.end()) {
        CNodeState *state = State(itToDownload->second.first);
        state->vBlocksToDownload.erase(itToDownload->second.second);
        state->nBlocksToDownload--;
        mapBlocksToDownload.erase(itToDownload);
    }

    map<uint256, pair<NodeId, list<QueuedBlock>::iterator> >::iterator itInFlight =
        mapBlocksInFlight.find(hash);
    if (itInFlight != mapBlocksInFlight.end()) {
        CNodeState *state = State(itInFlight->second.first);
        state->vBlocksInFlight.erase(itInFlight->second.second);
        state->nBlocksInFlight--;
        if (itInFlight->second.first == nodeFrom)
            state->nLastBlockReceive = GetTimeMicros();
        mapBlocksInFlight.erase(itInFlight);
    }
}

// Requires cs_main.
bool AddBlockToQueue(NodeId nodeid, const uint256 &hash) {
    if (mapBlocksToDownload.count(hash) || mapBlocksInFlight.count(hash)) {
        return false;
    }

    CNodeState *state = State(nodeid);
    if (state == NULL) {
        return false;
    }

    list<uint256>::iterator it =
        state->vBlocksToDownload.insert(state->vBlocksToDownload.end(), hash);
    state->nBlocksToDownload++;
    if (state->nBlocksToDownload > 5000) {
        LogPrint("INFO", "Misbehaving: AddBlockToQueue download too many times, nMisbehavior add 10\n");
        Misbehaving(nodeid, 10);
    }
    mapBlocksToDownload[hash] = make_pair(nodeid, it);
    return true;
}

// Requires cs_main.
void MarkBlockAsInFlight(NodeId nodeid, const uint256 &hash) {
    CNodeState *state = State(nodeid);
    assert(state != NULL);

    // Make sure it's not listed somewhere already.
    MarkBlockAsReceived(hash);

    QueuedBlock newentry = {hash, GetTimeMicros(), state->nBlocksInFlight};
    if (state->nBlocksInFlight == 0)
        state->nLastBlockReceive = newentry.nTime;  // Reset when a first request is sent.
    list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(), newentry);
    state->nBlocksInFlight++;
    mapBlocksInFlight[hash] = make_pair(nodeid, it);
}

}  // namespace

bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats) {
    LOCK(cs_main);
    CNodeState *state = State(nodeid);
    if (state == NULL)
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

CBlockIndex *CChain::SetTip(CBlockIndex *pindex) {
    if (pindex == NULL) {
        vChain.clear();
        return NULL;
    }
    vChain.resize(pindex->nHeight + 1);
    while (pindex && vChain[pindex->nHeight] != pindex) {
        vChain[pindex->nHeight] = pindex;
        pindex                  = pindex->pprev;
    }
    return pindex;
}

CBlockLocator CChain::GetLocator(const CBlockIndex *pindex) const {
    int nStep = 1;
    vector<uint256> vHave;
    vHave.reserve(32);

    if (!pindex)
        pindex = Tip();
    while (pindex) {
        vHave.push_back(pindex->GetBlockHash());
        // Stop when we have added the genesis block.
        if (pindex->nHeight == 0)
            break;
        // Exponentially larger steps back, plus the genesis block.
        int nHeight = max(pindex->nHeight - nStep, 0);
        // Jump back quickly to the same height as the chain.
        if (pindex->nHeight > nHeight)
            pindex = pindex->GetAncestor(nHeight);
        // In case pindex is not in this chain, iterate pindex->pprev to find blocks.
        while (!Contains(pindex))
            pindex = pindex->pprev;
        // If pindex is in this chain, use direct height-based access.
        if (pindex->nHeight > nHeight)
            pindex = (*this)[nHeight];
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
            CBlockIndex *pindex = (*mi).second;
            if (pindex && Contains(pindex))
                return pindex;
        }
    }
    return Genesis();
}

CAccountViewDB *pAccountViewDB     = NULL;
CBlockTreeDB *pblocktree           = NULL;
CAccountViewCache *pAccountViewTip = NULL;
CTransactionDB *pTxCacheDB         = NULL;
CTransactionDBCache *pTxCacheTip   = NULL;
CScriptDB *pScriptDB               = NULL;
CScriptDBViewCache *pScriptDBTip   = NULL;

unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans) {
    unsigned int nEvicted = 0;
    while (mapOrphanTransactions.size() > nMaxOrphans) {
        // Evict a random orphan:
        uint256 randomhash = GetRandHash();
        map<uint256, std::shared_ptr<CBaseTx> >::iterator it =
            mapOrphanTransactions.lower_bound(randomhash);
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
    unsigned int sz = ::GetSerializeSize(pBaseTx->GetNewInstance(), SER_NETWORK, CBaseTx::CURRENT_VERSION);
    if (sz >= MAX_STANDARD_TX_SIZE) {
        reason = "tx-size";
        return false;
    }

    return true;
}

bool IsFinalTx(CBaseTx *ptx, int nBlockHeight, int64_t nBlockTime) {
    AssertLockHeld(cs_main);
    return true;
}

int CMerkleTx::SetMerkleBranch(const CBlock *pblock) {
    AssertLockHeld(cs_main);
    CBlock blockTmp;

    if (pblock) {
        // Update the tx's hashBlock
        hashBlock = pblock->GetHash();

        // Locate the transaction
        for (nIndex = 0; nIndex < (int)pblock->vptx.size(); nIndex++)
            if ((pblock->vptx[nIndex])->GetHash() == pTx->GetHash())
                break;
        if (nIndex == (int)pblock->vptx.size()) {
            vMerkleBranch.clear();
            nIndex = -1;
            LogPrint("INFO", "ERROR: SetMerkleBranch() : couldn't find tx in block\n");
            return 0;
        }

        // Fill in merkle branch
        vMerkleBranch = pblock->GetMerkleBranch(nIndex);
    }

    // Is the tx in a block that's in the main chain
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex *pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    return chainActive.Height() - pindex->nHeight + 1;
}

bool CheckSignScript(const uint256 &sigHash, const std::vector<unsigned char> signature, const CPubKey pubKey) {
    int64_t nTimeStart = GetTimeMicros();
    if (signatureCache.Get(sigHash, signature, pubKey))
        return true;

    if (!pubKey.Verify(sigHash, signature))
        return false;

    signatureCache.Set(sigHash, signature, pubKey);
    int64_t nSpentTime = GetTimeMicros() - nTimeStart;
    LogPrint(LOG_CATEGORY_BENCH, "- Verify Signature with secp256k1: %.2fms\n", MILLI * nSpentTime);
    return true;
}

bool CheckTransaction(CBaseTx *ptx, CValidationState &state, CAccountViewCache &view,
                      CScriptDBViewCache &scriptDB)
{
    if (REWARD_TX == ptx->nTxType)
        return true;

    // Size limits
    if (::GetSerializeSize(ptx->GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, ERRORMSG("CheckTransaction() : size limits failed"),
            REJECT_INVALID, "bad-txns-oversize");

    if (!ptx->CheckTransaction(state, view, scriptDB))
        return false;

    return true;
}

int64_t GetMinRelayFee(const CBaseTx *pBaseTx, unsigned int nBytes, bool fAllowFree) {
    int64_t nBaseFee = pBaseTx->nMinRelayTxFee;
    int64_t nMinFee  = (1 + (int64_t)nBytes / 1000) * nBaseFee;

    if (fAllowFree) {
        // There is a free transaction area in blocks created by most miners,
        // * If we are relaying we allow transactions up to DEFAULT_BLOCK_PRIORITY_SIZE - 1000
        //   to be considered to fall into this category. We don't want to encourage sending
        //   multiple transactions instead of one big transaction to avoid fees.
        if (nBytes < (DEFAULT_BLOCK_PRIORITY_SIZE - 1000))
            nMinFee = 0;
    }

    if (!CheckMoneyRange(nMinFee))
        nMinFee = GetMaxMoney();

    return nMinFee;
}

bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx,
                        bool fLimitFree, bool fRejectInsaneFee) {
    AssertLockHeld(cs_main);

    // is it already in the memory pool?
    uint256 hash = pBaseTx->GetHash();
    if (pool.Exists(hash))
        return state.Invalid(
            ERRORMSG("AcceptToMemoryPool() : tx[%s] already in mempool", hash.GetHex()),
            REJECT_INVALID, "tx-already-in-mempool");

    // is it a miner reward tx?
    if (pBaseTx->IsCoinBase())
        return state.Invalid(
            ERRORMSG("AcceptToMemoryPool() : tx[%s] is a miner reward tx, can't put into mempool",
                     hash.GetHex()),
            REJECT_INVALID, "tx-coinbase-to-mempool");

    if (!CheckTransaction(pBaseTx, state, *pool.pAccountViewCache, *pool.pScriptDBViewCache))
        return ERRORMSG("AcceptToMemoryPool() : CheckTransaction failed");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    string reason;
    if (SysCfg().NetworkID() == MAIN_NET && !IsStandardTx(pBaseTx, reason))
        return state.DoS(0, ERRORMSG("AcceptToMemoryPool() : nonstandard transaction: %s", reason),
                         REJECT_NONSTANDARD, reason);

    {
        double dPriority = pBaseTx->GetPriority();
        int64_t nFees    = pBaseTx->GetFee();

        CTxMemPoolEntry entry(pBaseTx, nFees, GetTime(), dPriority, chainActive.Height());
        unsigned int nSize = entry.GetTxSize();

        if (pBaseTx->nTxType == COMMON_TX) {
            CCommonTx *pTx = static_cast<CCommonTx *>(pBaseTx);
            if (pTx->llValues < CBaseTx::nDustAmountThreshold)
                return state.DoS(0,
                                 ERRORMSG("AcceptToMemoryPool() : common tx %d transfer amount(%d) "
                                          "too small, you must send a min (%d)",
                                          hash.ToString(), pTx->llValues, CBaseTx::nDustAmountThreshold),
                                 REJECT_DUST, "dust amount");
        }

        int64_t txMinFee = GetMinRelayFee(pBaseTx, nSize, true);
        if (fLimitFree && nFees < txMinFee)
            return state.DoS(0,
                             ERRORMSG("AcceptToMemoryPool() : not enough fees %s, %d < %d",
                                      hash.ToString(), nFees, txMinFee),
                             REJECT_INSUFFICIENTFEE, "insufficient fee");

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
                return state.DoS(
                    0, ERRORMSG("AcceptToMemoryPool() : free transaction rejected by rate limiter"),
                    REJECT_INSUFFICIENTFEE, "insufficient priority");

            LogPrint("INFO", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount + nSize);
            dFreeCount += nSize;
        }

        if (fRejectInsaneFee && nFees > SysCfg().GetMaxFee())
            return ERRORMSG("AcceptToMemoryPool() : insane fees %s, %d > %d", hash.ToString(),
                            nFees, SysCfg().GetMaxFee());

        // Store transaction in memory
        if (!pool.AddUnchecked(hash, entry, state))
            return ERRORMSG("AcceptToMemoryPool() : AddUnchecked failed hash:%s\n",
                            hash.ToString());
    }

    g_signals.SyncTransaction(hash, pBaseTx, NULL);

    return true;
}

int CMerkleTx::GetDepthInMainChainINTERNAL(CBlockIndex *&pindexRet) const {
    if (hashBlock.IsNull() || nIndex == -1)
        return 0;
    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex *pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified) {
        if (CBlock::CheckMerkleBranch(pTx->GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    pindexRet = pindex;
    return chainActive.Height() - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChain(CBlockIndex *&pindexRet) const {
    AssertLockHeld(cs_main);
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.Exists(pTx->GetHash()))
        return -1;  // Not in chain, not in mempool

    return nResult;
}

int CMerkleTx::GetBlocksToMaturity() const {
    if (!pTx->IsCoinBase())
        return 0;
    return max(0, (COINBASE_MATURITY + 1) - GetDepthInMainChain());
}

int GetTxConfirmHeight(const uint256 &hash, CScriptDBViewCache &scriptDBCache) {
    if (SysCfg().IsTxIndex()) {
        CDiskTxPos postx;
        if (scriptDBCache.ReadTxIndex(hash, postx)) {
            CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
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

// Return transaction in tx, and if it was found inside a block, its hash is placed in hashBlock
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash,
                    CScriptDBViewCache &scriptDBCache, bool bSearchMemPool)
{
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
            CDiskTxPos postx;
            if (scriptDBCache.ReadTxIndex(hash, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
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
    unsigned int nSize = fileout.GetSerializeSize(block);
    fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

    // Write block
    long fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return ERRORMSG("WriteBlockToDisk : ftell failed");
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool ReadBlockFromDisk(CBlock &block, const CDiskBlockPos &pos)
{
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

bool ReadBlockFromDisk(CBlock &block, const CBlockIndex *pindex)
{
    if (!ReadBlockFromDisk(block, pindex->GetBlockPos()))
        return false;

    if (block.GetHash() != pindex->GetBlockHash())
        return ERRORMSG("ReadBlockFromDisk(CBlock&, CBlockIndex*) : GetHash() doesn't match index");

    return true;
}

uint256 static GetOrphanRoot(const uint256 &hash) {
    map<uint256, COrphanBlock *>::iterator it = mapOrphanBlocks.find(hash);
    if (it == mapOrphanBlocks.end())
        return hash;

    // Work back to the first block in the orphan chain
    do {
        map<uint256, COrphanBlock *>::iterator it2 = mapOrphanBlocks.find(it->second->hashPrev);
        if (it2 == mapOrphanBlocks.end())
            return it->first;
        it = it2;
    } while (true);
}

// Remove a random orphan block (which does not have any dependent orphans).
bool static PruneOrphanBlocks(int nHeight) {
    if (mapOrphanBlocksByPrev.size() <= MAX_ORPHAN_BLOCKS) {
        return true;
    }

    COrphanBlock *pOrphanBlock = *setOrphanBlock.rbegin();
    if (pOrphanBlock->height <= nHeight) {
        return false;
    }
    uint256 hash     = pOrphanBlock->hashBlock;
    uint256 prevHash = pOrphanBlock->hashPrev;
    setOrphanBlock.erase(pOrphanBlock);
    multimap<uint256, COrphanBlock *>::iterator beg = mapOrphanBlocksByPrev.lower_bound(prevHash);
    multimap<uint256, COrphanBlock *>::iterator end = mapOrphanBlocksByPrev.upper_bound(prevHash);
    while (beg != end) {
        if (beg->second->hashBlock == hash) {
            mapOrphanBlocksByPrev.erase(beg);
            break;
        }
        ++beg;
    }
    mapOrphanBlocks.erase(hash);
    delete pOrphanBlock;
    return true;
}

int64_t GetBlockValue(int nHeight, int64_t nFees) {
    int64_t nSubsidy = 50 * COIN;
    int halvings     = nHeight / SysCfg().GetSubsidyHalvingInterval();

    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64) {
        return nFees;
    }
    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;

    return nSubsidy + nFees;
}

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int ComputeMinWork(unsigned int nBase, int64_t nTime) {
    arith_uint256 bnLimit = SysCfg().ProofOfWorkLimit();
    // LogPrint("INFO", "bnLimit:%s\n", bnLimit.getuint256().GetHex());
    bool fNegative;
    bool fOverflow;

    arith_uint256 bnResult;
    bnResult.SetCompact(nBase, &fNegative, &fOverflow);
    bnResult *= 2;
    while (nTime > 0 && bnResult < bnLimit) {
        // Maximum 200% adjustment per day...
        bnResult *= 2;
        nTime -= 24 * 60 * 60;
    }
    if (fNegative || bnResult == 0 || fOverflow || bnResult > bnLimit)
        bnResult = bnLimit;

    return bnResult.GetCompact();
}

double CaculateDifficulty(unsigned int nBits) {
    int nShift = (nBits >> 24) & 0xff;

    double dDiff = (double)0x0000ffff / (double)(nBits & 0x00ffffff);

    while (nShift < 29) {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29) {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits) {
    // bool fNegative;
    // bool fOverflow;
    // arith_uint256 bnTarget;

    // bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // // Check range
    // if (fNegative || bnTarget == 0 || fOverflow || bnTarget > SysCfg().ProofOfWorkLimit())
    //     return ERRORMSG("CheckProofOfWork(): nBits below minimum work");

    // // Check proof of work matches claimed amount
    // if (UintToArith256(hash) > bnTarget) {
    //     LogPrint("INFO", "ac")
    //     return ERRORMSG("CheckProofOfWork(): hash doesn't match nBits");
    // }

    return true;
}

bool IsInitialBlockDownload() {
    LOCK(cs_main);
    if (SysCfg().IsImporting() || SysCfg().IsReindex() || chainActive.Height() < Checkpoints::GetTotalBlocksEstimate())
        return true;
    static int64_t nLastUpdate;
    static CBlockIndex *pindexLastBest;
    if (chainActive.Tip() != pindexLastBest) {
        pindexLastBest = chainActive.Tip();
        nLastUpdate    = GetTime();
    }
    return (GetTime() - nLastUpdate < 10 && chainActive.Tip()->GetBlockTime() < GetTime() - 24 * 60 * 60);
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
CBlockIndex *pindexBestForkTip = NULL, *pindexBestForkBase = NULL;

void CheckForkWarningConditions() {
    AssertLockHeld(cs_main);
    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before the last checkpoint)
    if (IsInitialBlockDownload())
        return;

    // If our best fork is no longer within 72 blocks (+/- 12 hours if no one mines it)
    // of our head, drop it
    if (pindexBestForkTip && chainActive.Height() - pindexBestForkTip->nHeight >= 72)
        pindexBestForkTip = NULL;

    if (pindexBestForkTip || (pindexBestInvalid && pindexBestInvalid->nChainWork > chainActive.Tip()->nChainWork + (GetBlockProof(*chainActive.Tip()) * 6))) {
        if (!fLargeWorkForkFound && pindexBestForkBase) {
            string strCmd = SysCfg().GetArg("-alertnotify", "");
            if (!strCmd.empty()) {
                string warning = string("'Warning: Large-work fork detected, forking after block ") +
                                 pindexBestForkBase->phashBlock->ToString() + string("'");
                boost::replace_all(strCmd, "%s", warning);
                boost::thread t(runCommand, strCmd);  // thread runs free
            }
        }
        if (pindexBestForkTip && pindexBestForkBase) {
            LogPrint("INFO", "CheckForkWarningConditions: Warning: Large valid fork found\n  forking the chain at height %d (%s)\n  lasting to height %d (%s).\nChain state database corruption likely.\n",
                     pindexBestForkBase->nHeight, pindexBestForkBase->phashBlock->ToString(),
                     pindexBestForkTip->nHeight, pindexBestForkTip->phashBlock->ToString());
            fLargeWorkForkFound = true;
        } else {
            LogPrint("INFO", "CheckForkWarningConditions: Warning: Found invalid chain at least ~6 blocks longer than our best chain.\nChain state database corruption likely.\n");
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
        while (plonger && plonger->nHeight > pfork->nHeight)
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
    if (pfork && (!pindexBestForkTip || (pindexBestForkTip && pindexNewForkTip->nHeight > pindexBestForkTip->nHeight)) &&
        pindexNewForkTip->nChainWork - pfork->nChainWork > (GetBlockProof(*pfork) * 7) &&
        chainActive.Height() - pindexNewForkTip->nHeight < 72) {
        pindexBestForkTip  = pindexNewForkTip;
        pindexBestForkBase = pfork;
    }

    CheckForkWarningConditions();
}

// Requires cs_main.
void Misbehaving(NodeId pnode, int howmuch) {
    if (howmuch == 0)
        return;

    CNodeState *state = State(pnode);
    if (state == NULL)
        return;

    state->nMisbehavior += howmuch;
    if (state->nMisbehavior >= SysCfg().GetArg("-banscore", 100)) {
        LogPrint("INFO", "Misbehaving: %s (%d -> %d) BAN THRESHOLD EXCEEDED\n",
                 state->name, state->nMisbehavior - howmuch, state->nMisbehavior);
        state->fShouldBan = true;
    } else {
        LogPrint("INFO", "Misbehaving: %s (%d -> %d)\n",
                 state->name, state->nMisbehavior - howmuch, state->nMisbehavior);
    }
}

void static InvalidChainFound(CBlockIndex *pindexNew) {
    if (!pindexBestInvalid || pindexNew->nChainWork > pindexBestInvalid->nChainWork) {
        pindexBestInvalid = pindexNew;
        // The current code doesn't actually read the BestInvalidWork entry in
        // the block database anymore, as it is derived from the flags in block
        // index entry. We only write it for backward compatibility.
        pblocktree->WriteBestInvalidWork(ArithToUint256(pindexBestInvalid->nChainWork));
        uiInterface.NotifyBlocksChanged(pindexNew->GetBlockTime(), chainActive.Height(),
                                        chainActive.Tip()->GetBlockHash());
    }
    LogPrint("INFO", "InvalidChainFound: invalid block=%s  height=%d  log2_work=%.8g  date=%s\n",
             pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
             log(pindexNew->nChainWork.getdouble()) / log(2.0),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pindexNew->GetBlockTime()));
    LogPrint("INFO", "InvalidChainFound:  current best=%s  height=%d  log2_work=%.8g  date=%s\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
             log(chainActive.Tip()->nChainWork.getdouble()) / log(2.0),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()));
    CheckForkWarningConditions();
}

void static InvalidBlockFound(CBlockIndex *pindex, const CValidationState &state) {
    int nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        map<uint256, NodeId>::iterator it = mapBlockSource.find(pindex->GetBlockHash());
        if (it != mapBlockSource.end() && State(it->second)) {
            CBlockReject reject = {state.GetRejectCode(), state.GetRejectReason(), pindex->GetBlockHash()};
            State(it->second)->rejects.push_back(reject);
            if (nDoS > 0) {
                LogPrint("INFO", "Misebehaving: found invalid block, hash:%s, Misbehavior add %d",
                         it->first.GetHex(), nDoS);
                Misbehaving(it->second, nDoS);
            }
        }
    }
    if (!state.CorruptionPossible()) {
        pindex->nStatus |= BLOCK_FAILED_VALID;
        pblocktree->WriteBlockIndex(CDiskBlockIndex(pindex));
        setBlockIndexValid.erase(pindex);
        InvalidChainFound(pindex);
    }
}

bool InvalidateBlock(CValidationState &state, CBlockIndex *pindex) {
    AssertLockHeld(cs_main);

    // Mark the block itself as invalid.
    pindex->nStatus |= BLOCK_FAILED_VALID;
    pblocktree->WriteBlockIndex(CDiskBlockIndex(pindex));
    setBlockIndexValid.erase(pindex);

    LogPrint("INFO", "Invalidate block[%d]: %s BLOCK_FAILED_VALID\n",
             pindex->nHeight, pindex->GetBlockHash().ToString());

    while (chainActive.Contains(pindex)) {
        CBlockIndex *pindexWalk = chainActive.Tip();
        pindexWalk->nStatus |= BLOCK_FAILED_CHILD;
        pblocktree->WriteBlockIndex(CDiskBlockIndex(pindexWalk));
        setBlockIndexValid.erase(pindexWalk);

        LogPrint("INFO", "Invalidate block[%d]: %s BLOCK_FAILED_CHILD\n",
                 pindexWalk->nHeight, pindexWalk->GetBlockHash().ToString());

        // ActivateBestChain considers blocks already in chainActive
        // unconditionally valid already, so force disconnect away from it.
        if (!DisconnectBlockFromTip(state)) {
            return false;
        }
    }

    InvalidChainFound(pindex);
    return true;
}

bool ReconsiderBlock(CValidationState &state, CBlockIndex *pindex) {
    AssertLockHeld(cs_main);

    // Remove the invalidity flag from this block and all its descendants.
    map<uint256, CBlockIndex *>::const_iterator it = mapBlockIndex.begin();
    int nHeight                                    = pindex->nHeight;
    while (it != mapBlockIndex.end()) {
        if (it->second->nStatus & BLOCK_FAILED_MASK && it->second->GetAncestor(nHeight) == pindex) {
            it->second->nStatus &= ~BLOCK_FAILED_MASK;
            pblocktree->WriteBlockIndex(CDiskBlockIndex(it->second));
            setBlockIndexValid.insert(it->second);
            if (it->second == pindexBestInvalid) {
                // Reset invalid block marker if it was pointing to one of those.
                pindexBestInvalid = NULL;
            }
        }
        it++;
    }

    // Remove the invalidity flag from all ancestors too.
    while (pindex != NULL) {
        if (pindex->nStatus & BLOCK_FAILED_MASK) {
            pindex->nStatus &= ~BLOCK_FAILED_MASK;
            setBlockIndexValid.insert(pindex);
            pblocktree->WriteBlockIndex(CDiskBlockIndex(pindex));
        }
        pindex = pindex->pprev;
    }
    return true;
}

void UpdateTime(CBlockHeader &block, const CBlockIndex *pindexPrev) {
    block.SetTime(max(pindexPrev->GetMedianTimePast() + 1, GetAdjustedTime()));
}

bool DisconnectBlock(CBlock &block, CValidationState &state, CAccountViewCache &view,
                     CBlockIndex *pindex, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache, bool *pfClean) {
    assert(pindex->GetBlockHash() == view.GetBestBlock());

    if (pfClean)
        *pfClean = false;

    bool fClean = true;

    CBlockUndo blockUndo;
    CDiskBlockPos pos = pindex->GetUndoPos();
    if (pos.IsNull())
        return ERRORMSG("DisconnectBlock() : no undo data available");
    if (!blockUndo.ReadFromDisk(pos, pindex->pprev->GetBlockHash()))
        return ERRORMSG("DisconnectBlock() : failure reading undo data");

    if ((blockUndo.vtxundo.size() != block.vptx.size()) &&
        (blockUndo.vtxundo.size() != (block.vptx.size() + 1)))
        return ERRORMSG("DisconnectBlock() : block and undo data inconsistent");

    //    LogPrint("INFO","height= %d\n,%s", pindex->nHeight,blockUndo.ToString());
    //    int64_t llTime = GetTimeMillis();
    CTxUndo txundo;
    if (pindex->nHeight - COINBASE_MATURITY > 0) {
        //undo mature reward tx
        txundo = blockUndo.vtxundo.back();
        blockUndo.vtxundo.pop_back();
        CBlockIndex *pMatureIndex = pindex;
        for (int i = 0; i < COINBASE_MATURITY; ++i) {
            pMatureIndex = pMatureIndex->pprev;
        }
        if (NULL != pMatureIndex) {
            CBlock matureBlock;
            if (!ReadBlockFromDisk(matureBlock, pMatureIndex)) {
                return state.DoS(100, ERRORMSG("ConnectBlock() : read mature block error"),
                                 REJECT_INVALID, "bad-read-block");
            }
            if (!matureBlock.vptx[0]->UndoExecuteTx(-1, view, state, txundo, pindex->nHeight,
                                                    txCache, scriptCache))
                return ERRORMSG("ConnectBlock() : execure mature block reward tx error!");
        }
    }

    //undo reward tx
    std::shared_ptr<CBaseTx> pBaseTx = block.vptx[0];
    txundo                                    = blockUndo.vtxundo.back();
    LogPrint("undo_account", "tx Hash:%s\n", pBaseTx->GetHash().ToString());
    if (!pBaseTx->UndoExecuteTx(0, view, state, txundo, pindex->nHeight, txCache, scriptCache))
        return false;
    //  LogPrint("INFO", "reward tx undo elapse:%lld ms\n", GetTimeMillis() - llTime);

    // undo transactions in reverse order
    for (int i = block.vptx.size() - 1; i >= 1; i--) {
        //      llTime = GetTimeMillis();
        std::shared_ptr<CBaseTx> pBaseTx = block.vptx[i];
        CTxUndo txundo                            = blockUndo.vtxundo[i - 1];
        LogPrint("undo_account", "tx Hash:%s\n", pBaseTx->GetHash().ToString());
        if (!pBaseTx->UndoExecuteTx(i, view, state, txundo, pindex->nHeight, txCache, scriptCache))
            return false;
        //  LogPrint("INFO", "tx type:%d,undo elapse:%lld ms\n", pBaseTx->nTxType, GetTimeMillis() - llTime);
    }
    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());

    if (!txCache.DeleteBlockFromCache(block))
        return state.Abort(_("Disconnect tip block failed to delete tx from txcache"));

    //load a block tx into cache transaction
    if (pindex->nHeight - SysCfg().GetTxCacheHeight() > 0) {
        CBlockIndex *pReLoadBlockIndex = pindex;
        int nCacheHeight               = SysCfg().GetTxCacheHeight();
        while (pReLoadBlockIndex && nCacheHeight-- > 0) {
            pReLoadBlockIndex = pReLoadBlockIndex->pprev;
        }
        CBlock reLoadblock;
        if (!ReadBlockFromDisk(reLoadblock, pReLoadBlockIndex))
            return state.Abort(_("Failed to read block"));
        if (!txCache.AddBlockToCache(reLoadblock))
            return state.Abort(_("Disconnect tip block reload preblock tx to txcache"));
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

bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize);

bool ConnectBlock(CBlock &block, CValidationState &state, CAccountViewCache &view, CBlockIndex *pindex,
                  CTransactionDBCache &txCache, CScriptDBViewCache &scriptDBCache, bool fJustCheck) {
    AssertLockHeld(cs_main);
    // Check it again in case a previous version let a bad block in
    if (block.GetHash() != SysCfg().HashGenesisBlock() && !CheckBlock(block, state, view, scriptDBCache,
                                                                      !fJustCheck, !fJustCheck))
        return false;

    if (!fJustCheck) {
        // verify that the view's current state corresponds to the previous block
        uint256 hashPrevBlock = pindex->pprev == NULL ? uint256() : pindex->pprev->GetBlockHash();
        if (hashPrevBlock != view.GetBestBlock()) {
            LogPrint("INFO", "hashPrevBlock=%s, bestblock=%s\n",
                     hashPrevBlock.GetHex(), view.GetBestBlock().GetHex());
            assert(hashPrevBlock == view.GetBestBlock());
        }
    }

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == SysCfg().HashGenesisBlock()) {
        view.SetBestBlock(pindex->GetBlockHash());
        for (unsigned int i = 1; i < block.vptx.size(); i++) {
            if (block.vptx[i]->nTxType == REWARD_TX) {
                assert(i <= 1);
                std::shared_ptr<CRewardTx> pRewardTx =
                    dynamic_pointer_cast<CRewardTx>(block.vptx[i]);
                CAccount sourceAccount;
                CRegID accountId(pindex->nHeight, i);
                CPubKey pubKey      = boost::get<CPubKey>(pRewardTx->account);
                CKeyID keyId        = pubKey.GetKeyID();
                sourceAccount.keyID = keyId;
                sourceAccount.SetRegId(accountId);
                sourceAccount.pubKey = pubKey;
                sourceAccount.llValues  = pRewardTx->rewardValue;
                assert(view.SaveAccountInfo(accountId, keyId, sourceAccount));
            } else if (block.vptx[i]->nTxType == DELEGATE_TX) {
                std::shared_ptr<CDelegateTx> pDelegateTx =
                    dynamic_pointer_cast<CDelegateTx>(block.vptx[i]);
                assert(pDelegateTx->userId.type() == typeid(CRegID));
                CAccount voteAcct;
                assert(view.GetAccount(pDelegateTx->userId, voteAcct));
                uint64_t maxVotes = 0;
                CScriptDBOperLog operDbLog;
                int j = i;
                for (auto &operFund : pDelegateTx->operVoteFunds) {
                    assert(operFund.operType == ADD_FUND);
                    if (operFund.fund.value > maxVotes) {
                        maxVotes = operFund.fund.value;
                    }
                    if (voteAcct.pubKey == operFund.fund.pubKey) {
                        voteAcct.llVotes = operFund.fund.value;
                        assert(scriptDBCache.SetDelegateData(voteAcct, operDbLog));
                    } else {
                        CAccount delegateAcct;
                        assert(!view.GetAccount(operFund.fund.pubKey, delegateAcct));
                        CRegID delegateRegId(pindex->nHeight, j++);
                        delegateAcct.keyID = operFund.fund.pubKey.GetKeyID();
                        delegateAcct.SetRegId(delegateRegId);
                        delegateAcct.pubKey = operFund.fund.pubKey;
                        delegateAcct.llVotes   = operFund.fund.value;
                        assert(view.SaveAccountInfo(delegateRegId, delegateAcct.keyID, delegateAcct));
                        assert(scriptDBCache.SetDelegateData(delegateAcct, operDbLog));
                    }
                    voteAcct.vVoteFunds.push_back(operFund.fund);
                    sort(voteAcct.vVoteFunds.begin(), voteAcct.vVoteFunds.end(),
                         [](CVoteFund fund1, CVoteFund fund2) {
                             return fund1.value > fund2.value;
                         });
                }
                assert(voteAcct.llValues >= maxVotes);
                voteAcct.llValues -= maxVotes;
                assert(view.SaveAccountInfo(voteAcct.regID, voteAcct.keyID, voteAcct));
            }
        }
        return true;
    }

    if (!VerifyPosTx(&block, view, txCache, scriptDBCache, false)) {
        return state.DoS(100, ERRORMSG("ConnectBlock() : the block Hash=%s check pos tx error", block.GetHash().GetHex()), REJECT_INVALID, "bad-pos-tx");
    }

    CBlockUndo blockundo;
    int64_t nStart = GetTimeMicros();
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vptx.size()));
    std::vector<pair<uint256, CDiskTxPos> > vPos;
    vPos.reserve(block.vptx.size());

    //push reward pos
    vPos.push_back(make_pair(block.GetTxHash(0), pos));
    pos.nTxOffset += ::GetSerializeSize(block.vptx[0], SER_DISK, CLIENT_VERSION);

    LogPrint("op_account", "block height:%d block hash:%s\n", block.GetHeight(), block.GetHash().GetHex());
    uint64_t nTotalRunStep(0);
    int64_t nTotalFuel(0);
    if (block.vptx.size() > 1) {
        for (unsigned int i = 1; i < block.vptx.size(); i++) {
            std::shared_ptr<CBaseTx> pBaseTx = block.vptx[i];
            if (uint256() != txCache.HasTx((pBaseTx->GetHash()))) {
                return state.DoS(100,
                                 ERRORMSG("ConnectBlock() : the TxHash %s the confirm duplicate",
                                          pBaseTx->GetHash().GetHex()),
                                 REJECT_INVALID, "bad-cb-amount");
            }
            assert(mapBlockIndex.count(view.GetBestBlock()));
            if (!pBaseTx->IsValidHeight(mapBlockIndex[view.GetBestBlock()]->nHeight,
                                        SysCfg().GetTxCacheHeight())) {
                return state.DoS(100, ERRORMSG("ConnectBlock() : txhash=%s beyond the scope of valid height",
                    pBaseTx->GetHash().GetHex()), REJECT_INVALID, "tx-invalid-height");
            }

            if (CONTRACT_TX == pBaseTx->nTxType) {
                LogPrint("vm", "tx hash=%s ConnectBlock run contract\n", pBaseTx->GetHash().GetHex());
            }
            LogPrint("op_account", "tx index:%d tx hash:%s\n", i, pBaseTx->GetHash().GetHex());
            CTxUndo txundo;
            pBaseTx->nFuelRate = block.GetFuelRate();
            if (!pBaseTx->ExecuteTx(i, view, state, txundo, pindex->nHeight, txCache, scriptDBCache)) {
                return false;
            }

            nTotalRunStep += pBaseTx->nRunStep;
            if (nTotalRunStep > MAX_BLOCK_RUN_STEP) {
                return state.DoS(100, ERRORMSG("block hash=%s total run steps exceed max run step", block.GetHash().GetHex()), REJECT_INVALID, "exeed-max_step");
            }
            uint64_t llFuel = ceil(pBaseTx->nRunStep / 100.f) * block.GetFuelRate();
            if (REG_CONT_TX == pBaseTx->nTxType) {
                if (llFuel < 1 * COIN) {
                    llFuel = 1 * COIN;
                }
            }
            nTotalFuel += llFuel;
            LogPrint("fuel", "connect block total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txhash:%s \n",
                     nTotalFuel, llFuel, pBaseTx->nRunStep, block.GetFuelRate(), pBaseTx->GetHash().GetHex());
            vPos.push_back(make_pair(block.GetTxHash(i), pos));
            pos.nTxOffset += ::GetSerializeSize(pBaseTx, SER_DISK, CLIENT_VERSION);
            blockundo.vtxundo.push_back(txundo);
        }

        if (nTotalFuel != block.GetFuel()) {
            return ERRORMSG("fuel value at block header calculate error(actual fuel:%ld vs block fuel:%ld)",
                            nTotalFuel, block.GetFuel());
        }
    }

    std::shared_ptr<CRewardTx> pRewardTx = dynamic_pointer_cast<CRewardTx>(block.vptx[0]);
    CAccount minerAcct;
    if (!view.GetAccount(pRewardTx->account, minerAcct)) {
        assert(0);
    }
    // LogPrint("INFO", "miner address=%s\n", minerAcct.keyID.ToAddress());
    //校验reward
    uint64_t llValidReward = block.GetFee() - block.GetFuel();
    if (pRewardTx->rewardValue != llValidReward) {
        LogPrint("INFO", "block fee:%lld, block fuel:%lld\n", block.GetFee(), block.GetFuel());
        return state.DoS(100, ERRORMSG("ConnectBlock() : coinbase pays too much (actual=%d vs limit=%d)", pRewardTx->rewardValue, llValidReward), REJECT_INVALID, "bad-cb-amount");
    }
    //deal reward tx
    LogPrint("op_account", "tx index:%d tx hash:%s\n", 0, block.vptx[0]->GetHash().GetHex());
    CTxUndo txundo;
    if (!block.vptx[0]->ExecuteTx(0, view, state, txundo, pindex->nHeight, txCache, scriptDBCache))
        return ERRORMSG("ConnectBlock() : execute reward tx error!");
    blockundo.vtxundo.push_back(txundo);

    if (pindex->nHeight - COINBASE_MATURITY > 0) {
        //deal mature reward tx
        //CBlockIndex *pMatureIndex = chainActive[pindex->nHeight - COINBASE_MATURITY];
        CBlockIndex *pMatureIndex = pindex;
        for (int i = 0; i < COINBASE_MATURITY; ++i) {
            pMatureIndex = pMatureIndex->pprev;
        }
        if (NULL != pMatureIndex) {
            CBlock matureBlock;
            if (!ReadBlockFromDisk(matureBlock, pMatureIndex)) {
                return state.DoS(100, ERRORMSG("ConnectBlock() : read mature block error"),
                                 REJECT_INVALID, "bad-read-block");
            }
            if (!matureBlock.vptx[0]->ExecuteTx(-1, view, state, txundo, pindex->nHeight, txCache, scriptDBCache))
                return ERRORMSG("ConnectBlock() : execute mature block reward tx error!");
        }
        blockundo.vtxundo.push_back(txundo);
    }
    int64_t nTime = GetTimeMicros() - nStart;
    if (SysCfg().IsBenchmark())
        LogPrint("INFO", "- Connect %u transactions: %.2fms (%.3fms/tx)\n",
                 (unsigned)block.vptx.size(), 0.001 * nTime, 0.001 * nTime / block.vptx.size());

    if (fJustCheck)
        return true;

    if (SysCfg().IsTxIndex()) {
        LogPrint("txindex", " add tx index, block hash:%s\n", pindex->GetBlockHash().GetHex());
        vector<CScriptDBOperLog> vTxIndexOperDB;
        if (!scriptDBCache.WriteTxIndex(vPos, vTxIndexOperDB))
            return state.Abort(_("Failed to write transaction index"));
        auto itTxUndo = blockundo.vtxundo.rbegin();
        itTxUndo->vScriptOperLog.insert(itTxUndo->vScriptOperLog.begin(), vTxIndexOperDB.begin(),
                                        vTxIndexOperDB.end());
    }

    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull() || (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS) {
        if (pindex->GetUndoPos().IsNull()) {
            CDiskBlockPos pos;
            if (!FindUndoPos(state, pindex->nFile, pos, ::GetSerializeSize(blockundo, SER_DISK, CLIENT_VERSION) + 40))
                return ERRORMSG("ConnectBlock() : FindUndoPos failed");
            if (!blockundo.WriteToDisk(pos, pindex->pprev->GetBlockHash()))
                return state.Abort(_("Failed to write undo data"));

            // update nUndoPos in block index
            pindex->nUndoPos = pos.nPos;
            pindex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pindex->nStatus = (pindex->nStatus & ~BLOCK_VALID_MASK) | BLOCK_VALID_SCRIPTS;

        CDiskBlockIndex blockindex(pindex);
        if (!pblocktree->WriteBlockIndex(blockindex))
            return state.Abort(_("Failed to write block index"));
    }

    if (!txCache.AddBlockToCache(block))
        return state.Abort(_("Connect tip block failed add block tx to txcache"));

    if (pindex->nHeight - SysCfg().GetTxCacheHeight() > 0) {
        CBlockIndex *pDeleteBlockIndex = pindex;
        int nCacheHeight               = SysCfg().GetTxCacheHeight();
        while (pDeleteBlockIndex && nCacheHeight-- > 0) {
            pDeleteBlockIndex = pDeleteBlockIndex->pprev;
        }
        CBlock deleteBlock;
        if (!ReadBlockFromDisk(deleteBlock, pDeleteBlockIndex))
            return state.Abort(_("Failed to read block"));
        if (!txCache.DeleteBlockFromCache(deleteBlock))
            return state.Abort(_("Connect tip block failed delete block tx to txcache"));
    }

    // add this block to the view's block chain
    assert(view.SetBestBlock(pindex->GetBlockHash()));
    return true;
}

// Update the on-disk chain state.
bool static WriteChainState(CValidationState &state) {
    static int64_t nLastWrite = 0;
    unsigned int cachesize    = pAccountViewTip->GetCacheSize() + pScriptDBTip->GetCacheSize();
    if (!IsInitialBlockDownload() || cachesize > SysCfg().GetViewCacheSize() || GetTimeMicros() > nLastWrite + 600 * 1000000) {
        // Typical CCoins structures on disk are around 100 bytes in size.
        // Pushing a new one to the database can cause it to be written
        // twice (once in the log, and once in the tables). This is already
        // an overestimation, as most will delete an existing entry or
        // overwrite one. Still, use a conservative safety factor of 2.
        if (!CheckDiskSpace(cachesize))
            return state.Error("out of disk space");

        FlushBlockFile();
        pblocktree->Sync();
        if (!pAccountViewTip->Flush())
            return state.Abort(_("Failed to write to account database"));
        if (!pTxCacheTip->Flush())
            return state.Abort(_("Failed to write to tx cache database"));
        if (!pScriptDBTip->Flush())
            return state.Abort(_("Failed to write to script db database"));
        mapCache.clear();
        nLastWrite = GetTimeMicros();
    }
    return true;
}

// Update chainActive and related internal data structures.
void static UpdateTip(CBlockIndex *pindexNew, const CBlock &block) {
    chainActive.SetTip(pindexNew);

    SyncWithWallets(uint256(), NULL, &block);

    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = IsInitialBlockDownload();
    if ((chainActive.Height() % 20160) == 0 || (!fIsInitialDownload && (chainActive.Height() % 144) == 0))
        g_signals.SetBestChain(chainActive.GetLocator());

    // New best block
    SysCfg().SetBestRecvTime(GetTime());
    mempool.AddTransactionsUpdated(1);
    LogPrint("INFO", "UpdateTip: new best=%s  height=%d  log2_work=%.8g  tx=%lu  date=%s progress=%f txnumber=%d dFeePerKb=%lf nFuelRate=%d\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(), log(chainActive.Tip()->nChainWork.getdouble()) / log(2.0), (unsigned long)chainActive.Tip()->nChainTx,
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
             Checkpoints::GuessVerificationProgress(chainActive.Tip()), block.vptx.size(), chainActive.Tip()->dFeePerKb, chainActive.Tip()->nFuelRate);
    LogPrint("updatetip", "UpdateTip: new best=%s  height=%d  log2_work=%.8g  tx=%lu  date=%s progress=%f txnumber=%d dFeePerKb=%lf nFuelRate=%d difficulty=%.8lf\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(), log(chainActive.Tip()->nChainWork.getdouble()) / log(2.0), (unsigned long)chainActive.Tip()->nChainTx,
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
             Checkpoints::GuessVerificationProgress(chainActive.Tip()), block.vptx.size(), chainActive.Tip()->dFeePerKb, chainActive.Tip()->nFuelRate, CaculateDifficulty(chainActive.Tip()->nBits));
    // Check the version of the last 100 blocks to see if we need to upgrade:
    if (!fIsInitialDownload) {
        int nUpgraded             = 0;
        const CBlockIndex *pindex = chainActive.Tip();
        for (int i = 0; i < 100 && pindex != NULL; i++) {
            if (pindex->nVersion > CBlock::CURRENT_VERSION)
                ++nUpgraded;
            pindex = pindex->pprev;
        }
        if (nUpgraded > 0)
            LogPrint("INFO", "SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int)CBlock::CURRENT_VERSION);
        if (nUpgraded > 100 / 2)
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning = _("Warning: This version is obsolete, upgrade required!");
    }
}

// Disconnect chainActive's tip.
bool static DisconnectTip(CValidationState &state) {
    CBlockIndex *pindexDelete = chainActive.Tip();
    assert(pindexDelete);
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(block, pindexDelete))
        return state.Abort(_("Failed to read blocks from disk."));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CAccountViewCache view(*pAccountViewTip, true);
        CScriptDBViewCache scriptDBView(*pScriptDBTip, true);
        if (!DisconnectBlock(block, state, view, pindexDelete, *pTxCacheTip, scriptDBView, NULL))
            return ERRORMSG("DisconnectTip() : DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
        assert(view.Flush() && scriptDBView.Flush());
    }
    if (SysCfg().IsBenchmark())
        LogPrint("INFO", "- Disconnect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;
    // Update chainActive and related variables.
    UpdateTip(pindexDelete->pprev, block);
    // Resurrect mempool transactions from the disconnected block.
    for (const auto &ptx : block.vptx) {
        list<std::shared_ptr<CBaseTx> > removed;
        CValidationState stateDummy;
        if (!ptx->IsCoinBase()) {
            if (!AcceptToMemoryPool(mempool, stateDummy, ptx.get(), false)) {
                mempool.Remove(ptx.get(), removed, true);
            } else
                uiInterface.ReleaseTransaction(ptx->GetHash());
        } else {
            uiInterface.RemoveTransaction(ptx->GetHash());
            EraseTransaction(ptx->GetHash());
        }
    }

    if (SysCfg().GetArg("-blocklog", 0) != 0) {
        if (chainActive.Height() % SysCfg().GetArg("-blocklog", 0) == 0) {
            if (!pAccountViewTip->Flush())
                return state.Abort(_("Failed to write to account database"));
            if (!pTxCacheTip->Flush())
                return state.Abort(_("Failed to write to tx cache database"));
            if (!pScriptDBTip->Flush())
                return state.Abort(_("Failed to write to script db database"));
            WriteBlockLog(true, "DisConnectTip");
        }
    }
    return true;
}

void PrintInfo(const uint256 &hash, const int &nCurHeight, CScriptDBViewCache &scriptDBView,
               const string &scriptId) {
    vector<unsigned char> vScriptKey;
    vector<unsigned char> vScriptData;
    int nHeight;
    set<CScriptDBOperLog> setOperLog;
    CRegID regId(scriptId);
    int nCount(0);
    scriptDBView.GetContractItemCount(scriptId, nCount);
    bool ret = scriptDBView.GetContractData(nCurHeight, regId, 0, vScriptKey, vScriptData);
    LogPrint("scriptdbview", "\n\n\n");
    LogPrint("scriptdbview", "blockhash=%s,curHeight=%d\n", hash.GetHex(), nCurHeight);
    LogPrint("scriptdbview", "app script ID:%s key:%s value:%s height:%d, nCount:%d\n",
             scriptId.c_str(), HexStr(vScriptKey), HexStr(vScriptData), nHeight, nCount);
    while (ret) {
        ret = scriptDBView.GetContractData(nCurHeight, regId, 1, vScriptKey, vScriptData);
        scriptDBView.GetContractItemCount(scriptId, nCount);
        if (ret)
            LogPrint("scriptdbview", "app script ID:%s key:%s value:%s height:%d, nCount:%d\n",
                     scriptId.c_str(), HexStr(vScriptKey), HexStr(vScriptData), nHeight, nCount);
    }
}
// Connect a new block to chainActive.
bool static ConnectTip(CValidationState &state, CBlockIndex *pindexNew) {
    assert(pindexNew->pprev == chainActive.Tip());
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(block, pindexNew))
        return state.Abort(strprintf("Failed to read block hash:%s\n",
                                     pindexNew->GetBlockHash().GetHex()));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CInv inv(MSG_BLOCK, pindexNew->GetBlockHash());
        CAccountViewCache view(*pAccountViewTip, true);
        CScriptDBViewCache scriptDBView(*pScriptDBTip, true);
        if (!ConnectBlock(block, state, view, pindexNew, *pTxCacheTip, scriptDBView)) {
            if (state.IsInvalid())
                InvalidBlockFound(pindexNew, state);
            return ERRORMSG("ConnectTip() : ConnectBlock %s failed", pindexNew->GetBlockHash().ToString());
        }
        mapBlockSource.erase(inv.hash);
        assert(view.Flush() && scriptDBView.Flush());
        CAccountViewCache viewtemp(*pAccountViewTip, true);
        uint256 uBestblockHash = viewtemp.GetBestBlock();
        LogPrint("INFO", "uBestBlockHash[%d]: %s\n", nSyncTipHeight, uBestblockHash.GetHex());
    }
    if (SysCfg().IsBenchmark())
        LogPrint("INFO", "- Connect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);

    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;

    // Update chainActive & related variables.
    UpdateTip(pindexNew, block);

    // Write new block info to log, if necessary.
    if (SysCfg().GetArg("-blocklog", 0) != 0) {
        if (chainActive.Height() % SysCfg().GetArg("-blocklog", 0) == 0) {
            if (!pAccountViewTip->Flush())
                return state.Abort(_("Failed to write to account database"));
            if (!pTxCacheTip->Flush())
                return state.Abort(_("Failed to write to tx cache database"));
            if (!pScriptDBTip->Flush())
                return state.Abort(_("Failed to write to script db database"));
            WriteBlockLog(true, "ConnectTip");
        }
    }

    for (auto &pTxItem : block.vptx) {
        mempool.mapTx.erase(pTxItem->GetHash());
    }
    return true;
}

// Make chainMostWork correspond to the chain with the most work in it, that isn't
// known to be invalid (it's however far from certain to be valid).
void static FindMostWorkChain() {
    CBlockIndex *pindexNew = NULL;

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
            pindexNew = *it;
        }

        // Check whether all blocks on the path between the currently active chain and the candidate are valid.
        // Just going until the active chain is an optimization, as we know all blocks in it are valid already.
        CBlockIndex *pindexTest = pindexNew;
        bool fInvalidAncestor   = false;
        while (pindexTest && !chainActive.Contains(pindexTest)) {
            if (pindexTest->nStatus & BLOCK_FAILED_MASK) {
                // Candidate has an invalid ancestor, remove entire chain from the set.
                if (pindexBestInvalid == NULL || pindexNew->nChainWork > pindexBestInvalid->nChainWork)
                    pindexBestInvalid = pindexNew;
                CBlockIndex *pindexFailed = pindexNew;
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
    if (chainMostWork.Tip() && !CBlockIndexWorkComparator()(chainMostWork.Tip(), pindexNew))
        return;

    // We have a new best.
    chainMostWork.SetTip(pindexNew);
}

// Try to activate to the most-work chain (thereby connecting it).
bool ActivateBestChain(CValidationState &state) {
    LOCK(cs_main);
    CBlockIndex *pindexOldTip = chainActive.Tip();
    bool fComplete            = false;
    while (!fComplete) {
        FindMostWorkChain();
        fComplete = true;

        // Check whether we have something to do.
        if (chainMostWork.Tip() == NULL) break;

        // Disconnect active blocks which are no longer in the best chain.
        while (chainActive.Tip() && !chainMostWork.Contains(chainActive.Tip())) {
            if (!DisconnectTip(state))
                return false;
            if (chainActive.Tip() && chainMostWork.Contains(chainActive.Tip())) {
                mempool.ReScanMemPoolTx(pAccountViewTip, pScriptDBTip);
            }
        }

        // Connect new blocks.
        while (!chainActive.Contains(chainMostWork.Tip())) {
            CBlockIndex *pindexConnect = chainMostWork[chainActive.Height() + 1];
            if (!ConnectTip(state, pindexConnect)) {
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
                mempool.ReScanMemPoolTx(pAccountViewTip, pScriptDBTip);
            }
        }
    }

    if (chainActive.Tip() != pindexOldTip) {
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
    CBlockIndex *pindexNew = new CBlockIndex(block);
    assert(pindexNew);
    {
        LOCK(cs_nBlockSequenceId);
        pindexNew->nSequenceId = nBlockSequenceId++;
    }
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    // LogPrint("INFO", "in map hash:%s map size:%d\n", hash.GetHex(), mapBlockIndex.size());
    pindexNew->phashBlock                        = &((*mi).first);
    map<uint256, CBlockIndex *>::iterator miPrev = mapBlockIndex.find(block.GetHashPrevBlock());
    if (miPrev != mapBlockIndex.end()) {
        pindexNew->pprev   = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
        pindexNew->BuildSkip();
    }
    pindexNew->nTx        = block.vptx.size();
    pindexNew->nChainWork = pindexNew->nHeight;
    pindexNew->nChainTx   = (pindexNew->pprev ? pindexNew->pprev->nChainTx : 0) + pindexNew->nTx;
    pindexNew->nFile      = pos.nFile;
    pindexNew->nDataPos   = pos.nPos;
    pindexNew->nUndoPos   = 0;
    pindexNew->nStatus    = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
    setBlockIndexValid.insert(pindexNew);

    if (!pblocktree->WriteBlockIndex(CDiskBlockIndex(pindexNew)))
        return state.Abort(_("Failed to write block index"));
    int64_t tempTime = GetTimeMillis();
    // New best?
    if (!ActivateBestChain(state)) {
        LogPrint("INFO", "ActivateBestChain() elapse time:%lld ms\n", GetTimeMillis() - tempTime);
        return false;
    }
    // LogPrint("INFO", "ActivateBestChain() elapse time:%lld ms\n", GetTimeMillis() - tempTime);
    LOCK(cs_main);
    if (pindexNew == chainActive.Tip()) {
        // Clear fork warning if its no longer applicable
        CheckForkWarningConditions();
        // Notify UI to display prev block's coinbase if it was ours
        static uint256 hashPrevBestCoinBase;
        g_signals.UpdatedTransaction(hashPrevBestCoinBase);
        hashPrevBestCoinBase = block.GetTxHash(0);
    } else
        CheckForkWarningConditionsOnNewFork(pindexNew);

    if (!pblocktree->Flush())
        return state.Abort(_("Failed to sync block index"));

    if (chainActive.Tip()->nHeight > nSyncTipHeight)
        nSyncTipHeight = chainActive.Tip()->nHeight;
    uiInterface.NotifyBlocksChanged(pindexNew->GetBlockTime(), chainActive.Height(),
                                    chainActive.Tip()->GetBlockHash());
    return true;
}

bool FindBlockPos(CValidationState &state, CDiskBlockPos &pos, unsigned int nAddSize, unsigned int nHeight, uint64_t nTime, bool fKnown = false) {
    bool fUpdatedLast = false;

    LOCK(cs_LastBlockFile);

    if (fKnown) {
        if (nLastBlockFile != pos.nFile) {
            nLastBlockFile = pos.nFile;
            infoLastBlockFile.SetNull();
            pblocktree->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);
            fUpdatedLast = true;
        }
    } else {
        while (infoLastBlockFile.nSize + nAddSize >= MAX_BLOCKFILE_SIZE) {
            LogPrint("INFO", "Leaving block file %i: %s\n", nLastBlockFile, infoLastBlockFile.ToString());
            FlushBlockFile(true);
            nLastBlockFile++;
            infoLastBlockFile.SetNull();
            pblocktree->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);  // check whether data for the new file somehow already exist; can fail just fine
            fUpdatedLast = true;
        }
        pos.nFile = nLastBlockFile;
        pos.nPos  = infoLastBlockFile.nSize;
    }

    infoLastBlockFile.nSize += nAddSize;
    infoLastBlockFile.AddBlock(nHeight, nTime);

    if (!fKnown) {
        unsigned int nOldChunks = (pos.nPos + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        unsigned int nNewChunks = (infoLastBlockFile.nSize + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
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

    if (!pblocktree->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        return state.Abort(_("Failed to write file info"));
    if (fUpdatedLast)
        pblocktree->WriteLastBlockFile(nLastBlockFile);

    return true;
}

bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize) {
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    unsigned int nNewSize;
    if (nFile == nLastBlockFile) {
        pos.nPos = infoLastBlockFile.nUndoSize;
        nNewSize = (infoLastBlockFile.nUndoSize += nAddSize);
        if (!pblocktree->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
            return state.Abort(_("Failed to write block info"));
    } else {
        CBlockFileInfo info;
        if (!pblocktree->ReadBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to read block info"));
        pos.nPos = info.nUndoSize;
        nNewSize = (info.nUndoSize += nAddSize);
        if (!pblocktree->WriteBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to write block info"));
    }

    unsigned int nOldChunks = (pos.nPos + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    unsigned int nNewChunks = (nNewSize + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
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

bool CheckBlockProofWorkWithCoinDay(const CBlock &block, CBlockIndex *pPreBlockIndex, CValidationState &state)
{
    std::shared_ptr<CAccountViewCache>      pForkAcctViewCache;
    std::shared_ptr<CTransactionDBCache>    pForkTxCache;
    std::shared_ptr<CScriptDBViewCache>     pForkScriptDBCache;
    std::shared_ptr<CAccountViewCache>      pAcctViewCache;

    pAcctViewCache                  = std::make_shared<CAccountViewCache>(*pAccountViewDB, true);
    pAcctViewCache->cacheAccounts   = pAccountViewTip->cacheAccounts;
    pAcctViewCache->cacheKeyIds     = pAccountViewTip->cacheKeyIds;
    pAcctViewCache->hashBlock       = pAccountViewTip->hashBlock;

    std::shared_ptr<CTransactionDBCache> pTxCache = std::make_shared<CTransactionDBCache>(*pTxCacheDB, true);
    pTxCache->SetCacheMap(pTxCacheTip->GetCacheMap());

    std::shared_ptr<CScriptDBViewCache> pScriptDBCache = std::make_shared<CScriptDBViewCache>(*pScriptDB, true);
    pScriptDBCache->mapContractDb                      = pScriptDBTip->mapContractDb;

    uint256 preBlockHash;
    bool bFindForkChainTip(false);
    vector<CBlock> vPreBlocks;
    if (pPreBlockIndex->GetBlockHash() != chainActive.Tip()->GetBlockHash()) {
        while (!chainActive.Contains(pPreBlockIndex)) {
            if (!bFindForkChainTip && mapCache.count(pPreBlockIndex->GetBlockHash()) > 0) {
                preBlockHash = pPreBlockIndex->GetBlockHash();
                LogPrint("INFO", "ForkChainTip hash=%s, height=%d\n", pPreBlockIndex->GetBlockHash().GetHex(),
                    pPreBlockIndex->nHeight);
                bFindForkChainTip = true;
            }

            if (!bFindForkChainTip) {
                CBlock block;
                if (!ReadBlockFromDisk(block, pPreBlockIndex))
                    return state.Abort(_("Failed to read block"));

                vPreBlocks.push_back(block);  //将支链的block保存起来
            }

            pPreBlockIndex = pPreBlockIndex->pprev;
            // if (chainActive.Tip()->nHeight - pPreBlockIndex->nHeight > SysCfg().GetMaxForkHeight())
            //     return state.DoS(100, ERRORMSG("CheckBlockProofWorkWithCoinDay() : block at fork chain too earlier than tip block hash=%s block height=%d\n",
            //         block.GetHash().GetHex(), block.GetHeight()));

            map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(pPreBlockIndex->GetBlockHash());
            if (mi == mapBlockIndex.end())
                return state.DoS(10, ERRORMSG("CheckBlockProofWorkWithCoinDay() : prev block not found"), 0, "bad-prevblk");
        }  //如果进来的preblock hash不为tip的hash,找到主链中分叉处

        int64_t tempTime = GetTimeMillis();
        if (mapCache.count(pPreBlockIndex->GetBlockHash()) > 0) {
            LogPrint("INFO", "hash=%s, height=%d\n", pPreBlockIndex->GetBlockHash().GetHex(), pPreBlockIndex->nHeight);
            pAcctViewCache = std::get<0>(mapCache[pPreBlockIndex->GetBlockHash()]);
            pTxCache       = std::get<1>(mapCache[pPreBlockIndex->GetBlockHash()]);
            pScriptDBCache = std::get<2>(mapCache[pPreBlockIndex->GetBlockHash()]);
        } else {
            CBlockIndex *pBlockIndex = chainActive.Tip();
            while (pPreBlockIndex != pBlockIndex) {  //数据库状态回滚到主链分叉处
                LogPrint("INFO", "CheckBlockProofWorkWithCoinDay() DisconnectBlock block nHeight=%d hash=%s\n",
                         pBlockIndex->nHeight, pBlockIndex->GetBlockHash().GetHex());
                CBlock block;
                if (!ReadBlockFromDisk(block, pBlockIndex))
                    return state.Abort(_("Failed to read block"));

                bool bfClean = true;
                if (!DisconnectBlock(block, state, *pAcctViewCache, pBlockIndex, *pTxCache, *pScriptDBCache, &bfClean))
                    return ERRORMSG("CheckBlockProofWorkWithCoinDay() : DisconnectBlock %s failed",
                        pBlockIndex->GetBlockHash().ToString());

                pBlockIndex = pBlockIndex->pprev;
            }
            std::tuple
                <std::shared_ptr<CAccountViewCache>,
                std::shared_ptr<CTransactionDBCache>,
                std::shared_ptr<CScriptDBViewCache> > forkCache = std::make_tuple(pAcctViewCache, pTxCache, pScriptDBCache);
            LogPrint("INFO", "add mapCache Key:%s height:%d\n", pPreBlockIndex->GetBlockHash().GetHex(), pPreBlockIndex->nHeight);
            LogPrint("INFO", "add pAcctViewCache:%x \n", pAcctViewCache.get());
            LogPrint("INFO", "view best block hash:%s \n", pAcctViewCache->GetBestBlock().GetHex());
            mapCache[pPreBlockIndex->GetBlockHash()] = forkCache;
        }

        LogPrint("INFO", "CheckBlockProofWorkWithCoinDay() DisconnectBlock elapse :%lld ms\n", GetTimeMillis() - tempTime);
        if (bFindForkChainTip) {
            pForkAcctViewCache = std::get<0>(mapCache[preBlockHash]);
            pForkTxCache       = std::get<1>(mapCache[preBlockHash]);
            pForkScriptDBCache = std::get<2>(mapCache[preBlockHash]);
            pForkAcctViewCache->SetBaseData(pAcctViewCache.get());
            pForkTxCache->SetBaseData(pTxCache.get());
            pForkScriptDBCache->SetBaseData(pScriptDBCache.get());
        } else {
            pForkAcctViewCache.reset(new CAccountViewCache(*pAcctViewCache, true));
            pForkTxCache.reset(new CTransactionDBCache(*pTxCache, true));
            pForkScriptDBCache.reset(new CScriptDBViewCache(*pScriptDBCache, true));
        }

        LogPrint("INFO", "pForkAcctView:%x\n", pForkAcctViewCache.get());
        LogPrint("INFO", "view best block hash:%s height:%d\n",
            pForkAcctViewCache->GetBestBlock().GetHex(), mapBlockIndex[pForkAcctViewCache->GetBestBlock()]->nHeight);

        vector<CBlock>::reverse_iterator rIter = vPreBlocks.rbegin();
        for (; rIter != vPreBlocks.rend(); ++rIter) {  //连接支链的block
            LogPrint("INFO", "CheckBlockProofWorkWithCoinDay() ConnectBlock block nHeight=%d hash=%s\n",
                     rIter->GetHeight(), rIter->GetHash().GetHex());

            if (!ConnectBlock(*rIter, state, *pForkAcctViewCache, mapBlockIndex[rIter->GetHash()],
                *pForkTxCache, *pForkScriptDBCache, false))
                return ERRORMSG("CheckBlockProofWorkWithCoinDay() : ConnectBlock %s failed", rIter->GetHash().ToString());

            CBlockIndex *pConnBlockIndex = mapBlockIndex[rIter->GetHash()];
            if (pConnBlockIndex->nStatus | BLOCK_FAILED_MASK)
                pConnBlockIndex->nStatus = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
        }

        //校验pos交易
        if (!VerifyPosTx(&block, *pForkAcctViewCache, *pForkTxCache, *pForkScriptDBCache, true)) {
            return state.DoS(100,
                ERRORMSG("CheckBlockProofWorkWithCoinDay() : the block Hash=%s check pos tx error",
                block.GetHash().GetHex()), REJECT_INVALID, "bad-pos-tx");
        }

        //校验利息是否正常
        std::shared_ptr<CRewardTx> pRewardTx = dynamic_pointer_cast<CRewardTx>(block.vptx[0]);
        uint64_t llValidReward                        = block.GetFee() - block.GetFuel();
        if (pRewardTx->rewardValue != llValidReward)
            return state.DoS(100, ERRORMSG("CheckBlockProofWorkWithCoinDay() : coinbase pays too much (actual=%d vs limit=%d)",
                pRewardTx->rewardValue, llValidReward), REJECT_INVALID, "bad-cb-amount");

        for (auto &item : block.vptx) {
            //校验交易是否在有效高度
            if (!item->IsValidHeight(mapBlockIndex[pForkAcctViewCache->GetBestBlock()]->nHeight, SysCfg().GetTxCacheHeight())) {
                return state.DoS(100, ERRORMSG("CheckBlockProofWorkWithCoinDay() : txhash=%s beyond the scope of valid height\n ",
                    item->GetHash().GetHex()), REJECT_INVALID, "tx-invalid-height");
            }
            //校验是否有重复确认交易
            if (uint256() != pForkTxCache->HasTx(item->GetHash()))
                return state.DoS(100, ERRORMSG("CheckBlockProofWorkWithCoinDay() : tx hash %s has been confirmed\n",
                    item->GetHash().GetHex()), REJECT_INVALID, "bad-txns-oversize");
        }

        if (!vPreBlocks.empty()) {
            vector<CBlock>::iterator iterBlock = vPreBlocks.begin();
            if (bFindForkChainTip) {
                LogPrint("INFO", "delete mapCache Key:%s\n", preBlockHash.GetHex());
                mapCache.erase(preBlockHash);
            }
            std::tuple<std::shared_ptr<CAccountViewCache>, std::shared_ptr<CTransactionDBCache>,
                std::shared_ptr<CScriptDBViewCache> > cache = std::make_tuple(pForkAcctViewCache, pForkTxCache, pForkScriptDBCache);
            LogPrint("INFO", "add mapCache Key:%s\n", iterBlock->GetHash().GetHex());
            mapCache[iterBlock->GetHash()] = cache;
        }
    } else {
        return true;
    }
    return true;
}

bool CheckBlock(const CBlock &block, CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDBCache, bool fCheckTx, bool fCheckMerkleRoot) {
    if (block.vptx.empty() || block.vptx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, ERRORMSG("CheckBlock() : size limits failed"),
            REJECT_INVALID, "bad-blk-length");

    if (block.GetHash() != SysCfg().HashGenesisBlock() && block.GetVersion() != CBlockHeader::CURRENT_VERSION)
        return state.Invalid(ERRORMSG("CheckBlock() : block version must be set 3"),
            REJECT_INVALID, "block-version-error");

    // Check timestamp 12minutes limits
    if (block.GetBlockTime() > GetAdjustedTime() + 12 * 60)
        return state.Invalid(ERRORMSG("CheckBlock() : block timestamp too far in the future"),
                             REJECT_INVALID, "time-too-new");

    // First transaction must be coinbase, the rest must not be
    if (block.vptx.empty() || !block.vptx[0]->IsCoinBase())
        return state.DoS(100, ERRORMSG("CheckBlock() : first tx is not coinbase"),
                         REJECT_INVALID, "bad-cb-missing");

    // Build the merkle tree already. We need it anyway later, and it makes the
    // block cache the transaction hashes, which means they don't need to be
    // recalculated many times during this block's validation.
    block.BuildMerkleTree();

    // Check for duplicate txids. This is caught by ConnectInputs(),
    // but catching it earlier avoids a potential DoS attack:
    set<uint256> uniqueTx;
    for (unsigned int i = 0; i < block.vptx.size(); i++) {
        uniqueTx.insert(block.GetTxHash(i));

        if (fCheckTx && !CheckTransaction(block.vptx[i].get(), state, view, scriptDBCache))
            return ERRORMSG("CheckBlock() :tx hash:%s CheckTransaction failed", block.vptx[i]->GetHash().GetHex());

        if (block.GetHash() != SysCfg().HashGenesisBlock()) {
            if (0 != i && block.vptx[i]->IsCoinBase())
                return state.DoS(100, ERRORMSG("CheckBlock() : more than one coinbase"), REJECT_INVALID, "bad-cb-multiple");
        }
    }

    if (uniqueTx.size() != block.vptx.size())
        return state.DoS(100, ERRORMSG("CheckBlock() : duplicate transaction"),
                         REJECT_INVALID, "bad-txns-duplicate", true);

    // Check merkle root
    if (fCheckMerkleRoot && block.GetHashMerkleRoot() != block.vMerkleTree.back())
        return state.DoS(100, ERRORMSG("CheckBlock() : hashMerkleRoot mismatch, block.hashMerkleRoot=%s, block.vMerkleTree.back()=%s", block.GetHashMerkleRoot().ToString(), block.vMerkleTree.back().ToString()),
                         REJECT_INVALID, "bad-txnmrklroot", true);

    //check nonce
    uint64_t maxNonce = SysCfg().GetBlockMaxNonce();  //cacul times
    if (block.GetNonce() > maxNonce) {
        return state.Invalid(ERRORMSG("CheckBlock() : Nonce is larger than maxNonce"),
                             REJECT_INVALID, "Nonce-too-large");
    }

    return true;
}

bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = block.GetHash();
    LogPrint("INFO", "AcceptBlock[%d]: %s\n", block.GetHeight(), hash.GetHex());
    if (mapBlockIndex.count(hash))
        return state.Invalid(ERRORMSG("AcceptBlock() : block already in mapBlockIndex"), 0, "duplicated");

    assert(block.GetHash() == SysCfg().HashGenesisBlock() || mapBlockIndex.count(block.GetHashPrevBlock()));
    if (block.GetHash() != SysCfg().HashGenesisBlock() &&
        block.GetFuelRate() != GetElementForBurn(mapBlockIndex[block.GetHashPrevBlock()]))
        return state.DoS(100, ERRORMSG("CheckBlock() : block fuel rate unmatched"), REJECT_INVALID, "fuel-rate-unmatch");

    // Get prev block index
    CBlockIndex *pBlockIndexPrev = NULL;
    int nHeight = 0;
    if (hash != SysCfg().HashGenesisBlock()) {
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(block.GetHashPrevBlock());
        if (mi == mapBlockIndex.end())
            return state.DoS(10, ERRORMSG("AcceptBlock() : prev block not found"), 0, "bad-prevblk");

        pBlockIndexPrev = (*mi).second;
        nHeight = pBlockIndexPrev->nHeight + 1;

        if (block.GetHeight() != (unsigned int)nHeight)
            return state.DoS(100, ERRORMSG("AcceptBlock() : height in block claimed dismatched it's actual height"),
                REJECT_INVALID, "incorrect-height");

        int64_t tempTime = GetTimeMillis();

        // Check timestamp against prev
        if (block.GetBlockTime() <= pBlockIndexPrev->GetBlockTime() ||
            (block.GetBlockTime() - pBlockIndexPrev->GetBlockTime()) < SysCfg().GetBlockInterval())
            return state.Invalid(ERRORMSG("AcceptBlock() : block's timestamp is too early"),
                REJECT_INVALID, "time-too-early");

        // Check that the block chain matches the known block chain up to a checkpoint
        if (!Checkpoints::CheckBlock(nHeight, hash))
            return state.DoS(100, ERRORMSG("AcceptBlock() : rejected by checkpoint lock-in at %d", nHeight),
                REJECT_CHECKPOINT, "checkpoint mismatch");

        // Don't accept any forks from the main chain prior to last checkpoint
        CBlockIndex *pCheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
        if (pCheckpoint && (nHeight < pCheckpoint->nHeight))
            return state.DoS(100, ERRORMSG("AcceptBlock() : forked chain older than last checkpoint (height %d)", nHeight));

        //Check proof of pos tx
        if (!CheckBlockProofWorkWithCoinDay(block, pBlockIndexPrev, state)) {
            LogPrint("INFO", "CheckBlockProofWorkWithCoinDay() end: %lld ms\n", GetTimeMillis() - tempTime);
            return state.DoS(100, ERRORMSG("AcceptBlock() : check proof of pos tx"), REJECT_INVALID, "bad-pos-tx");
        }

        // Reject block.nVersion=1 blocks when 95% (75% on testnet) of the network has upgraded:
        if (block.GetVersion() < 2) {
            if ((!TestNet() && CBlockIndex::IsSuperMajority(2, pBlockIndexPrev, 950, 1000)) ||
                (TestNet() && CBlockIndex::IsSuperMajority(2, pBlockIndexPrev, 75, 100))) {
                return state.Invalid(ERRORMSG("AcceptBlock() : rejected nVersion=1 block"), REJECT_OBSOLETE, "bad-version");
            }
        }
    }

    // Write block to history file
    try {
        unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
        CDiskBlockPos blockPos;
        if (dbp != NULL)
            blockPos = *dbp;
        if (!FindBlockPos(state, blockPos, nBlockSize + 8, nHeight, block.GetTime(), dbp != NULL))
            return ERRORMSG("AcceptBlock() : FindBlockPos failed");
        if (dbp == NULL)
            if (!WriteBlockToDisk(block, blockPos))
                return state.Abort(_("Failed to write block"));
        if (!AddToBlockIndex(block, state, blockPos))
            return ERRORMSG("AcceptBlock() : AddToBlockIndex failed");
    } catch (std::runtime_error &e) {
        return state.Abort(_("System error: ") + e.what());
    }

    // Relay inventory, but don't relay old inventory during initial block download
    int nBlockEstimate = Checkpoints::GetTotalBlocksEstimate();
    if (chainActive.Tip()->GetBlockHash() == hash) {
        LOCK(cs_vNodes);
        for (auto pnode : vNodes)
            if (chainActive.Height() > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : nBlockEstimate))
                pnode->PushInventory(CInv(MSG_BLOCK, hash));
    }

    return true;
}

bool CBlockIndex::IsSuperMajority(int minVersion, const CBlockIndex *pstart, unsigned int nRequired, unsigned int nToCheck) {
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; i++) {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }
    return (nFound >= nRequired);
}

int64_t CBlockIndex::GetMedianTime() const {
    AssertLockHeld(cs_main);
    const CBlockIndex *pindex = this;
    for (int i = 0; i < nMedianTimeSpan / 2; i++) {
        if (!chainActive.Next(pindex))
            return GetBlockTime();
        pindex = chainActive.Next(pindex);
    }
    return pindex->GetMedianTimePast();
}

/** Turn the lowest '1' bit in the binary representation of a number into a '0'. */
int static inline InvertLowestOne(int n) { return n & (n - 1); }

/** Compute what height to jump back to with the CBlockIndex::pskip pointer. */
int static inline GetSkipHeight(int height) {
    if (height < 2)
        return 0;

    // Determine which height to jump back to. Any number strictly lower than height is acceptable,
    // but the following expression seems to perform well in simulations (max 110 steps to go back
    // up to 2**18 blocks).
    return (height & 1) ? InvertLowestOne(InvertLowestOne(height - 1)) + 1 : InvertLowestOne(height);
}

CBlockIndex *CBlockIndex::GetAncestor(int height) {
    if (height > nHeight || height < 0)
        return NULL;

    CBlockIndex *pindexWalk = this;
    int heightWalk          = nHeight;
    while (heightWalk > height) {
        int heightSkip     = GetSkipHeight(heightWalk);
        int heightSkipPrev = GetSkipHeight(heightWalk - 1);
        if (heightSkip == height ||
            (heightSkip > height && !(heightSkipPrev < heightSkip - 2 && heightSkipPrev >= height))) {
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

const CBlockIndex *CBlockIndex::GetAncestor(int height) const {
    return const_cast<CBlockIndex *>(this)->GetAncestor(height);
}

void CBlockIndex::BuildSkip() {
    if (pprev)
        pskip = pprev->GetAncestor(GetSkipHeight(nHeight));
}

void PushGetBlocks(CNode *pnode, CBlockIndex *pindexBegin, uint256 hashEnd) {
    // Ask this guy to fill in what we're missing
    AssertLockHeld(cs_main);
    // Filter out duplicate requests
    if (pindexBegin == pnode->pindexLastGetBlocksBegin && hashEnd == pnode->hashLastGetBlocksEnd) {
        LogPrint("net", "filter the same GetLocator\n");
        return;
    }
    pnode->pindexLastGetBlocksBegin = pindexBegin;
    pnode->hashLastGetBlocksEnd     = hashEnd;
    CBlockLocator blockLocator      = chainActive.GetLocator(pindexBegin);
    pnode->PushMessage("getblocks", blockLocator, hashEnd);
    LogPrint("net", "getblocks from peer %s, hashEnd:%s\n", pnode->addr.ToString(), hashEnd.GetHex());
}

void PushGetBlocksOnCondition(CNode *pnode, CBlockIndex *pindexBegin, uint256 hashEnd) {
    // Ask this guy to fill in what we're missing
    AssertLockHeld(cs_main);
    // Filter out duplicate requests
    if (pindexBegin == pnode->pindexLastGetBlocksBegin && hashEnd == pnode->hashLastGetBlocksEnd) {
        LogPrint("net", "filter the same GetLocator\n");
        static CBloomFilter filter(5000, 0.0001, 0, BLOOM_UPDATE_NONE);
        static unsigned int count = 0;
        string key                = to_string(pnode->id) + ":" + to_string((GetTime() / 2));
        if (!filter.contains(vector<unsigned char>(key.begin(), key.end()))) {
            filter.insert(vector<unsigned char>(key.begin(), key.end()));
            ++count;
            pnode->pindexLastGetBlocksBegin = pindexBegin;
            pnode->hashLastGetBlocksEnd     = hashEnd;
            CBlockLocator blockLocator      = chainActive.GetLocator(pindexBegin);
            pnode->PushMessage("getblocks", blockLocator, hashEnd);
            LogPrint("net", "getblocks from peer %s, hashEnd:%s\n", pnode->addr.ToString(), hashEnd.GetHex());
        } else {
            if (count >= 5000) {
                count = 0;
                filter.Clear();
            }
        }
    } else {
        pnode->pindexLastGetBlocksBegin = pindexBegin;
        pnode->hashLastGetBlocksEnd     = hashEnd;
        CBlockLocator blockLocator      = chainActive.GetLocator(pindexBegin);
        pnode->PushMessage("getblocks", blockLocator, hashEnd);
        LogPrint("net", "getblocks from peer %s, hashEnd:%s\n", pnode->addr.ToString(), hashEnd.GetHex());
    }
}

bool ProcessBlock(CValidationState &state, CNode *pfrom, CBlock *pblock, CDiskBlockPos *dbp) {
    int64_t llBeginTime = GetTimeMillis();
    //  LogPrint("INFO", "ProcessBlock() enter:%lld\n", llBeginTime);
    AssertLockHeld(cs_main);
    // Check for duplicate
    uint256 hash = pblock->GetHash();
    if (mapBlockIndex.count(hash))
        return state.Invalid(ERRORMSG("ProcessBlock() : already have block %d %s", mapBlockIndex[hash]->nHeight, hash.ToString()), 0, "duplicate");
    if (mapOrphanBlocks.count(hash))
        return state.Invalid(ERRORMSG("ProcessBlock() : already have block (orphan) %s", hash.ToString()), 0, "duplicate");

    int64_t llBeginCheckBlockTime = GetTimeMillis();
    CAccountViewCache view(*pAccountViewTip, true);
    CScriptDBViewCache scriptDBCache(*pScriptDBTip, true);
    // Preliminary checks
    if (!CheckBlock(*pblock, state, view, scriptDBCache, false)) {
        LogPrint("INFO", "CheckBlock() id: %d elapse time:%lld ms\n", chainActive.Height(), GetTimeMillis() - llBeginCheckBlockTime);
        return ERRORMSG("ProcessBlock() :block hash:%s CheckBlock FAILED", pblock->GetHash().GetHex());
    }

    // If we don't already have its previous block, shunt it off to holding area until we get it
    if (!pblock->GetHashPrevBlock().IsNull() && !mapBlockIndex.count(pblock->GetHashPrevBlock())) {
        if (pblock->GetHeight() > (unsigned int) nSyncTipHeight) {
            LogPrint("DEBUG", "blockHeight=%d syncTipHeight=%d\n", pblock->GetHeight(), nSyncTipHeight );
            nSyncTipHeight = pblock->GetHeight();
        }

        // Accept orphans as long as there is a node to request its parents from
        if (pfrom) {
            bool success = PruneOrphanBlocks(pblock->GetHeight());
            if (success) {
                COrphanBlock *pblock2 = new COrphanBlock();
                {
                    CDataStream ss(SER_DISK, CLIENT_VERSION);
                    ss << *pblock;
                    pblock2->vchBlock = vector<unsigned char>(ss.begin(), ss.end());
                }
                pblock2->hashBlock = hash;
                pblock2->hashPrev  = pblock->GetHashPrevBlock();
                pblock2->height    = pblock->GetHeight();
                mapOrphanBlocks.insert(make_pair(hash, pblock2));
                mapOrphanBlocksByPrev.insert(make_pair(pblock2->hashPrev, pblock2));
                setOrphanBlock.insert(pblock2);
            }

            // Ask this guy to fill in what we're missing
            LogPrint("net", "receive an orphan block height=%d hash=%s, %s it, and lead to getblocks, current height=%d, current orphan blocks=%d\n",
                     pblock->GetHeight(), pblock->GetHash().GetHex(), success ? "keep" : "abandon", chainActive.Tip()->nHeight, mapOrphanBlocksByPrev.size());
            PushGetBlocksOnCondition(pfrom, chainActive.Tip(), GetOrphanRoot(hash));
        }
        return true;
    }

    int64_t llAcceptBlockTime = GetTimeMillis();
    // Store to disk
    if (!AcceptBlock(*pblock, state, dbp)) {
        LogPrint("INFO", "AcceptBlock() elapse time:%lld ms\n", GetTimeMillis() - llAcceptBlockTime);
        return ERRORMSG("ProcessBlock() : AcceptBlock FAILED");
    }
    // LogPrint("INFO", "AcceptBlock() elapse time:%lld ms\n", GetTimeMillis() - llAcceptBlockTime);

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (unsigned int i = 0; i < vWorkQueue.size(); i++) {
        uint256 hashPrev = vWorkQueue[i];
        for (multimap<uint256, COrphanBlock *>::iterator mi = mapOrphanBlocksByPrev.lower_bound(hashPrev);
             mi != mapOrphanBlocksByPrev.upper_bound(hashPrev); ++mi) {
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
                vWorkQueue.push_back(mi->second->hashBlock);
            }
            setOrphanBlock.erase(mi->second);
            mapOrphanBlocks.erase(mi->second->hashBlock);
            delete mi->second;
        }
        mapOrphanBlocksByPrev.erase(hashPrev);
    }

    LogPrint("INFO", "ProcessBlock() elapse time:%lld ms\n", GetTimeMillis() - llBeginTime);
    return true;
}

bool CheckActiveChain(int nHeight, uint256 hash) {
    LogPrint("CHECKPOINT", "CheckActiveChain Enter====\n");
    LogPrint("CHECKPOINT", "check point hash:%s\n", hash.ToString());
    if (nHeight < 1) {
        return true;
    }
    LOCK(cs_main);
    CBlockIndex *pindexOldTip = chainActive.Tip();
    LogPrint("CHECKPOINT", "Current tip block:\n");
    LogPrint("CHECKPOINT", pindexOldTip->ToString().c_str());

    //Find the active chain dismatch checkpoint
    if (NULL == chainActive[nHeight] || hash != chainActive[nHeight]->GetBlockHash()) {
        CBlockIndex *pcheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
        LogPrint("CHECKPOINT", "Get Last check point:\n");
        if (pcheckpoint) {
            if (NULL == chainActive[nHeight] && chainActive.Contains(pcheckpoint)) {
                return true;
            }
            pcheckpoint->Print();
            chainMostWork.SetTip(pcheckpoint);
            bool bInvalidBlock                                                      = false;
            std::set<CBlockIndex *, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexValid.rbegin();
            for (; (it != setBlockIndexValid.rend()) && !chainMostWork.Contains(*it);) {
                bInvalidBlock           = false;
                CBlockIndex *pIndexTest = *it;
                LogPrint("CHECKPOINT", "iterator:height=%d, hash=%s\n", pIndexTest->nHeight, pIndexTest->GetBlockHash().GetHex());
                if (pcheckpoint->nHeight < nHeight) {
                    if (pIndexTest->nHeight >= nHeight) {
                        LogPrint("CHECKPOINT", "CheckActiveChain delete blockindex:%s\n", pIndexTest->GetBlockHash().GetHex());
                        setBlockIndexValid.erase(pIndexTest);
                        it            = setBlockIndexValid.rbegin();
                        bInvalidBlock = true;
                    }

                } else {
                    CBlockIndex *pIndexCheck = pIndexTest->pprev;
                    while (pIndexCheck && !chainMostWork.Contains(pIndexCheck)) {
                        pIndexCheck = pIndexCheck->pprev;
                    }
                    if (NULL == pIndexCheck || pIndexCheck->nHeight < pcheckpoint->nHeight) {
                        CBlockIndex *pIndexFailed = pIndexCheck;
                        while (pIndexTest != pIndexFailed) {
                            LogPrint("CHECKPOINT", "CheckActiveChain delete blockindex height=%d hash=%s\n", pIndexTest->nHeight, pIndexTest->GetBlockHash().GetHex());
                            setBlockIndexValid.erase(pIndexTest);
                            it            = setBlockIndexValid.rbegin();
                            bInvalidBlock = true;
                            pIndexTest    = pIndexTest->pprev;
                        }
                    }
                    if (chainMostWork.Contains(pIndexCheck) && chainMostWork.Height() == pIndexCheck->nHeight && pIndexTest->nChainWork > chainMostWork.Tip()->nChainWork) {
                        chainMostWork.SetTip(pIndexTest);
                        LogPrint("CHECKPOINT", "chainMostWork tip:height=%d, hash=%s\n", pIndexTest->nHeight,
                                 pIndexTest->GetBlockHash().GetHex());
                    }
                }
                if (!bInvalidBlock)
                    ++it;
            }

        } else {
            if (NULL == chainActive[nHeight])
                return true;
            bool bInvalidBlock                                                      = false;
            std::set<CBlockIndex *, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexValid.rbegin();
            for (; it != setBlockIndexValid.rend();) {
                bInvalidBlock           = false;
                CBlockIndex *pBlockTest = *it;
                while (pBlockTest->nHeight > nHeight) {
                    pBlockTest = pBlockTest->pprev;
                }
                if (pBlockTest->GetBlockHash() != hash) {
                    CBlockIndex *pBlockIndexFailed = *it;
                    while (pBlockIndexFailed != pBlockTest) {
                        LogPrint("CHECKPOINT", "CheckActiveChain delete blockindex height=%d hash=%s\n", pBlockIndexFailed->nHeight, pBlockIndexFailed->GetBlockHash().GetHex());
                        setBlockIndexValid.erase(pBlockIndexFailed);
                        pBlockIndexFailed = pBlockIndexFailed->pprev;
                        it                = setBlockIndexValid.rbegin();
                        bInvalidBlock     = true;
                        LogPrint("CHECKPOINT", "setBlockIndexValid size:%d\n", setBlockIndexValid.size());
                    }
                }
                if (!bInvalidBlock)
                    ++it;
            }
            assert(chainActive[nHeight - 1]);
            chainMostWork.SetTip(chainActive[nHeight - 1]);
        }

        // Check whether we have something to do. sync chainMostWork to chainActive;disconnect block or connect block;
        if (chainMostWork.Tip() == NULL)
            return false;
        CValidationState state;
        while (chainActive.Tip() && !chainMostWork.Contains(chainActive.Tip())) {
            if (!DisconnectTip(state))
                return false;
        }
        while (NULL != chainMostWork[chainActive.Height() + 1]) {
            CBlockIndex *pindexConnect = chainMostWork[chainActive.Height() + 1];
            if (!ConnectTip(state, pindexConnect)) {
                if (state.IsInvalid()) {
                    // The block violates a consensus rule.
                    if (!state.CorruptionPossible())
                        InvalidChainFound(chainMostWork.Tip());
                    state = CValidationState();
                    break;
                } else {
                    // A system error occurred (disk space, database error, ...).
                    return false;
                }
            }
            setBlockIndexValid.insert(pindexConnect);
        }
    }

    if (chainActive.Tip() != pindexOldTip) {
        std::string strCmd = SysCfg().GetArg("-blocknotify", "");
        if (!IsInitialBlockDownload() && !strCmd.empty()) {
            boost::replace_all(strCmd, "%s", chainActive.Tip()->GetBlockHash().GetHex());
            boost::thread t(runCommand, strCmd);  // thread runs free
        }
    }
    LogPrint("CHECKPOINT", "CheckActiveChain End====\n");
    return true;
}

CMerkleBlock::CMerkleBlock(const CBlock &block, CBloomFilter &filter) {
    header = block.GetBlockHeader();

    vector<bool> vMatch;
    vector<uint256> vHashes;

    vMatch.reserve(block.vptx.size());
    vHashes.reserve(block.vptx.size());

    for (unsigned int i = 0; i < block.vptx.size(); i++) {
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

uint256 CPartialMerkleTree::CalcHash(int height, unsigned int pos, const vector<uint256> &vTxid) {
    if (height == 0) {
        // hash at height 0 is the txids themself
        return vTxid[pos];
    } else {
        // calculate left hash
        uint256 left = CalcHash(height - 1, pos * 2, vTxid), right;
        // calculate right hash if not beyong the end of the array - copy left hash otherwise1
        if (pos * 2 + 1 < CalcTreeWidth(height - 1))
            right = CalcHash(height - 1, pos * 2 + 1, vTxid);
        else
            right = left;
        // combine subhashes
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    }
}

void CPartialMerkleTree::TraverseAndBuild(int height, unsigned int pos, const vector<uint256> &vTxid, const vector<bool> &vMatch) {
    // determine whether this node is the parent of at least one matched txid
    bool fParentOfMatch = false;
    for (unsigned int p = pos << height; p < (pos + 1) << height && p < nTransactions; p++)
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

uint256 CPartialMerkleTree::TraverseAndExtract(int height, unsigned int pos,
                                               unsigned int &nBitsUsed, unsigned int &nHashUsed, vector<uint256> &vMatch) {
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

CPartialMerkleTree::CPartialMerkleTree(const vector<uint256> &vTxid, const vector<bool> &vMatch) : nTransactions(vTxid.size()), fBad(false) {
    // reset state
    vBits.clear();
    vHash.clear();

    // calculate height of tree
    int nHeight = 0;
    while (CalcTreeWidth(nHeight) > 1)
        nHeight++;

    // traverse the partial tree
    TraverseAndBuild(nHeight, 0, vTxid, vMatch);
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
    int nHeight = 0;
    while (CalcTreeWidth(nHeight) > 1)
        nHeight++;
    // traverse the partial tree
    unsigned int nBitsUsed = 0, nHashUsed = 0;
    uint256 hashMerkleRoot = TraverseAndExtract(nHeight, 0, nBitsUsed, nHashUsed, vMatch);
    // verify that no problems occured during the tree traversal
    if (fBad)
        return uint256();
    // verify that all bits were consumed (except for the padding caused by serializing it as a byte sequence)
    if ((nBitsUsed + 7) / 8 != (vBits.size() + 7) / 8)
        return uint256();
    // verify that all hashes were consumed
    if (nHashUsed != vHash.size())
        return uint256();
    return hashMerkleRoot;
}

bool AbortNode(const string &strMessage) {
    strMiscWarning = strMessage;
    LogPrint("INFO", "*** %s\n", strMessage);
    uiInterface.ThreadSafeMessageBox(strMessage, "", CClientUIInterface::MSG_ERROR);
    StartShutdown();
    return false;
}

bool CheckDiskSpace(uint64_t nAdditionalBytes) {
    uint64_t nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

    // Check for kMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < kMinDiskSpace + nAdditionalBytes)
        return AbortNode(_("Error: Disk space is low!"));

    return true;
}

FILE *OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly) {
    if (pos.IsNull())
        return NULL;
    boost::filesystem::path path = GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
    boost::filesystem::create_directories(path.parent_path());
    FILE *file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file) {
        LogPrint("INFO", "Unable to open file %s\n", path.string());
        return NULL;
    }
    if (pos.nPos) {
        if (fseek(file, pos.nPos, SEEK_SET)) {
            LogPrint("INFO", "Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return NULL;
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

CBlockIndex *InsertBlockIndex(uint256 hash) {
    if (hash.IsNull())
        return NULL;

    // Return existing
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    // Create new
    CBlockIndex *pindexNew = new CBlockIndex();
    if (!pindexNew)
        throw runtime_error("InsertBlockIndex() : new CBlockIndex failed");
    mi                    = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}

bool static LoadBlockIndexDB() {
    if (!pblocktree->LoadBlockIndexGuts())
        return false;

    boost::this_thread::interruption_point();

    // Calculate nChainWork
    vector<pair<int, CBlockIndex *> > vSortedByHeight;
    vSortedByHeight.reserve(mapBlockIndex.size());
    for (const auto &item : mapBlockIndex) {
        CBlockIndex *pindex = item.second;
        vSortedByHeight.push_back(make_pair(pindex->nHeight, pindex));
    }
    sort(vSortedByHeight.begin(), vSortedByHeight.end());
    for (const auto &item : vSortedByHeight) {
        CBlockIndex *pindex = item.second;
        pindex->nChainWork  = pindex->nHeight;
        pindex->nChainTx    = (pindex->pprev ? pindex->pprev->nChainTx : 0) + pindex->nTx;
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS && !(pindex->nStatus & BLOCK_FAILED_MASK))
            setBlockIndexValid.insert(pindex);
        if (pindex->nStatus & BLOCK_FAILED_MASK && (!pindexBestInvalid || pindex->nChainWork > pindexBestInvalid->nChainWork))
            pindexBestInvalid = pindex;
        if (pindex->pprev)
            pindex->BuildSkip();
    }

    // Load block file info
    pblocktree->ReadLastBlockFile(nLastBlockFile);
    LogPrint("INFO", "LoadBlockIndexDB(): last block file = %i\n", nLastBlockFile);
    if (pblocktree->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        LogPrint("INFO", "LoadBlockIndexDB(): last block file info: %s\n", infoLastBlockFile.ToString());

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    pblocktree->ReadReindexing(fReindexing);

    bool fcurReinx = SysCfg().IsReindex();
    SysCfg().SetReIndex(fcurReinx |= fReindexing);

    // Check whether we have a transaction index
    bool bTxIndex = SysCfg().IsTxIndex();
    pblocktree->ReadFlag("txindex", bTxIndex);
    SysCfg().SetTxIndex(bTxIndex);
    LogPrint("INFO", "LoadBlockIndexDB(): transaction index %s\n", bTxIndex ? "enabled" : "disabled");

    // Load pointer to end of best chain
    map<uint256, CBlockIndex *>::iterator it = mapBlockIndex.find(pAccountViewTip->GetBestBlock());

    //    for(auto &item : mapBlockIndex)
    //      LogPrint("INFO", "block hash:%s\n", item.first.GetHex());
    LogPrint("INFO", "best block hash:%s\n", pAccountViewTip->GetBestBlock().GetHex());
    if (it == mapBlockIndex.end())
        return true;
    chainActive.SetTip(it->second);
    LogPrint("INFO", "LoadBlockIndexDB(): hashBestChain=%s height=%d date=%s progress=%f\n",
             chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
             DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
             Checkpoints::GuessVerificationProgress(chainActive.Tip()));

    return true;
}

bool VerifyDB(int nCheckLevel, int nCheckDepth) {
    LOCK(cs_main);
    if (chainActive.Tip() == NULL || chainActive.Tip()->pprev == NULL)
        return true;

    // Verify blocks in the best chain
    if (nCheckDepth <= 0)
        nCheckDepth = 1000000000;  // suffices until the year 19000
    if (nCheckDepth > chainActive.Height())
        nCheckDepth = chainActive.Height();
    nCheckLevel = max(0, min(4, nCheckLevel));
    LogPrint("INFO", "Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    CAccountViewCache view(*pAccountViewTip, true);
    CTransactionDBCache txCacheTemp(*pTxCacheTip, true);
    CScriptDBViewCache scriptDBCache(*pScriptDBTip, true);
    CBlockIndex *pindexState   = chainActive.Tip();
    CBlockIndex *pindexFailure = NULL;
    int nGoodTransactions      = 0;
    CValidationState state;
    //   int64_t llTime = 0;
    for (CBlockIndex *pindex = chainActive.Tip(); pindex && pindex->pprev; pindex = pindex->pprev) {
        //      llTime = GetTimeMillis();
        boost::this_thread::interruption_point();
        if (pindex->nHeight < chainActive.Height() - nCheckDepth)
            break;
        CBlock block;
        //       LogPrint("INFO", "block hash:%s", pindex->GetBlockHash().ToString());
        // check level 0: read from disk
        if (!ReadBlockFromDisk(block, pindex))
            return ERRORMSG("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !CheckBlock(block, state, view, scriptDBCache))
            return ERRORMSG("VerifyDB() : *** found bad block at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pindex) {
            CBlockUndo undo;
            CDiskBlockPos pos = pindex->GetUndoPos();
            if (!pos.IsNull()) {
                if (!undo.ReadFromDisk(pos, pindex->pprev->GetBlockHash()))
                    return ERRORMSG("VerifyDB() : *** found bad undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        if (nCheckLevel >= 3 && pindex == pindexState /*&& (coins.GetCacheSize() + pcoinsTip->GetCacheSize()) <= 2*nCoinCacheSize + 32000*/) {
            bool fClean = true;

            if (!DisconnectBlock(block, state, view, pindex, txCacheTemp, scriptDBCache, &fClean))
                return ERRORMSG("VerifyDB() : *** irrecoverable inconsistency in block data at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            pindexState = pindex->pprev;
            if (!fClean) {
                nGoodTransactions = 0;
                pindexFailure     = pindex;
            } else
                nGoodTransactions += block.vptx.size();
        }
        //        LogPrint("INFO", "VerifyDB block height:%d, hash:%s ,elapse time:%lld ms\n", pindex->nHeight, pindex->GetBlockHash().GetHex(), GetTimeMillis() - llTime);
    }
    if (pindexFailure)
        return ERRORMSG("VerifyDB() : *** coin database inconsistencies found (last %i blocks, %i good transactions before that)\n", chainActive.Height() - pindexFailure->nHeight + 1, nGoodTransactions);

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4) {
        CBlockIndex *pindex = pindexState;
        while (pindex != chainActive.Tip()) {
            boost::this_thread::interruption_point();
            pindex = chainActive.Next(pindex);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex))
                return ERRORMSG("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            if (!ConnectBlock(block, state, view, pindex, txCacheTemp, scriptDBCache, false))
                return ERRORMSG("VerifyDB() : *** found unconnectable block at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        }
    }

    LogPrint("INFO", "No coin database inconsistencies in last %i blocks (%i transactions)\n", chainActive.Height() - pindexState->nHeight, nGoodTransactions);

    return true;
}

void UnloadBlockIndex() {
    mapBlockIndex.clear();
    setBlockIndexValid.clear();
    chainActive.SetTip(NULL);
    pindexBestInvalid = NULL;
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
    if (chainActive.Genesis() != NULL)
        return true;

    // Use the provided setting for -txindex in the new database
    SysCfg().SetTxIndex(SysCfg().GetBoolArg("-txindex", true));
    pblocktree->WriteFlag("txindex", SysCfg().IsTxIndex());
    LogPrint("INFO", "Initializing databases...\n");

    // Only add the genesis block if not reindexing (in which case we reuse the one already on disk)
    if (!SysCfg().IsReindex()) {
        try {
            CBlock &block = const_cast<CBlock &>(SysCfg().GenesisBlock());
            // Start new block file
            unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
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
        CBlockIndex *pindex = (*mi).second;
        mapNext[pindex->pprev].push_back(pindex);
        // test
        //while (rand() % 3 == 0)
        //    mapNext[pindex->pprev].push_back(pindex);
    }

    vector<pair<int, CBlockIndex *> > vStack;
    vStack.push_back(make_pair(0, chainActive.Genesis()));

    int nPrevCol = 0;
    while (!vStack.empty()) {
        int nCol            = vStack.back().first;
        CBlockIndex *pindex = vStack.back().second;
        vStack.pop_back();

        // print split or gap
        if (nCol > nPrevCol) {
            for (int i = 0; i < nCol - 1; i++)
                LogPrint("INFO", "| ");
            LogPrint("INFO", "|\\\n");
        } else if (nCol < nPrevCol) {
            for (int i = 0; i < nCol; i++)
                LogPrint("INFO", "| ");
            LogPrint("INFO", "|\n");
        }
        nPrevCol = nCol;

        // print columns
        for (int i = 0; i < nCol; i++)
            LogPrint("INFO", "| ");

        // print item
        CBlock block;
        ReadBlockFromDisk(block, pindex);
        LogPrint("INFO", "%d (blk%05u.dat:0x%x)  %s  tx %u\n",
                 pindex->nHeight,
                 pindex->GetBlockPos().nFile, pindex->GetBlockPos().nPos,
                 DateTimeStrFormat("%Y-%m-%d %H:%M:%S", block.GetBlockTime()),
                 block.vptx.size());

        // put the main time-chain first
        vector<CBlockIndex *> &vNext = mapNext[pindex];
        for (unsigned int i = 0; i < vNext.size(); i++) {
            if (chainActive.Next(vNext[i])) {
                swap(vNext[0], vNext[i]);
                break;
            }
        }

        // iterate children
        for (unsigned int i = 0; i < vNext.size(); i++)
            vStack.push_back(make_pair(nCol + i, vNext[i]));
    }
}

bool LoadExternalBlockFile(FILE *fileIn, CDiskBlockPos *dbp) {
    int64_t nStart = GetTimeMillis();
    int nLoaded    = 0;
    try {
        CBufferedFile blkdat(fileIn, 2 * MAX_BLOCK_SIZE, MAX_BLOCK_SIZE + 8, SER_DISK, CLIENT_VERSION);
        uint64_t nStartByte = 0;
        if (dbp) {
            // (try to) skip already indexed part
            CBlockFileInfo info;
            if (pblocktree->ReadBlockFileInfo(dbp->nFile, info)) {
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
            unsigned int nSize = 0;
            try {
                // locate a header
                unsigned char buf[MESSAGE_START_SIZE];
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
                    if (ProcessBlock(state, NULL, &block, dbp))
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
    int nPriority = 0;
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

//////////////////////////////////////////////////////////////////////////////
//
// Messages
//

bool static AlreadyHave(const CInv &inv) {
    switch (inv.type) {
        case MSG_TX: {
            bool txInMap = false;
            txInMap      = mempool.Exists(inv.hash);
            return txInMap || mapOrphanTransactions.count(inv.hash);
        }
        case MSG_BLOCK:
            return mapBlockIndex.count(inv.hash) ||
                   mapOrphanBlocks.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}

void static ProcessGetData(CNode *pfrom) {
    deque<CInv>::iterator it = pfrom->vRecvGetData.begin();

    vector<CInv> vNotFound;

    LOCK(cs_main);

    while (it != pfrom->vRecvGetData.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;

        const CInv &inv = *it;
        {
            boost::this_thread::interruption_point();
            it++;

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK) {
                bool send                                = false;
                map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(inv.hash);
                if (mi != mapBlockIndex.end()) {
                    // If the requested block is at a height below our last
                    // checkpoint, only serve it if it's in the checkpointed chain
                    int nHeight              = mi->second->nHeight;
                    CBlockIndex *pcheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
                    if (pcheckpoint && nHeight < pcheckpoint->nHeight) {
                        if (!chainActive.Contains(mi->second)) {
                            LogPrint("INFO", "ProcessGetData(): ignoring request for old block that isn't in the main chain\n");
                        } else {
                            send = true;
                        }
                    } else {
                        send = true;
                    }
                }
                if (send) {
                    // Send block from disk
                    CBlock block;
                    ReadBlockFromDisk(block, (*mi).second);
                    if (inv.type == MSG_BLOCK)
                        pfrom->PushMessage("block", block);
                    else  // MSG_FILTERED_BLOCK)
                    {
                        LOCK(pfrom->cs_filter);
                        if (pfrom->pfilter) {
                            CMerkleBlock merkleBlock(block, *pfrom->pfilter);
                            pfrom->PushMessage("merkleblock", merkleBlock);
                            // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                            // This avoids hurting performance by pointlessly requiring a round-trip
                            // Note that there is currently no way for a node to request any single transactions we didnt send here -
                            // they must either disconnect and retry or request the full block.
                            // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                            // however we MUST always provide at least what the remote peer needs
                            for (auto &pair : merkleBlock.vMatchedTxn)
                                if (!pfrom->setInventoryKnown.count(CInv(MSG_TX, pair.second)))
                                    pfrom->PushMessage("tx", block.vptx[pair.first]);
                        }
                        // else
                        // no response
                    }

                    // Trigger them to send a getblocks request for the next batch of inventory
                    if (inv.hash == pfrom->hashContinue) {
                        // Bypass PushInventory, this must send even if redundant,
                        // and we want it right after the last block so they don't
                        // wait for other stuff first.
                        vector<CInv> vInv;
                        vInv.push_back(CInv(MSG_BLOCK, chainActive.Tip()->GetBlockHash()));
                        pfrom->PushMessage("inv", vInv);
                        pfrom->hashContinue.SetNull();
                        LogPrint("net", "reset node hashcontinue\n");
                    }
                }
            } else if (inv.IsKnownType()) {
                // Send stream from relay memory
                bool pushed = false;
                {
                    LOCK(cs_mapRelay);
                    map<CInv, CDataStream>::iterator mi = mapRelay.find(inv);
                    if (mi != mapRelay.end()) {
                        pfrom->PushMessage(inv.GetCommand(), (*mi).second);
                        pushed = true;
                    }
                }
                if (!pushed && inv.type == MSG_TX) {
                    std::shared_ptr<CBaseTx> pBaseTx = mempool.Lookup(inv.hash);
                    if (pBaseTx.get()) {
                        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                        ss.reserve(1000);
                        if (COMMON_TX == pBaseTx->nTxType) {
                            ss << *((CCommonTx *)(pBaseTx.get()));
                        } else if (CONTRACT_TX == pBaseTx->nTxType) {
                            ss << *((CContractTx *)(pBaseTx.get()));
                        } else if (REG_ACCT_TX == pBaseTx->nTxType) {
                            ss << *((CRegisterAccountTx *)pBaseTx.get());
                        } else if (REG_CONT_TX == pBaseTx->nTxType) {
                            ss << *((CRegisterContractTx *)pBaseTx.get());
                        } else if (DELEGATE_TX == pBaseTx->nTxType) {
                            ss << *((CDelegateTx *)pBaseTx.get());
                        }
                        pfrom->PushMessage("tx", ss);
                        pushed = true;
                    }
                }
                if (!pushed) {
                    vNotFound.push_back(inv);
                }
            }

            // Track requests for our stuff.
            // g_signals.Inventory(inv.hash);

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
                break;
        }
    }

    pfrom->vRecvGetData.erase(pfrom->vRecvGetData.begin(), it);

    if (!vNotFound.empty()) {
        // Let the peer know that we didn't find what it asked for, so it doesn't
        // have to wait around forever. Currently only SPV clients actually care
        // about this message: it's needed when they are recursively walking the
        // dependencies of relevant unconfirmed transactions. SPV clients want to
        // do that because they want to know about (and store and rebroadcast and
        // risk analyze) the dependencies of transactions relevant to them, without
        // having to download the entire memory pool.
        pfrom->PushMessage("notfound", vNotFound);
    }
}

bool static ProcessMessage(CNode *pfrom, string strCommand, CDataStream &vRecv)
{
    RandAddSeedPerfmon();
    LogPrint("net", "received: %s (%u bytes)\n", strCommand, vRecv.size());
    //  if (GetRand(atoi(SysCfg().GetArg("-dropmessagestest", "0"))) == 0)
    //  {
    //      LogPrint("INFO","dropmessagestest DROPPING RECV MESSAGE\n");
    //      return true;
    //  }

    {
        LOCK(cs_main);
        State(pfrom->GetId())->nLastBlockProcess = GetTimeMicros();
    }

    if (strCommand == "version") {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0) {
            pfrom->PushMessage("reject", strCommand, REJECT_DUPLICATE, string("Duplicate version message"));
            LogPrint("INFO", "Misbehaving: Duplicated version message, nMisbehavior add 1\n");
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        vRecv >> pfrom->nVersion >> pfrom->nServices >> nTime >> addrMe;
        if (pfrom->nVersion < MIN_PEER_PROTO_VERSION) {
            // disconnect from peers older than this proto version
            LogPrint("INFO", "partner %s using obsolete version %i; disconnecting\n", pfrom->addr.ToString(), pfrom->nVersion);
            pfrom->PushMessage("reject", strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", MIN_PEER_PROTO_VERSION));
            pfrom->fDisconnect = true;
            return false;
        }

        if (pfrom->nVersion == 10300)
            pfrom->nVersion = 300;

        if (!vRecv.empty())
            vRecv >> addrFrom >> nNonce;

        if (!vRecv.empty()) {
            vRecv >> pfrom->strSubVer;
            pfrom->cleanSubVer = SanitizeString(pfrom->strSubVer);
        }

        if (!vRecv.empty())
            vRecv >> pfrom->nStartingHeight;

        if (!vRecv.empty())
            vRecv >> pfrom->fRelayTxes;  // set to true after we get the first filter* message
        else
            pfrom->fRelayTxes = true;

        if (pfrom->fInbound && addrMe.IsRoutable()) {
            pfrom->addrLocal = addrMe;
            SeenLocal(addrMe);
        }

        // Disconnect if we connected to ourself
        if (nNonce == nLocalHostNonce && nNonce > 1) {
            LogPrint("INFO", "connected to self at %s, disconnecting\n", pfrom->addr.ToString());
            pfrom->fDisconnect = true;
            return true;
        }

        // Be shy and don't send version until we hear
        if (pfrom->fInbound)
            pfrom->PushVersion();

        pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);

        // Change version
        pfrom->PushMessage("verack");
        pfrom->ssSend.SetVersion(min(pfrom->nVersion, PROTOCOL_VERSION));

        if (!pfrom->fInbound) {
            // Advertise our address
            if (!fNoListen && !IsInitialBlockDownload()) {
                CAddress addr = GetLocalAddress(&pfrom->addr);
                if (addr.IsRoutable())
                    pfrom->PushAddress(addr);
            }

            // Get recent addresses
            if (pfrom->fOneShot || /*pfrom->nVersion >= CADDR_TIME_VERSION || */ addrman.size() < 1000) {
                pfrom->PushMessage("getaddr");
                pfrom->fGetAddr = true;
            }
            addrman.Good(pfrom->addr);
        } else {
            if (((CNetAddr)pfrom->addr) == (CNetAddr)addrFrom) {
                addrman.Add(addrFrom, addrFrom);
                addrman.Good(addrFrom);
            }
        }

        // Relay alerts
        {
            LOCK(cs_mapAlerts);
            for (const auto &item : mapAlerts)
                item.second.RelayTo(pfrom);
        }

        pfrom->fSuccessfullyConnected = true;

        LogPrint("INFO", "receive version message: %s: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", pfrom->cleanSubVer, pfrom->nVersion, pfrom->nStartingHeight, addrMe.ToString(), addrFrom.ToString(), pfrom->addr.ToString());

        AddTimeData(pfrom->addr, nTime);

        LOCK(cs_main);
        cPeerBlockCounts.input(pfrom->nStartingHeight);

        if (pfrom->nStartingHeight > Checkpoints::GetTotalBlocksEstimate()) {
            pfrom->PushMessage("getcheck", Checkpoints::GetTotalBlocksEstimate());
        }
    }

    else if (pfrom->nVersion == 0) {
        // Must have a version message before anything else
        Misbehaving(pfrom->GetId(), 1);
        return false;
    }

    else if (strCommand == "verack") {
        pfrom->SetRecvVersion(min(pfrom->nVersion, PROTOCOL_VERSION));
    }

    else if (strCommand == "addr") {
        vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Don't want addr from older versions unless seeding
        //        if (pfrom->nVersion < CADDR_TIME_VERSION && addrman.size() > 1000)
        //            return true;
        if (vAddr.size() > 1000) {
            Misbehaving(pfrom->GetId(), 20);
            return ERRORMSG("message addr size() = %u", vAddr.size());
        }

        // Store the new addresses
        vector<CAddress> vAddrOk;
        int64_t nNow   = GetAdjustedTime();
        int64_t nSince = nNow - 10 * 60;
        for (auto &addr : vAddr) {
            boost::this_thread::interruption_point();

            if (addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
                addr.nTime = nNow - 5 * 24 * 60 * 60;

            pfrom->AddAddressKnown(addr);
            bool fReachable = IsReachable(addr);
            if (addr.nTime > nSince && !pfrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable()) {
                // Relay to a limited number of other nodes
                {
                    LOCK(cs_vNodes);
                    // Use deterministic randomness to send to the same nodes for 24 hours
                    // at a time so the setAddrKnowns of the chosen nodes prevent repeats
                    static uint256 hashSalt;
                    if (hashSalt.IsNull())
                        hashSalt = GetRandHash();
                    uint64_t hashAddr = addr.GetHash();
                    uint256 hashRand  = ArithToUint256(UintToArith256(hashSalt) ^ (hashAddr << 32) ^ ((GetTime() + hashAddr) / (24 * 60 * 60)));
                    hashRand          = Hash(BEGIN(hashRand), END(hashRand));
                    multimap<uint256, CNode *> mapMix;
                    for (auto pnode : vNodes) {
                        //                        if (pnode->nVersion < CADDR_TIME_VERSION)
                        //                            continue;
                        unsigned int nPointer;
                        memcpy(&nPointer, &pnode, sizeof(nPointer));
                        uint256 hashKey = ArithToUint256(UintToArith256(hashRand) ^ nPointer);
                        hashKey         = Hash(BEGIN(hashKey), END(hashKey));
                        mapMix.insert(make_pair(hashKey, pnode));
                    }
                    int nRelayNodes = fReachable ? 2 : 1;  // limited relaying of addresses outside our network(s)
                    for (multimap<uint256, CNode *>::iterator mi = mapMix.begin(); mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
                        ((*mi).second)->PushAddress(addr);
                }
            }
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        addrman.Add(vAddrOk, pfrom->addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom->fGetAddr = false;
        if (pfrom->fOneShot)
            pfrom->fDisconnect = true;
    }

    else if (strCommand == "inv") {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ) {
            Misbehaving(pfrom->GetId(), 20);
            return ERRORMSG("message inv size() = %u", vInv.size());
        }

        LOCK(cs_main);

        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++) {
            const CInv &inv = vInv[nInv];

            boost::this_thread::interruption_point();
            pfrom->AddInventoryKnown(inv);
            bool fAlreadyHave = AlreadyHave(inv);

            int nBlockHeight = 0;
            if (inv.type == MSG_BLOCK && mapBlockIndex.count(inv.hash))
                nBlockHeight = mapBlockIndex[inv.hash]->nHeight;

            LogPrint("net", "got inventory[%d]: %s %s %d from peer %s\n", nInv, inv.ToString(),
                fAlreadyHave ? "have" : "new", nBlockHeight, pfrom->addr.ToString());

            if (!fAlreadyHave) {
                if (!SysCfg().IsImporting() && !SysCfg().IsReindex()) {
                    if (inv.type == MSG_BLOCK)
                        AddBlockToQueue(pfrom->GetId(), inv.hash);
                    else
                        pfrom->AskFor(inv);  // MSG_TX
                }
            } else if (inv.type == MSG_BLOCK && mapOrphanBlocks.count(inv.hash)) {
                COrphanBlock *pOrphanBlock = mapOrphanBlocks[inv.hash];
                LogPrint("net", "receive orphan block inv height=%d hash=%s lead to getblocks, current height=%d\n",
                    pOrphanBlock->height, inv.hash.GetHex(), chainActive.Tip()->nHeight);
                PushGetBlocksOnCondition(pfrom, chainActive.Tip(), GetOrphanRoot(inv.hash));
            }

            // Track requests for our stuff
            // g_signals.Inventory(inv.hash);

            if (pfrom->nSendSize > (SendBufferSize() * 2)) {
                Misbehaving(pfrom->GetId(), 50);
                return ERRORMSG("send buffer size() = %u", pfrom->nSendSize);
            }
        }
    }

    else if (strCommand == "getdata") {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ) {
            Misbehaving(pfrom->GetId(), 20);
            return ERRORMSG("message getdata size() = %u", vInv.size());
        }

        if ((vInv.size() != 1))
            LogPrint("net", "received getdata (%u invsz)\n", vInv.size());

        if ((vInv.size() > 0) || (vInv.size() == 1))
            LogPrint("net", "received getdata for: %s\n", vInv[0].ToString());

        pfrom->vRecvGetData.insert(pfrom->vRecvGetData.end(), vInv.begin(), vInv.end());
        ProcessGetData(pfrom);
    }

    else if (strCommand == "getblocks") {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        LOCK(cs_main);

        // Find the last block the caller has in the main chain
        CBlockIndex *pindex = chainActive.FindFork(locator);

        // Send the rest of the chain
        if (pindex)
            pindex = chainActive.Next(pindex);
        int nLimit = 500;
        LogPrint("net", "getblocks %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString(), nLimit);
        for (; pindex; pindex = chainActive.Next(pindex)) {
            if (pindex->GetBlockHash() == hashStop) {
                LogPrint("net", "getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0) {
                // When this block is requested, we'll send an inv that'll make them
                // getblocks the next batch of inventory.
                LogPrint("net", "getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                pfrom->hashContinue = pindex->GetBlockHash();
                break;
            }
        }
    }

    else if (strCommand == "getheaders") {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        LOCK(cs_main);

        CBlockIndex *pindex = NULL;
        if (locator.IsNull()) {
            // If locator is null, return the hashStop block
            map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hashStop);
            if (mi == mapBlockIndex.end())
                return true;
            pindex = (*mi).second;
        } else {
            // Find the last block the caller has in the main chain
            pindex = chainActive.FindFork(locator);
            if (pindex)
                pindex = chainActive.Next(pindex);
        }

        // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
        vector<CBlock> vHeaders;
        int nLimit = 2000;
        LogPrint("NET", "getheaders %d to %s\n", (pindex ? pindex->nHeight : -1), hashStop.ToString());
        for (; pindex; pindex = chainActive.Next(pindex)) {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        }
        pfrom->PushMessage("headers", vHeaders);
    }

    else if (strCommand == "tx") {
        std::shared_ptr<CBaseTx> pBaseTx = CreateNewEmptyTransaction(vRecv[0]);

        if (REWARD_TX == pBaseTx->nTxType)
            return ERRORMSG("Reward tx from network NOT accepted. Hex:%s", HexStr(vRecv.begin(), vRecv.end()));

        vRecv >> pBaseTx;

        CInv inv(MSG_TX, pBaseTx->GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK(cs_main);
        CValidationState state;
        if (AcceptToMemoryPool(mempool, state, pBaseTx.get(), true)) {
            RelayTransaction(pBaseTx.get(), inv.hash);
            mapAlreadyAskedFor.erase(inv);

            LogPrint("INFO", "AcceptToMemoryPool: %s %s : accepted %s (poolsz %u)\n",
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                pBaseTx->GetHash().ToString(),
                mempool.mapTx.size());
        }

        int nDoS = 0;
        if (state.IsInvalid(nDoS)) {
            LogPrint("INFO", "%s from %s %s was not accepted into the memory pool: %s\n",
                pBaseTx->GetHash().ToString(),
                pfrom->addr.ToString(),
                pfrom->cleanSubVer,
                state.GetRejectReason());

            pfrom->PushMessage("reject", strCommand, state.GetRejectCode(), state.GetRejectReason(), inv.hash);
            // if (nDoS > 0) {
            //     LogPrint("INFO", "Misebehaving, add to tx hash %s mempool error, Misbehavior add %d", pBaseTx->GetHash().GetHex(), nDoS);
            //     Misbehaving(pfrom->GetId(), nDoS);
            // }
        }
    }

    else if (strCommand == "block" && !SysCfg().IsImporting() && !SysCfg().IsReindex())  // Ignore blocks received while importing
    {
        CBlock block;
        vRecv >> block;

        LogPrint("net", "received block %s from %s\n", block.GetHash().ToString(), pfrom->addr.ToString());
        // block.Print();

        CInv inv(MSG_BLOCK, block.GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK(cs_main);
        // Remember who we got this block from.
        mapBlockSource[inv.hash] = pfrom->GetId();
        MarkBlockAsReceived(inv.hash, pfrom->GetId());

        CValidationState state;
        ProcessBlock(state, pfrom, &block);
    }

    else if (strCommand == "getaddr") {
        pfrom->vAddrToSend.clear();
        vector<CAddress> vAddr = addrman.GetAddr();
        for (const auto &addr : vAddr)
            pfrom->PushAddress(addr);
    }

    else if (strCommand == "mempool") {
        LOCK2(cs_main, pfrom->cs_filter);

        vector<uint256> vtxid;
        mempool.QueryHash(vtxid);
        vector<CInv> vInv;
        for (auto &hash : vtxid) {
            CInv inv(MSG_TX, hash);
            std::shared_ptr<CBaseTx> pBaseTx = mempool.Lookup(hash);
            if (pBaseTx.get())
                continue;  // another thread removed since queryHashes, maybe...

            if ((pfrom->pfilter && pfrom->pfilter->contains(hash)) ||  //other type transaction
                (!pfrom->pfilter))
                vInv.push_back(inv);

            if (vInv.size() == MAX_INV_SZ) {
                pfrom->PushMessage("inv", vInv);
                vInv.clear();
            }
        }
        if (vInv.size() > 0)
            pfrom->PushMessage("inv", vInv);
    }

    else if (strCommand == "ping") {
        //        if (pfrom->nVersion > BIP0031_VERSION)
        //        {
        uint64_t nonce = 0;
        vRecv >> nonce;
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
        pfrom->PushMessage("pong", nonce);
        //        }
    }

    else if (strCommand == "pong") {
        int64_t pingUsecEnd = GetTimeMicros();
        uint64_t nonce      = 0;
        size_t nAvail       = vRecv.in_avail();
        bool bPingFinished  = false;
        string sProblem;

        if (nAvail >= sizeof(nonce)) {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (pfrom->nPingNonceSent != 0) {
                if (nonce == pfrom->nPingNonceSent) {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished        = true;
                    int64_t pingUsecTime = pingUsecEnd - pfrom->nPingUsecStart;
                    if (pingUsecTime > 0) {
                        // Successful ping time measurement, replace previous
                        pfrom->nPingUsecTime = pingUsecTime;
                    } else {
                        // This should never happen
                        sProblem = "Timing mishap";
                    }
                } else {
                    // Nonce mismatches are normal when pings are overlapping
                    sProblem = "Nonce mismatch";
                    if (nonce == 0) {
                        // This is most likely a bug in another implementation somewhere, cancel this ping
                        bPingFinished = true;
                        sProblem      = "Nonce zero";
                    }
                }
            } else {
                sProblem = "Unsolicited pong without ping";
            }
        } else {
            // This is most likely a bug in another implementation somewhere, cancel this ping
            bPingFinished = true;
            sProblem      = "Short payload";
        }

        if (!(sProblem.empty())) {
            LogPrint("net", "pong %s %s: %s, %x expected, %x received, %u bytes\n",
                     pfrom->addr.ToString(),
                     pfrom->cleanSubVer,
                     sProblem,
                     pfrom->nPingNonceSent,
                     nonce,
                     nAvail);
        }
        if (bPingFinished) {
            pfrom->nPingNonceSent = 0;
        }
    }

    else if (strCommand == "alert") {
        CAlert alert;
        vRecv >> alert;

        uint256 alertHash = alert.GetHash();
        if (pfrom->setKnown.count(alertHash) == 0) {
            if (alert.ProcessAlert()) {
                // Relay
                pfrom->setKnown.insert(alertHash);
                {
                    LOCK(cs_vNodes);
                    for (auto pnode : vNodes)
                        alert.RelayTo(pnode);
                }
            } else {
                // Small DoS penalty so peers that send us lots of
                // duplicate/expired/invalid-signature/whatever alerts
                // eventually get banned.
                // This isn't a Misbehaving(100) (immediate ban) because the
                // peer might be an older or different implementation with
                // a different signature key, etc.
                Misbehaving(pfrom->GetId(), 10);
            }
        }
    }

    else if (strCommand == "filterload") {
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints()) {
            LogPrint("INFO", "Misebehaving: filter is not within size constraints, Misbehavior add 100");
            // There is no excuse for sending a too-large filter
            Misbehaving(pfrom->GetId(), 100);
        } else {
            LOCK(pfrom->cs_filter);
            delete pfrom->pfilter;
            pfrom->pfilter = new CBloomFilter(filter);
            pfrom->pfilter->UpdateEmptyFull();
        }
        pfrom->fRelayTxes = true;
    }

    else if (strCommand == "filteradd") {
        vector<unsigned char> vData;
        vRecv >> vData;

        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        if (vData.size() > 520)  //MAX_SCRIPT_ELEMENT_SIZE)
        {
            LogPrint("INFO", "Misbehaving: send a data item > 520 bytes, Misbehavior add 100");
            Misbehaving(pfrom->GetId(), 100);
        } else {
            LOCK(pfrom->cs_filter);
            if (pfrom->pfilter)
                pfrom->pfilter->insert(vData);
            else {
                LogPrint("INFO", "Misbehaving: filter error, Misbehavior add 100");
                Misbehaving(pfrom->GetId(), 100);
            }
        }
    }

    else if (strCommand == "filterclear") {
        LOCK(pfrom->cs_filter);
        delete pfrom->pfilter;
        pfrom->pfilter    = new CBloomFilter();
        pfrom->fRelayTxes = true;
    }

    else if (strCommand == "reject") {
        if (SysCfg().IsDebug()) {
            string strMsg;
            unsigned char ccode;
            string strReason;
            vRecv >> strMsg >> ccode >> strReason;

            ostringstream ss;
            ss << strMsg << " code " << itostr(ccode) << ": " << strReason;

            if (strMsg == "block" || strMsg == "tx") {
                uint256 hash;
                vRecv >> hash;
                ss << ": hash " << hash.ToString();
            }
            // Truncate to reasonable length and sanitize before printing:
            string s = ss.str();
            if (s.size() > 111)
                s.erase(111, string::npos);

            LogPrint("net", "Reject %s\n", SanitizeString(s));
        }

    } else if (strCommand == "checkpoint") {
        LogPrint("CHECKPOINT", "enter checkpoint\n");
        std::vector<int> vIndex;
        std::vector<SyncData::CSyncData> vdata;
        vRecv >> vdata;
        BOOST_FOREACH (SyncData::CSyncData &data, vdata) {
            if (data.CheckSignature(SysCfg().GetCheckPointPKey())) {
                SyncData::CSyncCheckPoint point;
                point.SetData(data);
                SyncData::CSyncDataDb db;
                if (!db.ExistCheckpoint(point.m_height)) {
                    db.WriteCheckpoint(point.m_height, data);
                    Checkpoints::AddCheckpoint(point.m_height, point.m_hashCheckpoint);
                    CheckActiveChain(point.m_height, point.m_hashCheckpoint);
                    pfrom->setcheckPointKnown.insert(point.m_height);
                    vIndex.push_back(point.m_height);
                }
            }
        }
        if (vIndex.size() == 1 && vIndex.size() == vdata.size()) {
            LOCK(cs_vNodes);
            BOOST_FOREACH (CNode *pnode, vNodes) {
                if (pnode->setcheckPointKnown.count(vIndex[0]) == 0) {
                    pnode->setcheckPointKnown.insert(vIndex[0]);
                    pnode->PushMessage("checkpoint", vdata);
                }
            }
        }
    } else if (strCommand == "getcheck") {
        int height = 0;
        vRecv >> height;
        SyncData::CSyncDataDb db;
        std::vector<SyncData::CSyncData> vdata;
        std::vector<int> vheight;
        Checkpoints::GetCheckpointByHeight(height, vheight);
        for (std::size_t i = 0; i < vheight.size(); ++i) {
            SyncData::CSyncData data;
            if (pfrom->setcheckPointKnown.count(vheight[i]) == 0 && db.ReadCheckpoint(vheight[i], data)) {
                pfrom->setcheckPointKnown.insert(vheight[i]);
                vdata.push_back(data);
            }
        }
        if (!vdata.empty()) {
            pfrom->PushMessage("checkpoint", vdata);
        }
    } else {
        // Ignore unknown commands for extensibility
    }

    // Update the last seen time for this node's address
    if (pfrom->fNetworkNode)
        if (strCommand == "version" || strCommand == "addr" || strCommand == "inv" || strCommand == "getdata" || strCommand == "ping")
            AddressCurrentlyConnected(pfrom->addr);

    return true;
}

// requires LOCK(cs_vRecvMsg)
bool ProcessMessages(CNode *pfrom) {
    //if (fDebug)
    //    LogPrint("INFO","ProcessMessages(%u messages)\n", pfrom->vRecvMsg.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fOk = true;

    if (!pfrom->vRecvGetData.empty())
        ProcessGetData(pfrom);

    // this maintains the order of responses
    if (!pfrom->vRecvGetData.empty()) return fOk;

    deque<CNetMessage>::iterator it = pfrom->vRecvMsg.begin();
    while (!pfrom->fDisconnect && it != pfrom->vRecvMsg.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
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
        unsigned int nMessageSize = hdr.nMessageSize;

        // Checksum
        CDataStream &vRecv     = msg.vRecv;
        uint256 hash           = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        unsigned int nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));
        if (nChecksum != hdr.nChecksum) {
            LogPrint("INFO", "ProcessMessages(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
                     strCommand, nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Process message
        bool fRet = false;
        try {
            fRet = ProcessMessage(pfrom, strCommand, vRecv);
            boost::this_thread::interruption_point();
        } catch (std::ios_base::failure &e) {
            pfrom->PushMessage("reject", strCommand, REJECT_MALFORMED, string("error parsing message"));
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
            PrintExceptionContinue(NULL, "ProcessMessages()");
        }

        if (!fRet)
            LogPrint("INFO", "ProcessMessage(%s, %u bytes) FAILED\n", strCommand, nMessageSize);

        break;
    }

    // In case the connection got shut down, its receive buffer was wiped
    if (!pfrom->fDisconnect)
        pfrom->vRecvMsg.erase(pfrom->vRecvMsg.begin(), it);

    return fOk;
}

bool SendMessages(CNode *pto, bool fSendTrickle) {
    {
        // Don't send anything until we get their version message
        if (pto->nVersion == 0)
            return true;

        //
        // Message: ping
        //
        bool pingSend = false;
        if (pto->fPingQueued) {
            // RPC ping request by user
            pingSend = true;
        }
        if (pto->nLastSend && GetTime() - pto->nLastSend > 30 * 60 && pto->vSendMsg.empty()) {
            // Ping automatically sent as a keepalive
            pingSend = true;
        }
        if (pingSend) {
            uint64_t nonce = 0;
            while (nonce == 0) {
                RAND_bytes((unsigned char *)&nonce, sizeof(nonce));
            }
            pto->nPingNonceSent = nonce;
            pto->fPingQueued    = false;
            //            if (pto->nVersion > BIP0031_VERSION) {
            // Take timestamp as close as possible before transmitting ping
            pto->nPingUsecStart = GetTimeMicros();
            pto->PushMessage("ping", nonce);
            //            } else {
            //                // Peer is too old to support ping command with nonce, pong will never arrive, disable timing
            //                pto->nPingUsecStart = 0;
            //                pto->PushMessage("ping");
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
                for (auto pnode : vNodes) {
                    // Periodically clear setAddrKnown to allow refresh broadcasts
                    if (nLastRebroadcast)
                        pnode->setAddrKnown.clear();

                    // Rebroadcast our address
                    if (!fNoListen) {
                        CAddress addr = GetLocalAddress(&pnode->addr);
                        if (addr.IsRoutable())
                            pnode->PushAddress(addr);
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
            vAddr.reserve(pto->vAddrToSend.size());
            for (const auto &addr : pto->vAddrToSend) {
                // returns true if wasn't already contained in the set
                if (pto->setAddrKnown.insert(addr).second) {
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than 1000
                    if (vAddr.size() >= 1000) {
                        pto->PushMessage("addr", vAddr);
                        vAddr.clear();
                    }
                }
            }
            pto->vAddrToSend.clear();
            if (!vAddr.empty())
                pto->PushMessage("addr", vAddr);
        }

        CNodeState &state = *State(pto->GetId());
        if (state.fShouldBan) {
            if (pto->addr.IsLocal())
                LogPrint("INFO", "Warning: not banning local node %s!\n", pto->addr.ToString());
            else {
                LogPrint("INFO", "Warning: banned a remote node %s!\n", pto->addr.ToString());
                pto->fDisconnect = true;
                CNode::Ban(pto->addr);
            }
            state.fShouldBan = false;
        }

        for (const auto &reject : state.rejects)
            pto->PushMessage("reject", (string) "block", reject.chRejectCode, reject.strRejectReason, reject.hashBlock);
        state.rejects.clear();

        // Start block sync
        if (pto->fStartSync && !SysCfg().IsImporting() && !SysCfg().IsReindex()) {
            pto->fStartSync = false;
            nSyncTipHeight  = pto->nStartingHeight;
            uiInterface.NotifyBlocksChanged(0, chainActive.Tip()->nHeight, chainActive.Tip()->GetBlockHash());
            LogPrint("net", "start block sync lead to getblocks\n");
            PushGetBlocks(pto, chainActive.Tip(), uint256());
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
            LOCK(pto->cs_inventory);
            vInv.reserve(pto->vInventoryToSend.size());
            vInvWait.reserve(pto->vInventoryToSend.size());
            for (const auto &inv : pto->vInventoryToSend) {
                if (pto->setInventoryKnown.count(inv))
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
                if (pto->setInventoryKnown.insert(inv).second) {
                    vInv.push_back(inv);
                    if (vInv.size() >= 1000) {
                        pto->PushMessage("inv", vInv);
                        vInv.clear();
                    }
                }
            }
            pto->vInventoryToSend = vInvWait;
        }
        if (!vInv.empty())
            pto->PushMessage("inv", vInv);

        // Detect stalled peers. Require that blocks are in flight, we haven't
        // received a (requested) block in one minute, and that all blocks are
        // in flight for over two minutes, since we first had a chance to
        // process an incoming block.
        int64_t nNow = GetTimeMicros();
        if (!pto->fDisconnect && state.nBlocksInFlight &&
            state.nLastBlockReceive < state.nLastBlockProcess - BLOCK_DOWNLOAD_TIMEOUT * 1000000 &&
            state.vBlocksInFlight.front().nTime < state.nLastBlockProcess - 2 * BLOCK_DOWNLOAD_TIMEOUT * 1000000) {
            LogPrint("INFO", "Peer %s is stalling block download, disconnecting\n", state.name.c_str());
            pto->fDisconnect = true;
        }

        //
        // Message: getdata (blocks)
        //
        vector<CInv> vGetData;
        int nIndex(0);
        while (!pto->fDisconnect && state.nBlocksToDownload && state.nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            uint256 hash = state.vBlocksToDownload.front();
            vGetData.push_back(CInv(MSG_BLOCK, hash));
            MarkBlockAsInFlight(pto->GetId(), hash);
            LogPrint("net", "Requesting block [%d] %s from %s, nBlocksInFlight=%d\n",
                     ++nIndex, hash.ToString().c_str(), state.name.c_str(), state.nBlocksInFlight);
            if (vGetData.size() >= 1000) {
                pto->PushMessage("getdata", vGetData);
                vGetData.clear();
                nIndex = 0;
            }
        }

        //
        // Message: getdata (non-blocks)
        //
        while (!pto->fDisconnect && !pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow) {
            const CInv &inv = (*pto->mapAskFor.begin()).second;
            if (!AlreadyHave(inv)) {
                LogPrint("net", "sending getdata: %s\n", inv.ToString());
                vGetData.push_back(inv);
                if (vGetData.size() >= 1000) {
                    pto->PushMessage("getdata", vGetData);
                    vGetData.clear();
                }
            }
            pto->mapAskFor.erase(pto->mapAskFor.begin());
        }
        if (!vGetData.empty())
            pto->PushMessage("getdata", vGetData);
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

std::shared_ptr<CBaseTx> CreateNewEmptyTransaction(unsigned char uType) {
    switch (uType) {
        case COMMON_TX:
            return std::make_shared<CCommonTx>();
        case CONTRACT_TX:
            return std::make_shared<CContractTx>();
        case REG_ACCT_TX:
            return std::make_shared<CRegisterAccountTx>();
        case REWARD_TX:
            return std::make_shared<CRewardTx>();
        case REG_CONT_TX:
            return std::make_shared<CRegisterContractTx>();
        case DELEGATE_TX:
            return std::make_shared<CDelegateTx>();
        default:
            ERRORMSG("CreateNewEmptyTransaction type error");
            break;
    }
    return NULL;
}

string CBlockUndo::ToString() const {
    vector<CTxUndo>::const_iterator iterUndo = vtxundo.begin();
    string str("");
    LogPrint("DEBUG", "list txundo:\n");
    for (; iterUndo != vtxundo.end(); ++iterUndo) {
        str += iterUndo->ToString();
    }
    return str;
}

bool DisconnectBlockFromTip(CValidationState &state) {
    return DisconnectTip(state);
}

bool GetTxOperLog(const uint256 &txHash, vector<CAccountLog> &vAccountLog) {
    if (SysCfg().IsTxIndex()) {
        CDiskTxPos postx;
        if (pScriptDBTip->ReadTxIndex(txHash, postx)) {
            CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
            CBlockHeader header;
            try {
                file >> header;
            } catch (std::exception &e) {
                return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
            }
            uint256 blockHash = header.GetHash();
            if (mapBlockIndex.count(blockHash) > 0) {
                CBlockIndex *pIndex = mapBlockIndex[blockHash];
                CBlockUndo blockUndo;
                CDiskBlockPos pos = pIndex->GetUndoPos();
                if (pos.IsNull())
                    return ERRORMSG("DisconnectBlock() : no undo data available");
                if (!blockUndo.ReadFromDisk(pos, pIndex->pprev->GetBlockHash()))
                    return ERRORMSG("DisconnectBlock() : failure reading undo data");

                for (auto &txUndo : blockUndo.vtxundo) {
                    if (txUndo.txHash == txHash) {
                        vAccountLog = txUndo.vAccountLog;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

Value ListSetBlockIndexValid() {
    Object result;
    std::set<CBlockIndex *, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexValid.rbegin();
    for (; it != setBlockIndexValid.rend(); ++it) {
        CBlockIndex *pIndex = *it;
        result.push_back(Pair(tfm::format("height=%d status=%b", pIndex->nHeight, pIndex->nStatus).c_str(), pIndex->GetBlockHash().GetHex()));
    }
    uint256 hash            = uint256S("0x6dccf719d146184b9a26e37d62be193fd51d0d49b2f8aa15f84656d790e1d46c");
    CBlockIndex *blockIndex = mapBlockIndex[hash];
    for (; blockIndex != NULL && blockIndex->nHeight > 157332; blockIndex = blockIndex->pprev) {
        result.push_back(Pair(tfm::format("height=%d status=%b", blockIndex->nHeight, blockIndex->nStatus).c_str(), blockIndex->GetBlockHash().GetHex()));
    }
    return result;
}

bool EraseBlockIndexFromSet(CBlockIndex *pIndex) {
    AssertLockHeld(cs_main);
    return setBlockIndexValid.erase(pIndex) > 0;
}

uint64_t GetBlockSubsidy(int nHeight) {
    return IniCfg().GetBlockSubsidyCfg(nHeight);
}
