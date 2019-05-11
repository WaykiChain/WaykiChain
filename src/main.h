// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MAIN_H
#define COIN_MAIN_H

#if defined(HAVE_CONFIG_H)
#include "coin-config.h"
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
#include "commons/uint256.h"
#include "chainparams.h"
#include "core.h"
#include "net.h"
#include "persistence/blockdb.h"
#include "persistence/accountview.h"
#include "persistence/accountdb.h"
#include "persistence/txdb.h"
#include "persistence/contractdb.h"
#include "sigcache.h"
#include "sync.h"
#include "tx/txmempool.h"
#include "tx/accountreg.h"
#include "tx/bcoin.h"
#include "tx/contract.h"
#include "tx/delegate.h"
#include "tx/tx.h"
#include "tx/bcoin.h"
#include "tx/delegate.h"

class CBlockIndex;
class CBloomFilter;
class CInv;
class CContractScript;

/** the total blocks of burn fee need */
static const unsigned int DEFAULT_BURN_BLOCK_SIZE = 50;
/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 4000000;
/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
static const unsigned int DEFAULT_BLOCK_MAX_SIZE = 3750000;
static const unsigned int DEFAULT_BLOCK_MIN_SIZE = 1024 * 10;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_SIZE = 100000;
/** The maximum number of orphan blocks kept in memory */
static const unsigned int MAX_ORPHAN_BLOCKS = 750;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000;  // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000;  // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000;  // 1 MiB
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;
/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000;  // Tue Nov  5 00:53:20 1985 UTC
/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 128;
/** Timeout in seconds before considering a block download peer unresponsive. */
static const unsigned int BLOCK_DOWNLOAD_TIMEOUT = 60;
static const unsigned long MAX_BLOCK_RUN_STEP    = 12000000;
static const int64_t POS_REWARD                  = 10 * COIN;
static const int64_t INIT_FUEL_RATES             = 100;  //100 unit / 100 step
static const int64_t MIN_FUEL_RATES              = 1;    //1 unit / 100 step

/** max size of signature of tx or block */
static const int MAX_BLOCK_SIGNATURE_SIZE = 100;

#ifdef USE_UPNP
static const int fHaveUPnP = true;
#else
static const int fHaveUPnP = false;
#endif

/** "reject" message codes **/
static const unsigned char REJECT_MALFORMED       = 0x01;
static const unsigned char REJECT_INVALID         = 0x10;
static const unsigned char REJECT_OBSOLETE        = 0x11;
static const unsigned char REJECT_DUPLICATE       = 0x12;
static const unsigned char REJECT_NONSTANDARD     = 0x40;
static const unsigned char REJECT_DUST            = 0x41;
static const unsigned char REJECT_INSUFFICIENTFEE = 0x42;
static const unsigned char REJECT_CHECKPOINT      = 0x43;

static const unsigned char READ_ACCOUNT_FAIL   = 0X51;
static const unsigned char WRITE_ACCOUNT_FAIL  = 0X52;
static const unsigned char UPDATE_ACCOUNT_FAIL = 0X53;

static const unsigned char PRICE_FEED_FAIL     = 0X54;

static const unsigned char READ_SCRIPT_FAIL  = 0X61;
static const unsigned char WRITE_SCRIPT_FAIL = 0X62;

static const uint64_t kMinDiskSpace              = 52428800;  // Minimum disk space required
static const int kContractScriptMaxSize          = 65536;     // 64 KB max for contract script size
static const int kContractArgumentMaxSize        = 4096;      // 4 KB max for contract argument size
static const int kCommonTxMemoMaxSize            = 100;       // 100 bytes max for memo size
static const int kContractMemoMaxSize            = 100;       // 100 bytes max for memo size
static const int kMostRecentBlockNumberThreshold = 1000;      // most recent block number threshold
static const int kRegIdMaturePeriodByBlock       = 100;       // RegId's mature period measured by blocks
static const int kMultisigNumberThreshold        = 15;        // m-n multisig, refer to n
static const int KMultisigScriptMaxSize          = 1000;      // multisig script max size
static const string kContractScriptPathPrefix    = "/tmp/lua/";

extern CCriticalSection cs_main;
extern CTxMemPool mempool;
extern map<uint256, CBlockIndex *> mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
extern const string strMessageMagic;

class CBlockTreeDB;
struct CDiskBlockPos;
class CTxUndo;
class CValidationState;
class CWalletInterface;
struct CNodeStateStats;
class CAccountViewDB;
class CTransactionDB;
class CScriptDB;

struct CBlockTemplate;

/** Register a wallet to receive updates from core */
void RegisterWallet(CWalletInterface *pwalletIn);
/** Unregister a wallet from core */
void UnregisterWallet(CWalletInterface *pwalletIn);
/** Unregister all wallets from core */
void UnregisterAllWallets();
/** Push an updated transaction to all registered wallets */
void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pblock = NULL);
/** Erase Tx from wallets **/
void EraseTransaction(const uint256 &hash);
/** Register with a network node to receive its signals */
void RegisterNodeSignals(CNodeSignals &nodeSignals);
/** Unregister a network node */
void UnregisterNodeSignals(CNodeSignals &nodeSignals);
/** Push getblocks request */
void PushGetBlocks(CNode *pnode, CBlockIndex *pindexBegin, uint256 hashEnd);
/** Push getblocks request with different filtering strategies */
void PushGetBlocksOnCondition(CNode *pnode, CBlockIndex *pindexBegin, uint256 hashEnd);
/** Process an incoming block */
bool ProcessBlock(CValidationState &state, CNode *pfrom, CBlock *pblock, CDiskBlockPos *dbp = NULL);
/** Check whether enough disk space is available for an incoming block */
bool CheckDiskSpace(uint64_t nAdditionalBytes = 0);
/** Open a block file (blk?????.dat) */
FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Import blocks from an external file */
bool LoadExternalBlockFile(FILE *fileIn, CDiskBlockPos *dbp = NULL);
/** Initialize a new block tree database + block data on disk */
bool InitBlockIndex();
/** Load the block tree and coins database from disk */
bool LoadBlockIndex();
/** Unload database information */
void UnloadBlockIndex();
/** Verify consistency of the block and coin databases */
bool VerifyDB(int nCheckLevel, int nCheckDepth);
/** Print the loaded block tree */
void PrintBlockTree();
/** Process protocol messages received from a given node */
bool ProcessMessages(CNode *pfrom);
/** Send queued protocol messages to be sent to a give node */
bool SendMessages(CNode *pto, bool fSendTrickle);
/** Run an instance of the script checking thread */
void ThreadScriptCheck();
/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);
/** Calculate the minimum amount of work a received block needs, without knowing its direct parent */
unsigned int ComputeMinWork(unsigned int nBase, int64_t nTime);
/** Check whether we are doing an initial block download (synchronizing from disk or network) */
bool IsInitialBlockDownload();
/** Format a string that describes several potential problems detected by the core */
string GetWarnings(string strFor);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CScriptDBViewCache &scriptDBCache, bool bSearchMempool = true);
/** Retrieve a transaction height comfirmed in block*/
int GetTxConfirmHeight(const uint256 &hash, CScriptDBViewCache &scriptDBCache);

/** Find the best known block, and make it the tip of the block chain */
bool ActivateBestChain(CValidationState &state);
int64_t GetBlockValue(int nHeight, int64_t nFees);

/*calutate difficulty */
double CaculateDifficulty(unsigned int nBits);

/** receive checkpoint check make active chain accord to the checkpoint **/
bool CheckActiveChain(int nHeight, uint256 hash);

void UpdateTime(CBlockHeader &block, const CBlockIndex *pindexPrev);

/** Create a new block index entry for a given block hash */
CBlockIndex *InsertBlockIndex(uint256 hash);

/** Abort with a message */
bool AbortNode(const string &msg);
/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int howmuch);

bool CheckSignScript(const uint256 &sigHash, const std::vector<unsigned char> &signature, const CPubKey &pubKey);

/** (try to) add transaction to memory pool **/
bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx,
                        bool fLimitFree, bool fRejectInsaneFee = false);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState &state, CBlockIndex *pIndex);

/** Remove invalidity status from a block and its descendants. */
bool ReconsiderBlock(CValidationState &state, CBlockIndex *pIndex);

std::shared_ptr<CBaseTx> CreateNewEmptyTransaction(unsigned char uType);

struct CNodeStateStats {
    int nMisbehavior;
};

int64_t GetMinRelayFee(const CBaseTx *pBaseTx, unsigned int nBytes, bool fAllowFree);

inline bool AllowFree(double dPriority) {
    // Large (in bytes) low-priority (new, small-coin) transactions
    // need a fee.
    // return dPriority > COIN * 144 / 250;
    return true;
}

// Context-independent validity checks
bool CheckTx(CBaseTx *pBaseTx, CValidationState &state, CAccountViewCache &view);

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(CBaseTx *pBaseTx, string &reason);

bool IsFinalTx(CBaseTx *pBaseTx, int nBlockHeight = 0, int64_t nBlockTime = 0);

/** Functions for disk access for blocks */
bool WriteBlockToDisk(CBlock &block, CDiskBlockPos &pos);
bool ReadBlockFromDisk(const CDiskBlockPos &pos, CBlock &block);
bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block);

/** Functions for validating blocks and updating the block tree */

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  In case pfClean is provided, operation will try to be tolerant about errors, and *pfClean
 *  will be true if no problems were found. Otherwise, the return value will be false in case
 *  of problems. Note that in any case, coins may be modified. */
bool DisconnectBlock(CBlock &block, CValidationState &state, CAccountViewCache &view, CBlockIndex *pIndex,
                    CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache, bool *pfClean = NULL);

// Apply the effects of this block (with given index) on the UTXO set represented by coins
bool ConnectBlock(CBlock &block, CValidationState &state, CAccountViewCache &view, CBlockIndex *pIndex, CTransactionDBCache &txCache,
                CScriptDBViewCache &scriptCache, bool fJustCheck = false);

// Add this block to the block index, and if necessary, switch the active block chain to this
bool AddToBlockIndex(CBlock &block, CValidationState &state, const CDiskBlockPos &pos);

// Context-independent validity checks
bool CheckBlock(const CBlock &block, CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDBCache,
                bool fCheckTx = true, bool fCheckMerkleRoot = true);

bool ProcessForkedChain(const CBlock &block, CValidationState &state);

// Store block on disk
// if dbp is provided, the file is known to already reside on disk
bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp = NULL);

//disconnect block for test
bool DisconnectBlockFromTip(CValidationState &state);

//get tx operate account log
bool GetTxOperLog(const uint256 &txHash, vector<CAccountLog> &vAccountLog);

//get setBlockIndexValid
Value ListSetBlockIndexValid();

class CBlockFileInfo {
public:
    unsigned int nBlocks;       // number of blocks stored in file
    unsigned int nSize;         // number of used bytes of block file
    unsigned int nUndoSize;     // number of used bytes in the undo file
    unsigned int nHeightFirst;  // lowest height of block in file
    unsigned int nHeightLast;   // highest height of block in file
    uint64_t nTimeFirst;        // earliest time of block in file
    uint64_t nTimeLast;         // latest time of block in file

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nBlocks));
        READWRITE(VARINT(nSize));
        READWRITE(VARINT(nUndoSize));
        READWRITE(VARINT(nHeightFirst));
        READWRITE(VARINT(nHeightLast));
        READWRITE(VARINT(nTimeFirst));
        READWRITE(VARINT(nTimeLast));)

    void SetNull() {
        nBlocks      = 0;
        nSize        = 0;
        nUndoSize    = 0;
        nHeightFirst = 0;
        nHeightLast  = 0;
        nTimeFirst   = 0;
        nTimeLast    = 0;
    }

    CBlockFileInfo() {
        SetNull();
    }

    string ToString() const {
        return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks, nSize, nHeightFirst, nHeightLast, DateTimeStrFormat("%Y-%m-%d", nTimeFirst).c_str(), DateTimeStrFormat("%Y-%m-%d", nTimeLast).c_str());
    }

    // update statistics (does not update nSize)
    void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn) {
        if (nBlocks == 0 || nHeightFirst > nHeightIn)
            nHeightFirst = nHeightIn;
        if (nBlocks == 0 || nTimeFirst > nTimeIn)
            nTimeFirst = nTimeIn;
        nBlocks++;
        if (nHeightIn > nHeightLast)
            nHeightLast = nHeightIn;
        if (nTimeIn > nTimeLast)
            nTimeLast = nTimeIn;
    }
};

enum BlockStatus {
    BLOCK_VALID_UNKNOWN      = 0,
    BLOCK_VALID_HEADER       = 1,  // parsed, version ok, hash satisfies claimed PoW, 1 <= vtx count <= max, timestamp not in future
    BLOCK_VALID_TREE         = 2,  // parent found, difficulty matches, timestamp >= median previous, checkpoint
    BLOCK_VALID_TRANSACTIONS = 3,  // only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids, sigops, size, merkle root
    BLOCK_VALID_CHAIN        = 4,  // outputs do not overspend inputs, no double spends, coinbase output ok, immature coinbase spends, BIP30
    BLOCK_VALID_SCRIPTS      = 5,  // scripts/signatures ok                      0000 0101
    BLOCK_VALID_MASK         = 7,  //                                            0000 0111

    BLOCK_HAVE_DATA = 8,   // full block available in blk*.dat           0000 1000
    BLOCK_HAVE_UNDO = 16,  // undo data available in rev*.dat            0001 0000
    BLOCK_HAVE_MASK = 24,  // BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO          0001 1000

    BLOCK_FAILED_VALID = 32,  // stage after last reached validness failed  0010 0000
    BLOCK_FAILED_CHILD = 64,  // descends from failed block                 0100 0000
    BLOCK_FAILED_MASK  = 96   // BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD    0110 0000
};

/** The block chain is a tree shaped structure starting with the
 * genesis block at the root, with each block potentially having multiple
 * candidates to be the next block. A blockindex may have multiple pprev pointing
 * to it, but at most one of them can be part of the currently active branch.
 */
class CBlockIndex {
public:
    // pointer to the hash of the block, if any. memory is owned by this CBlockIndex
    const uint256 *pBlockHash;

    // pointer to the index of the predecessor of this block
    CBlockIndex *pprev;

    // pointer to the index of some further predecessor of this block
    CBlockIndex *pskip;

    // height of the entry in the chain. The genesis block has height 0
    int nHeight;

    // Which # file this block is stored in (blk?????.dat)
    int nFile;

    // Byte offset within blk?????.dat where this block's data is stored
    unsigned int nDataPos;

    // Byte offset within rev?????.dat where this block's undo data is stored
    unsigned int nUndoPos;

    // (memory only) Total amount of work (expected number of hashes) in the chain up to and including this block
    arith_uint256 nChainWork;

    // Number of transactions in this block.
    // Note: in a potential headers-first mode, this number cannot be relied upon
    unsigned int nTx;

    // (memory only) Number of transactions in the chain up to and including this block
    unsigned int nChainTx;  // change to 64-bit type when necessary; won't happen before 2030

    // Verification status of this block. See enum BlockStatus
    unsigned int nStatus;

    //the block's fee
    uint64_t nBlockFee;

    // block header
    int nVersion;
    uint256 merkleRootHash;
    uint256 hashPos;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;
    int64_t nFuel;
    int nFuelRate;
    vector<unsigned char> vSignature;

    double dFeePerKb;
    // (memory only) Sequencial id assigned to distinguish order in which blocks are received.
    uint32_t nSequenceId;

    CBlockIndex() {
        pBlockHash  = NULL;
        pprev       = NULL;
        pskip       = NULL;
        nHeight     = 0;
        nFile       = 0;
        nDataPos    = 0;
        nUndoPos    = 0;
        nChainWork  = 0;
        nTx         = 0;
        nChainTx    = 0;
        nStatus     = 0;
        nSequenceId = 0;
        dFeePerKb   = 0.0;
        nBlockFee   = 0;  //add the block's fee

        nVersion       = 0;
        merkleRootHash = uint256();
        hashPos        = uint256();
        nTime          = 0;
        nBits          = 0;
        nNonce         = 0;
        nFuel          = 0;
        nFuelRate      = INIT_FUEL_RATES;
        vSignature.clear();
    }

    CBlockIndex(CBlock &block) {
        pBlockHash  = NULL;
        pprev       = NULL;
        pskip       = NULL;
        nHeight     = 0;
        nFile       = 0;
        nDataPos    = 0;
        nUndoPos    = 0;
        nChainWork  = 0;
        nTx         = 0;
        nChainTx    = 0;
        nStatus     = 0;
        nSequenceId = 0;
        nBlockFee   = block.GetFee();  //add the block's fee

        int64_t nTxSize(0);
        for (auto &pTx : block.vptx) {
            nTxSize += pTx->GetSerializeSize(SER_DISK, PROTOCOL_VERSION);
        }

        dFeePerKb = double((nBlockFee - block.GetFuel())) / (double(nTxSize / 1000.0));

        nVersion       = block.GetVersion();
        merkleRootHash = block.GetMerkleRootHash();
        nTime          = block.GetTime();
        nNonce         = block.GetNonce();
        nFuel          = block.GetFuel();
        nFuelRate      = block.GetFuelRate();
        vSignature     = block.GetSignature();
    }

    CDiskBlockPos GetBlockPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_DATA) {
            ret.nFile = nFile;
            ret.nPos  = nDataPos;
        }
        return ret;
    }

    CDiskBlockPos GetUndoPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_UNDO) {
            ret.nFile = nFile;
            ret.nPos  = nUndoPos;
        }
        return ret;
    }

    CBlockHeader GetBlockHeader() const {
        CBlockHeader block;
        block.SetVersion(nVersion);
        if (pprev)
            block.SetPrevBlockHash(pprev->GetBlockHash());
        block.SetMerkleRootHash(merkleRootHash);
        block.SetTime(nTime);
        block.SetNonce(nNonce);
        block.SetHeight(nHeight);
        block.SetSignature(vSignature);
        return block;
    }

    int64_t GetBlockFee() const {
        return nBlockFee;
    }

    uint256 GetBlockHash() const {
        return *pBlockHash;
    }

    int64_t GetBlockTime() const {
        return (int64_t)nTime;
    }

    bool CheckIndex() const {
        return CheckProofOfWork(GetBlockHash(), nBits);
    }

    enum { nMedianTimeSpan = 11 };

    int64_t GetMedianTimePast() const {
        int64_t pmedian[nMedianTimeSpan];
        int64_t *pbegin = &pmedian[nMedianTimeSpan];
        int64_t *pend   = &pmedian[nMedianTimeSpan];

        const CBlockIndex *pIndex = this;
        for (int i = 0; i < nMedianTimeSpan && pIndex; i++, pIndex = pIndex->pprev)
            *(--pbegin) = pIndex->GetBlockTime();

        sort(pbegin, pend);
        return pbegin[(pend - pbegin) / 2];
    }

    int64_t GetMedianTime() const;

    /**
     * Returns true if there are nRequired or more blocks of minVersion or above
     * in the last nToCheck blocks, starting at pstart and going backwards.
     */
    static bool IsSuperMajority(int minVersion, const CBlockIndex *pstart,
                                unsigned int nRequired, unsigned int nToCheck);

    string ToString() const {
        return strprintf("CBlockIndex(pprev=%p, nHeight=%d, merkle=%s, blockHash=%s, blockFee=%d, chainWork=%s, feePerKb=%lf)",
                        pprev, nHeight, merkleRootHash.ToString().c_str(), GetBlockHash().ToString().c_str(),
                        nBlockFee, nChainWork.ToString().c_str(), dFeePerKb);
    }

    void Print() const {
        LogPrint("INFO", "%s\n", ToString().c_str());
    }

    // Build the skiplist pointer for this entry.
    void BuildSkip();

    // Efficiently find an ancestor of this block.
    CBlockIndex *GetAncestor(int height);
    const CBlockIndex *GetAncestor(int height) const;
};

/** Used to marshal pointers into hashes for db storage. */
class CDiskBlockIndex : public CBlockIndex {
public:
    uint256 hashPrev;

    CDiskBlockIndex() : hashPrev(uint256()) {}

    explicit CDiskBlockIndex(CBlockIndex *pIndex) : CBlockIndex(*pIndex) {
        hashPrev = (pprev ? pprev->GetBlockHash() : uint256());
    }

    IMPLEMENT_SERIALIZE(
        if (!(nType & SER_GETHASH))
            READWRITE(VARINT(nVersion));

        READWRITE(nBlockFee);
        READWRITE(VARINT(nHeight));
        READWRITE(VARINT(nStatus));
        READWRITE(VARINT(nTx));
        if (nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
            READWRITE(VARINT(nFile));
        if (nStatus & BLOCK_HAVE_DATA)
            READWRITE(VARINT(nDataPos));
        if (nStatus & BLOCK_HAVE_UNDO)
            READWRITE(VARINT(nUndoPos));

        // block header
        READWRITE(this->nVersion);
        READWRITE(hashPrev);
        READWRITE(merkleRootHash);
        READWRITE(hashPos);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
        READWRITE(nFuel);
        READWRITE(nFuelRate);
        READWRITE(vSignature);
        READWRITE(dFeePerKb);)

    uint256 GetBlockHash() const {
        CBlockHeader block;
        block.SetVersion(nVersion);
        block.SetPrevBlockHash(hashPrev);
        block.SetMerkleRootHash(merkleRootHash);
        block.SetTime(nTime);
        block.SetNonce(nNonce);
        block.SetHeight(nHeight);
        block.SetFuel(nFuel);
        block.SetFuelRate(nFuelRate);
        block.SetSignature(vSignature);
        return block.GetHash();
    }

    string ToString() const {
        string str = "CDiskBlockIndex(";
        str += CBlockIndex::ToString();
        str += strprintf("\n                blockHash=%s, hashPrev=%s)",
                         GetBlockHash().ToString().c_str(),
                         hashPrev.ToString().c_str());
        return str;
    }

    void Print() const {
        LogPrint("INFO", "%s\n", ToString().c_str());
    }
};

/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,    // everything ok
        MODE_INVALID,  // network rule violation (DoS value may be set)
        MODE_ERROR,    // run-time error
    } mode;
    int nDoS;
    string strRejectReason;
    unsigned char chRejectCode;
    bool corruptionPossible;

public:
    CValidationState() : mode(MODE_VALID), nDoS(0), corruptionPossible(false) {}
    bool DoS(int level, bool ret = false,
             unsigned char chRejectCodeIn = 0, string strRejectReasonIn = "",
             bool corruptionIn = false) {
        chRejectCode       = chRejectCodeIn;
        strRejectReason    = strRejectReasonIn;
        corruptionPossible = corruptionIn;
        if (mode == MODE_ERROR)
            return ret;
        nDoS += level;
        mode = MODE_INVALID;
        return ret;
    }
    bool Invalid(bool ret                    = false,
                 unsigned char _chRejectCode = 0, string _strRejectReason = "") {
        return DoS(0, ret, _chRejectCode, _strRejectReason);
    }
    bool Error(string strRejectReasonIn = "") {
        if (mode == MODE_VALID)
            strRejectReason = strRejectReasonIn;
        mode = MODE_ERROR;
        return false;
    }
    bool Abort(const string &msg) {
        AbortNode(msg);
        return Error(msg);
    }
    bool IsValid() const {
        return mode == MODE_VALID;
    }
    bool IsInvalid() const {
        return mode == MODE_INVALID;
    }
    bool IsError() const {
        return mode == MODE_ERROR;
    }
    bool IsInvalid(int &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const {
        return corruptionPossible;
    }
    unsigned char GetRejectCode() const { return chRejectCode; }
    string GetRejectReason() const { return strRejectReason; }
};

/** An in-memory indexed chain of blocks. */
class CChain {
private:
    vector<CBlockIndex *> vChain;

public:
    /** Returns the index entry for the genesis block of this chain, or NULL if none. */
    CBlockIndex *Genesis() const {
        return vChain.size() > 0 ? vChain[0] : NULL;
    }

    /** Returns the index entry for the tip of this chain, or NULL if none. */
    CBlockIndex *Tip() const {
        return vChain.size() > 0 ? vChain[vChain.size() - 1] : NULL;
    }

    /** Returns the index entry at a particular height in this chain, or NULL if no such height exists. */
    CBlockIndex *operator[](int nHeight) const {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return NULL;
        return vChain[nHeight];
    }

    /** Compare two chains efficiently. */
    friend bool operator==(const CChain &a, const CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.vChain.size() - 1] == b.vChain[b.vChain.size() - 1];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex *pIndex) const {
        return (*this)[pIndex->nHeight] == pIndex;
    }

    /** Find the successor of a block in this chain, or NULL if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pIndex) const {
        if (Contains(pIndex))
            return (*this)[pIndex->nHeight + 1];
        else
            return NULL;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->nHeight : -1. */
    int Height() const {
        return vChain.size() - 1;
    }

    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndex *SetTip(CBlockIndex *pIndex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pIndex = NULL) const;

    /** Find the last common block between this chain and a locator. */
    CBlockIndex *FindFork(const CBlockLocator &locator) const;
};

/** The currently-connected chain of blocks. */
extern CChain chainActive;

/** The currently best known chain of headers (some of which may be invalid). */
extern CChain chainMostWork;

/** Global variable that points to the active block tree (protected by cs_main) */
extern CBlockTreeDB *pblocktree;

/** account db cache*/
extern CAccountViewCache *pAccountViewTip;

/** account db */
extern CAccountViewDB *pAccountViewDB;

/** transaction db cache*/
extern CTransactionDB *pTxCacheDB;

/** srcipt db */
extern CScriptDB *pScriptDB;

/** tx db cache */
extern CTransactionDBCache *pTxCacheTip;

/** contract script db cache */
extern CScriptDBViewCache *pScriptDBTip;

/** nSyncTipHight  */
extern int nSyncTipHeight;

extern std::tuple<bool, boost::thread *> RunCoin(int argc, char *argv[]);
extern bool WriteBlockLog(bool falg, string suffix);

struct CBlockTemplate {
    CBlock block;
    vector<int64_t> vTxFees;
    vector<int64_t> vTxSigOps;
};

bool EraseBlockIndexFromSet(CBlockIndex *pIndex);

uint64_t GetBlockSubsidy(int nHeight);

/** Used to relay blocks as header + vector<merkle branch>
 * to filtered nodes.
 */
class CMerkleBlock {
public:
    // Public only for unit testing
    CBlockHeader header;
    CPartialMerkleTree txn;

public:
    // Public only for unit testing and relay testing
    // (not relayed)
    vector<pair<unsigned int, uint256> > vMatchedTxn;

    // Create from a CBlock, filtering transactions according to filter
    // Note that this will call IsRelevantAndUpdate on the filter for each transaction,
    // thus the filter will likely be modified.
    CMerkleBlock(const CBlock &block, CBloomFilter &filter);

    IMPLEMENT_SERIALIZE(
        READWRITE(header);
        READWRITE(txn);)
};

class CWalletInterface {
protected:
    virtual void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pblock) = 0;
    virtual void EraseTransaction(const uint256 &hash)                                        = 0;
    virtual void SetBestChain(const CBlockLocator &locator)                                   = 0;
    virtual void UpdatedTransaction(const uint256 &hash)                                      = 0;
    // virtual void Inventory(const uint256 &hash)                                               = 0;
    virtual void ResendWalletTransactions()                                                   = 0;
    friend void ::RegisterWallet(CWalletInterface *);
    friend void ::UnregisterWallet(CWalletInterface *);
    friend void ::UnregisterAllWallets();
};

extern CSignatureCache signatureCache;



//global overloadding fun
template <typename Stream>
void Serialize(Stream &os, const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char nTxType = pa->nTxType;
    Serialize(os, nTxType, nType, nVersion);
    if (pa->nTxType == ACCOUNT_REGISTER_TX) {
        Serialize(os, *((CAccountRegisterTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == BCOIN_TRANSFER_TX) {
        Serialize(os, *((CBaseCoinTransferTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == CONTRACT_INVOKE_TX) {
        Serialize(os, *((CContractInvokeTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == BLOCK_REWARD_TX) {
        Serialize(os, *((CBlockRewardTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == CONTRACT_DEPLOY_TX) {
        Serialize(os, *((CContractDeployTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == DELEGATE_VOTE_TX) {
        Serialize(os, *((CDelegateVoteTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == COMMON_MTX) {
        Serialize(os, *((CMulsigTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Serialize: nTxType (" + sTxType + ") value error.");
    }
}

//global overloadding fun
template <typename Stream>
void Unserialize(Stream &is, std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char nTxType;
    is.read((char *)&(nTxType), sizeof(nTxType));
    if (nTxType == ACCOUNT_REGISTER_TX) {
        pa = std::make_shared<CAccountRegisterTx>();
        Unserialize(is, *((CAccountRegisterTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == BCOIN_TRANSFER_TX) {
        pa = std::make_shared<CBaseCoinTransferTx>();
        Unserialize(is, *((CBaseCoinTransferTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == CONTRACT_INVOKE_TX) {
        pa = std::make_shared<CContractInvokeTx>();
        Unserialize(is, *((CContractInvokeTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == BLOCK_REWARD_TX) {
        pa = std::make_shared<CBlockRewardTx>();
        Unserialize(is, *((CBlockRewardTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == CONTRACT_DEPLOY_TX) {
        pa = std::make_shared<CContractDeployTx>();
        Unserialize(is, *((CContractDeployTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == DELEGATE_VOTE_TX) {
        pa = std::make_shared<CDelegateVoteTx>();
        Unserialize(is, *((CDelegateVoteTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == COMMON_MTX) {
        pa = std::make_shared<CMulsigTx>();
        Unserialize(is, *((CMulsigTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Unserialize: nTxType (" + sTxType + ") value error.");
    }
    pa->nTxType = nTxType;
}


#endif
