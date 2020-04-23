// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MAIN_H
#define COIN_MAIN_H

#if defined(HAVE_CONFIG_H)
#include "config/coin-config.h"
#endif

#include <stdint.h>
#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "commons/arith_uint256.h"
#include "commons/types.h"
#include "commons/uint256.h"
#include "config/chainparams.h"
#include "config/const.h"
#include "config/errorcode.h"
#include "chain/chain.h"
#include "chain/merkletree.h"
#include "net.h"
#include "p2p/node.h"
#include "persistence/cachewrapper.h"
#include "sigcache.h"
#include "tx/tx.h"
#include "tx/txmempool.h"
//#include "tx/txserializer.h"

class CBloomFilter;
class CChain;
class CInv;

extern CCriticalSection cs_main;
/** The currently-connected chain of blocks. */
extern CChain chainActive;
extern CSignatureCache signatureCache;

extern CTxMemPool mempool;
extern map<uint256, CBlockIndex *> mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
extern const string strMessageMagic;
extern bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block);

extern bool mining;     // could be changed due to vote change
extern CKeyID minerKeyId;  // miner accout keyId
extern CKeyID nodeKeyId;   // first keyId of the node

class CValidationState;
class CWalletInterface;

struct CNodeStateStats;

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

/** Register a wallet to receive updates from core */
void RegisterWallet(CWalletInterface *pWalletIn);
/** Unregister a wallet from core */
void UnregisterWallet(CWalletInterface *pWalletIn);
/** Unregister all wallets from core */
void UnregisterAllWallets();
/** Push an updated transaction to all registered wallets */
void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pBlock = nullptr);
/** Erase Tx from wallets **/
void EraseTransactionFromWallet(const uint256 &hash);
/** Register with a network node to receive its signals */
void RegisterNodeSignals(CNodeSignals &nodeSignals);
/** Unregister a network node */
void UnregisterNodeSignals(CNodeSignals &nodeSignals);
/** Check whether enough disk space is available for an incoming block */
bool CheckDiskSpace(uint64_t nAdditionalBytes = 0);

/** Verify consistency of the block and coin databases */
bool VerifyDB(int32_t nCheckLevel, int32_t nCheckDepth);

/** Run an instance of the script checking thread */
void ThreadScriptCheck();

/** Format a string that describes several potential problems detected by the core */
string GetWarnings(string strFor);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CBlockDBCache &blockCache, bool bSearchMempool = true);
/** Retrieve a transaction height comfirmed in block*/
int32_t GetTxConfirmHeight(const uint256 &hash, CBlockDBCache &blockCache);

/** Abort with a message */
bool AbortNode(const string &msg);
/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int32_t howmuch);

bool VerifySignature(const uint256 &sigHash, const std::vector<uint8_t> &signature, const CPubKey &pubKey);

/** (try to) add transaction to memory pool **/
bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx,
                        bool fLimitFree, bool fRejectInsaneFee = false);

struct CNodeStateStats {
    int32_t nMisbehavior;
};

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(CBaseTx *pBaseTx, string &reason);

bool IsInitialBlockDownload();

/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,    // everything ok
        MODE_INVALID,  // network rule violation (DoS value may be set)
        MODE_ERROR,    // run-time error
    } mode;
    int32_t nDoS;
    string rejectReason;
    uint8_t rejectCode;
    bool corruptionPossible;

    string ret;

public:
    CValidationState() : mode(MODE_VALID), nDoS(0), corruptionPossible(false) {}
    bool DoS(int32_t level, bool ret = false, uint8_t rejectCodeIn = 0, string rejectReasonIn = "",
             bool corruptionIn = false) {
        rejectCode         = rejectCodeIn;
        rejectReason       = rejectReasonIn;
        corruptionPossible = corruptionIn;
        if (mode == MODE_ERROR)
            return ret;
        nDoS += level;
        mode = MODE_INVALID;
        return ret;
    }
    bool Invalid(bool ret = false, uint8_t rejectCodeIn = 0, string rejectReasonIn = "") {
        return DoS(0, ret, rejectCodeIn, rejectReasonIn);
    }
    bool Error(string rejectReasonIn = "") {
        if (mode == MODE_VALID)
            rejectReason = rejectReasonIn;
        mode = MODE_ERROR;
        return false;
    }
    bool Abort(const string &msg) {
        AbortNode(msg);
        return Error(msg);
    }
    bool IsValid() const { return mode == MODE_VALID; }
    bool IsInvalid() const { return mode == MODE_INVALID; }
    bool IsError() const { return mode == MODE_ERROR; }
    bool IsInvalid(int32_t &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const { return corruptionPossible; }
    uint8_t GetRejectCode() const { return rejectCode; }
    string GetRejectReason() const { return rejectReason; }

    void SetReturn(std::string r) {ret = r;}
    std::string GetReturn() {return ret;}
};

/** The currently best known chain of headers (some of which may be invalid). */
extern CChain chainMostWork;
extern CCacheDBManager *pCdMan;
extern int32_t nSyncTipHeight;
extern std::tuple<bool, boost::thread *> RunCoin(int32_t argc, char *argv[]);
extern string publicIp;

bool EraseBlockIndexFromSet(CBlockIndex *pIndex);

class CWalletInterface {
protected:
    virtual void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pBlock) = 0;
    virtual void EraseTransaction(const uint256 &hash)                                        = 0;
    virtual void SetBestChain(const CBlockLocator &locator)                                   = 0;
    virtual void ResendWalletTransactions()                                                   = 0;
    friend void ::RegisterWallet(CWalletInterface *);
    friend void ::UnregisterWallet(CWalletInterface *);
    friend void ::UnregisterAllWallets();
};

/** Functions for validating blocks and updating the block tree */

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  In case pfClean is provided, operation will try to be tolerant about errors, and *pfClean
 *  will be true if no problems were found. Otherwise, the return value will be false in case
 *  of problems. Note that in any case, coins may be modified. */
bool DisconnectBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool *pfClean = nullptr);
// Apply the effects of this block (with given index) on the UTXO set represented by coins
bool ConnectBlock   (CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool fJustCheck = false);

// Add this block to the block index, and if necessary, switch the active block chain to this
bool AddToBlockIndex(CBlock &block, CValidationState &state, const CDiskBlockPos &pos);

// Context-independent validity checks
bool CheckBlock(const CBlock &block, CValidationState &state, CCacheWrapper &cw,
                bool fCheckTx = true, bool fCheckMerkleRoot = true);

bool ProcessForkedChain(const CBlock &block, CValidationState &state);

// Store block on disk
// if dbp is provided, the file is known to already reside on disk
bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp = nullptr, bool mining = false);

//disconnect block for test
bool DisconnectTip(CValidationState &state);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState &state, CBlockIndex *pIndex);

/** Import blocks from an external file */
bool LoadExternalBlockFile(FILE *fileIn, CDiskBlockPos *dbp = nullptr);
/** Initialize a new block tree database + block data on disk */
bool InitBlockIndex();
/** Load the block tree and coins database from disk */
bool LoadBlockIndex();
/** Unload database information */
void UnloadBlockIndex();
/** Push getblocks request */
void PushGetBlocks(CNode *pNode, CBlockIndex *pindexBegin, uint256 hashEnd);
/** Push getblocks request with different filtering strategies */
void PushGetBlocksOnCondition(CNode *pNode, CBlockIndex *pindexBegin, uint256 hashEnd);
/** Process an incoming block */
bool ProcessBlock(CValidationState &state, CNode *pFrom, CBlock *pBlock, CDiskBlockPos *dbp = nullptr);
/** Print the loaded block tree */
void PrintBlockTree();

void UpdateTime(CBlockHeader &block, const CBlockIndex *pIndexPrev);

/** Find the best known block, and make it the tip of the block chain */
bool ActivateBestChain(CValidationState &state, CBlockIndex* pNewIndex = nullptr);

/** Remove invalidity status from a block and its descendants. */
bool ReconsiderBlock(CValidationState &state, CBlockIndex *pIndex);

#endif
