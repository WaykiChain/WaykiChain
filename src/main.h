// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
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
#include "net.h"
#include "sigcache.h"
#include "persistence/accountdb.h"
#include "persistence/block.h"
#include "persistence/dexdb.h"
#include "tx/txmempool.h"
#include "tx/accountregtx.h"
#include "tx/bcointx.h"
#include "tx/contracttx.h"
#include "tx/delegatetx.h"
#include "tx/blockrewardtx.h"
#include "tx/blockpricemediantx.h"
#include "tx/fcointx.h"
#include "tx/mulsigtx.h"
#include "tx/tx.h"
#include "persistence/accountdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "persistence/cachewrapper.h"

class CBlockIndex;
class CBloomFilter;
class CChain;
class CInv;
class CAccountCache;
class CBlockTreeDB;

extern CCriticalSection cs_main;
/** The currently-connected chain of blocks. */
extern CChain chainActive;
extern CSignatureCache signatureCache;

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

/** max size of signature of tx or block */
static const int MAX_BLOCK_SIGNATURE_SIZE = 100;
// -dbcache default (MiB)
static const int64_t nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t nMaxDbCache = sizeof(void *) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t nMinDbCache = 4;

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

static const unsigned char READ_ACCOUNT_FAIL    = 0X51;
static const unsigned char WRITE_ACCOUNT_FAIL   = 0X52;
static const unsigned char UPDATE_ACCOUNT_FAIL  = 0X53;

static const unsigned char PRICE_FEED_FAIL      = 0X54;
static const unsigned char FCOIN_STAKE_FAIL     = 0X55;

static const unsigned char READ_SCRIPT_FAIL     = 0X61;
static const unsigned char WRITE_SCRIPT_FAIL    = 0X62;

static const unsigned char STAKE_CDP_FAIL       = 0X71;
static const unsigned char REDEEM_CDP_FAIL      = 0X71;

static const uint64_t kTotalBaseCoinCount               = 210000000; // 210 million
static const uint64_t kInitialRiskProvisionScoinCount   = 2100000;   // 2 million 100 thousand
static const uint64_t kYearBlockCount                   = 3153600;   // one year = 365 * 24 * 60 * 60 / 10
static const uint64_t kMinDiskSpace                     = 52428800;  // Minimum disk space required
static const int kContractScriptMaxSize                 = 65536;     // 64 KB max for contract script size
static const int kContractArgumentMaxSize               = 4096;      // 4 KB max for contract argument size
static const int kCommonTxMemoMaxSize                   = 100;       // 100 bytes max for memo size
static const int kContractMemoMaxSize                   = 100;       // 100 bytes max for memo size
static const int kMostRecentBlockNumberThreshold        = 1000;      // most recent block number threshold

static const int kMultisigNumberThreshold               = 15;        // m-n multisig, refer to n
static const int KMultisigScriptMaxSize                 = 1000;      // multisig script max size
static const int kRegIdMaturePeriodByBlock              = 100;       // RegId's mature period measured by blocks

static const uint64_t kFcoinGenesisTxHeight             = 5880000;
static const uint64_t kFcoinGenesisRegisterTxIndex      = 1;
static const uint64_t kFcoinGenesisIssueTxIndex         = 2;

static const int kScoinInterestIncreaseRate             = 3;        // increase by 3%
static const int kBcoinDexSellOrderDiscount             = 97;       // 97%

const uint16_t kMaxMinedBlocks                          = 100;      // maximun cache size for mined blocks

static const string kContractScriptPathPrefix           = "/tmp/lua/";

extern CTxMemPool mempool;
extern map<uint256, CBlockIndex *> mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
extern const string strMessageMagic;

class CTxUndo;
class CValidationState;
class CWalletInterface;
class CTransactionCache;

struct CNodeStateStats;

/** Register a wallet to receive updates from core */
void RegisterWallet(CWalletInterface *pWalletIn);
/** Unregister a wallet from core */
void UnregisterWallet(CWalletInterface *pWalletIn);
/** Unregister all wallets from core */
void UnregisterAllWallets();
/** Push an updated transaction to all registered wallets */
void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pBlock = nullptr);
/** Erase Tx from wallets **/
void EraseTransaction(const uint256 &hash);
/** Register with a network node to receive its signals */
void RegisterNodeSignals(CNodeSignals &nodeSignals);
/** Unregister a network node */
void UnregisterNodeSignals(CNodeSignals &nodeSignals);
/** Check whether enough disk space is available for an incoming block */
bool CheckDiskSpace(uint64_t nAdditionalBytes = 0);

/** Verify consistency of the block and coin databases */
bool VerifyDB(int nCheckLevel, int nCheckDepth);

/** Process protocol messages received from a given node */
bool ProcessMessages(CNode *pFrom);
/** Send queued protocol messages to be sent to a give node */
bool SendMessages(CNode *pTo, bool fSendTrickle);
/** Run an instance of the script checking thread */
void ThreadScriptCheck();

/** Calculate the minimum amount of work a received block needs, without knowing its direct parent */
unsigned int ComputeMinWork(unsigned int nBase, int64_t nTime);

/** Format a string that describes several potential problems detected by the core */
string GetWarnings(string strFor);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CContractCache &scriptDBCache, bool bSearchMempool = true);
/** Retrieve a transaction height comfirmed in block*/
int GetTxConfirmHeight(const uint256 &hash, CContractCache &scriptDBCache);

/** Abort with a message */
bool AbortNode(const string &msg);
/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int howmuch);

bool VerifySignature(const uint256 &sigHash, const std::vector<unsigned char> &signature, const CPubKey &pubKey);

/** (try to) add transaction to memory pool **/
bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx,
                        bool fLimitFree, bool fRejectInsaneFee = false);

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
bool CheckTx(int nHeight, CBaseTx *ptx, CCacheWrapper &cacheWrapper, CValidationState &state);

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(CBaseTx *pBaseTx, string &reason);

bool IsFinalTx(CBaseTx *pBaseTx, int nBlockHeight = 0, int64_t nBlockTime = 0);

//get tx operate account log
bool GetTxOperLog(const uint256 &txHash, vector<CAccountLog> &accountLogs);

/** An in-memory indexed chain of blocks. */
class CChain {
private:
    vector<CBlockIndex *> vChain;

public:
    /** Returns the index entry for the genesis block of this chain, or nullptr if none. */
    CBlockIndex *Genesis() const {
        return vChain.size() > 0 ? vChain[0] : nullptr;
    }

    /** Returns the index entry for the tip of this chain, or nullptr if none. */
    CBlockIndex *Tip() const {
        return vChain.size() > 0 ? vChain[vChain.size() - 1] : nullptr;
    }

    /** Returns the index entry at a particular height in this chain, or nullptr if no such height exists. */
    CBlockIndex *operator[](int nHeight) const {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return nullptr;
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

    /** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pIndex) const {
        if (Contains(pIndex))
            return (*this)[pIndex->nHeight + 1];
        else
            return nullptr;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->nHeight : -1. */
    int Height() const {
        return vChain.size() - 1;
    }

    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndex *SetTip(CBlockIndex *pIndex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pIndex = nullptr) const;

    /** Find the last common block between this chain and a locator. */
    CBlockIndex *FindFork(const CBlockLocator &locator) const;
}; //end of CChain



class CCacheDBManager {
public:
    CDBAccess            *pAccountDb;
    CAccountCache       *pAccountCache;

    CDBAccess           *pContractDb;
    CContractCache      *pContractCache;

    CDBAccess           *pDelegateDb;
    CDelegateCache      *pDelegateCache;

    CDBAccess           *pCdpDb;
    CCdpCacheManager    *pCdpCache;

    CDexCache           *pDexCache;

    CBlockTreeDB        *pBlockTreeDb;

    CTransactionCache   *pTxCache;
    CPricePointCache    *pPpCache;

    uint64_t            collateralRatioMin = 200; //minimum collateral ratio

public:
    CCacheDBManager(bool fReIndex, bool fMemory, size_t nAccountDBCache, size_t nContractDBCache,
                    size_t nDelegateDBCache, size_t nBlockTreeDBCache) {
        //TODO fix cache size

        pBlockTreeDb = new CBlockTreeDB(nBlockTreeDBCache, false, fReIndex);

        pAccountDb      = new CDBAccess(DBNameType::ACCOUNT, nAccountDBCache, false, fReIndex);
        pAccountCache   = new CAccountCache(pAccountDb);

        pContractDb     = new CDBAccess(DBNameType::CONTRACT, nContractDBCache, false, fReIndex);
        pContractCache  = new CContractCache(pContractDb);

        pDelegateDb     = new CDBAccess(DBNameType::DELEGATE, nDelegateDBCache, false, fReIndex);
        pDelegateCache  = new CDelegateCache(pDelegateDb);

        pCdpDb          = new CDBAccess(DBNameType::CDP, nAccountDBCache, false, fReIndex); //TODO fix cache size
        pCdpCache       = new CCdpCacheManager(pCdpDb);

        pDexCache       = new CDexCache();

        pTxCache        = new CTransactionCache();
        pPpCache        = new CPricePointCache();

    }

    ~CCacheDBManager() {

        delete pAccountCache;   pAccountCache = nullptr;
        delete pContractCache;  pContractCache = nullptr;
        delete pDelegateCache;  pDelegateCache = nullptr;
        delete pTxCache;        pTxCache = nullptr;
        delete pPpCache;        pPpCache = nullptr;

        delete pAccountDb;      pAccountDb = nullptr;
        delete pContractDb;     pContractDb = nullptr;
        delete pDelegateDb;     pDelegateDb = nullptr;
        delete pBlockTreeDb;    pBlockTreeDb = nullptr;

        delete pCdpCache;       pCdpCache = nullptr;
        delete pCdpDb;          pCdpDb = nullptr;

        delete pDexCache;       pDexCache = nullptr;
    }

    bool Flush() {
        if (pAccountCache)
            pAccountCache->Flush();

        if (pContractCache)
            pContractCache->Flush();

        if (pBlockTreeDb)
            pBlockTreeDb->Flush();

        return true;
    }
}; //end of CCacheDBManager

bool IsInitialBlockDownload();

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);

/** Undo information for a CBlock */
class CBlockUndo {
public:
    vector<CTxUndo> vtxundo;

    IMPLEMENT_SERIALIZE(
        READWRITE(vtxundo);)

    bool WriteToDisk(CDiskBlockPos &pos, const uint256 &blockHash) {
        // Open history file to append
        CAutoFile fileout = CAutoFile(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
        if (!fileout)
            return ERRORMSG("CBlockUndo::WriteToDisk : OpenUndoFile failed");

        // Write index header
        unsigned int nSize = fileout.GetSerializeSize(*this);
        fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

        // Write undo data
        long fileOutPos = ftell(fileout);
        if (fileOutPos < 0)
            return ERRORMSG("CBlockUndo::WriteToDisk : ftell failed");
        pos.nPos = (unsigned int)fileOutPos;
        fileout << *this;

        // calculate & write checksum
        CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
        hasher << blockHash;
        hasher << *this;

        fileout << hasher.GetHash();

        // Flush stdio buffers and commit to disk before returning
        fflush(fileout);
        if (!IsInitialBlockDownload())
            FileCommit(fileout);

        return true;
    }

    bool ReadFromDisk(const CDiskBlockPos &pos, const uint256 &blockHash) {
        // Open history file to read
        CAutoFile filein = CAutoFile(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
        if (!filein)
            return ERRORMSG("CBlockUndo::ReadFromDisk : OpenBlockFile failed");

        // Read block
        uint256 hashChecksum;
        try {
            filein >> *this;
            filein >> hashChecksum;
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
        }

        // Verify checksum
        CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
        hasher << blockHash;
        hasher << *this;

        if (hashChecksum != hasher.GetHash())
            return ERRORMSG("CBlockUndo::ReadFromDisk : Checksum mismatch");
        return true;
    }

    string ToString() const;
};


/** Data structure that represents a partial merkle tree.
 *
 * It respresents a subset of the txid's of a known block, in a way that
 * allows recovery of the list of txid's and the merkle root, in an
 * authenticated way.
 *
 * The encoding works as follows: we traverse the tree in depth-first order,
 * storing a bit for each traversed node, signifying whether the node is the
 * parent of at least one matched leaf txid (or a matched txid itself). In
 * case we are at the leaf level, or this bit is 0, its merkle node hash is
 * stored, and its children are not explorer further. Otherwise, no hash is
 * stored, but we recurse into both (or the only) child branch. During
 * decoding, the same depth-first traversal is performed, consuming bits and
 * hashes as they written during encoding.
 *
 * The serialization is fixed and provides a hard guarantee about the
 * encoded size:
 *
 *   SIZE <= 10 + ceil(32.25*N)
 *
 * Where N represents the number of leaf nodes of the partial tree. N itself
 * is bounded by:
 *
 *   N <= total_transactions
 *   N <= 1 + matched_transactions*tree_height
 *
 * The serialization format:
 *  - uint32     total_transactions (4 bytes)
 *  - varint     number of hashes   (1-3 bytes)
 *  - uint256[]  hashes in depth-first order (<= 32*N bytes)
 *  - varint     number of bytes of flag bits (1-3 bytes)
 *  - byte[]     flag bits, packed per 8 in a byte, least significant bit first (<= 2*N-1 bits)
 * The size constraints follow from this.
 */
class CPartialMerkleTree {
protected:
    // the total number of transactions in the block
    unsigned int nTransactions;

    // node-is-parent-of-matched-txid bits
    vector<bool> vBits;

    // txids and internal hashes
    vector<uint256> vHash;

    // flag set when encountering invalid data
    bool fBad;

    // helper function to efficiently calculate the number of nodes at given height in the merkle tree
    unsigned int CalcTreeWidth(int height) {
        return (nTransactions + (1 << height) - 1) >> height;
    }

    // calculate the hash of a node in the merkle tree (at leaf level: the txid's themself)
    uint256 CalcHash(int height, unsigned int pos, const vector<uint256> &vTxid);

    // recursive function that traverses tree nodes, storing the data as bits and hashes
    void TraverseAndBuild(int height, unsigned int pos, const vector<uint256> &vTxid, const vector<bool> &vMatch);

    // recursive function that traverses tree nodes, consuming the bits and hashes produced by TraverseAndBuild.
    // it returns the hash of the respective node.
    uint256 TraverseAndExtract(int height, unsigned int pos, unsigned int &nBitsUsed, unsigned int &nHashUsed, vector<uint256> &vMatch);

public:
    // serialization implementation
    IMPLEMENT_SERIALIZE(
        READWRITE(nTransactions);
        READWRITE(vHash);
        vector<unsigned char> vBytes;
        if (fRead) {
            READWRITE(vBytes);
            CPartialMerkleTree &us = *(const_cast<CPartialMerkleTree *>(this));
            us.vBits.resize(vBytes.size() * 8);
            for (unsigned int p = 0; p < us.vBits.size(); p++)
                us.vBits[p] = (vBytes[p / 8] & (1 << (p % 8))) != 0;
            us.fBad = false;
        } else {
            vBytes.resize((vBits.size() + 7) / 8);
            for (unsigned int p = 0; p < vBits.size(); p++)
                vBytes[p / 8] |= vBits[p] << (p % 8);
            READWRITE(vBytes);
        })

    // Construct a partial merkle tree from a list of transaction id's, and a mask that selects a subset of them
    CPartialMerkleTree(const vector<uint256> &vTxid, const vector<bool> &vMatch);

    CPartialMerkleTree();

    // extract the matching txid's represented by this partial merkle tree.
    // returns the merkle root, or 0 in case of failure
    uint256 ExtractMatches(vector<uint256> &vMatch);
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

/** The currently best known chain of headers (some of which may be invalid). */
extern CChain chainMostWork;

extern CCacheDBManager *pCdMan;

/** nSyncTipHight  */
extern int nSyncTipHeight;

extern std::tuple<bool, boost::thread *> RunCoin(int argc, char *argv[]);
extern bool WriteBlockLog(bool falg, string suffix);

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
bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp = nullptr);

//disconnect block for test
bool DisconnectBlockFromTip(CValidationState &state);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState &state, CBlockIndex *pIndex);

int64_t GetBlockValue(int nHeight, int64_t nFees);
/** Open a block file (blk?????.dat) */
FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);

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
bool ActivateBestChain(CValidationState &state);

/** Remove invalidity status from a block and its descendants. */
bool ReconsiderBlock(CValidationState &state, CBlockIndex *pIndex);
/** Functions for disk access for blocks */
bool WriteBlockToDisk(CBlock &block, CDiskBlockPos &pos);
bool ReadBlockFromDisk(const CDiskBlockPos &pos, CBlock &block);
bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block);

// global overloadding fun
inline unsigned int GetSerializeSize(const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    return pa->GetSerializeSize(nType, nVersion) + 1;
}

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
    } else if (pa->nTxType == BLOCK_PRICE_MEDIAN_TX) {
        Serialize(os, *((CBlockPriceMedianTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Serialize: nTxType (" + sTxType + ") value error.");
    }
}

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
    } else if (nTxType == BLOCK_PRICE_MEDIAN_TX) {
        pa = std::make_shared<CBlockPriceMedianTx>();
        Unserialize(is, *((CBlockPriceMedianTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Unserialize: nTxType (" + sTxType + ") value error.");
    }
    pa->nTxType = nTxType;
}

#endif
