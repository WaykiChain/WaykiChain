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
#include "net.h"
#include "persistence/accountdb.h"
#include "persistence/block.h"
#include "persistence/blockdb.h"
#include "persistence/cachewrapper.h"
#include "persistence/delegatedb.h"
#include "persistence/dexdb.h"
#include "persistence/logdb.h"
#include "persistence/pricefeeddb.h"
#include "persistence/txdb.h"
#include "sigcache.h"
#include "tx/accountregtx.h"
#include "tx/cointransfertx.h"
#include "tx/blockpricemediantx.h"
#include "tx/blockrewardtx.h"
#include "tx/cdptx.h"
#include "tx/coinrewardtx.h"
#include "tx/cointransfertx.h"
#include "tx/contracttx.h"
#include "tx/delegatetx.h"
#include "tx/dextx.h"
#include "tx/fcoinstaketx.h"
#include "tx/mulsigtx.h"
#include "tx/pricefeedtx.h"
#include "tx/tx.h"
#include "tx/txmempool.h"
#include "tx/assettx.h"

class CBlockIndex;
class CBloomFilter;
class CChain;
class CInv;
class CAccountDBCache;
class CBlockTreeDB;
class CSysParamDBCache;

extern CCriticalSection cs_main;
/** The currently-connected chain of blocks. */
extern CChain chainActive;
extern CSignatureCache signatureCache;

extern CTxMemPool mempool;
extern map<uint256, CBlockIndex *> mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
extern const string strMessageMagic;

class CTxUndo;
class CValidationState;
class CWalletInterface;
class CTxMemCache;
class CUserCDP;

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
bool VerifyDB(int32_t nCheckLevel, int32_t nCheckDepth);

/** Process protocol messages received from a given node */
bool ProcessMessages(CNode *pFrom);
/** Send queued protocol messages to be sent to a give node */
bool SendMessages(CNode *pTo, bool fSendTrickle);
/** Run an instance of the script checking thread */
void ThreadScriptCheck();

/** Format a string that describes several potential problems detected by the core */
string GetWarnings(string strFor);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CContractDBCache &scriptDBCache, bool bSearchMempool = true);
/** Retrieve a transaction height comfirmed in block*/
int32_t GetTxConfirmHeight(const uint256 &hash, CContractDBCache &scriptDBCache);

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

std::shared_ptr<CBaseTx> CreateNewEmptyTransaction(uint8_t txType);

struct CNodeStateStats {
    int32_t nMisbehavior;
};

int64_t GetMinRelayFee(const CBaseTx *pBaseTx, uint32_t nBytes, bool fAllowFree);

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(CBaseTx *pBaseTx, string &reason);

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
    CBlockIndex *operator[](int32_t height) const {
        if (height < 0 || height >= (int)vChain.size())
            return nullptr;
        return vChain[height];
    }

    /** Compare two chains efficiently. */
    friend bool operator==(const CChain &a, const CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.vChain.size() - 1] == b.vChain[b.vChain.size() - 1];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex *pIndex) const {
        return (*this)[pIndex->height] == pIndex;
    }

    /** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pIndex) const {
        if (Contains(pIndex))
            return (*this)[pIndex->height + 1];
        else
            return nullptr;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->height : -1. */
    int32_t Height() const {
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
    CDBAccess           *pSysParamDb;
    CSysParamDBCache    *pSysParamCache;

    CDBAccess           *pAccountDb;
    CAccountDBCache     *pAccountCache;
    CDBAccess           *pAssetDb;
    CAssetDBCache       *pAssetCache;

    CDBAccess           *pContractDb;
    CContractDBCache    *pContractCache;

    CDBAccess           *pDelegateDb;
    CDelegateDBCache    *pDelegateCache;

    CDBAccess           *pCdpDb;
    CCDPDBCache         *pCdpCache;

    CDBAccess           *pDexDb;
    CDexDBCache         *pDexCache;

    CBlockTreeDB        *pBlockTreeDb;

    CDBAccess           *pLogDb;
    CLogDBCache         *pLogCache;

    CDBAccess           *pTxReceiptDb;
    CTxReceiptDBCache   *pTxReceiptCache;

    CTxMemCache         *pTxCache;
    CPricePointMemCache *pPpCache;

public:
    CCacheDBManager(bool fReIndex, bool fMemory, size_t nAccountDBCache, size_t nContractDBCache,
                    size_t nDelegateDBCache, size_t nBlockTreeDBCache) {
        pSysParamDb     = new CDBAccess(DBNameType::SYSPARAM, nAccountDBCache, false, fReIndex);  // TODO fix cache size
        pSysParamCache  = new CSysParamDBCache(pSysParamDb);

        pAccountDb      = new CDBAccess(DBNameType::ACCOUNT, nAccountDBCache, false, fReIndex);
        pAccountCache   = new CAccountDBCache(pAccountDb);

        pAssetDb        = new CDBAccess(DBNameType::ASSET, nAccountDBCache, false, fReIndex); //TODO fix cache size
        pAssetCache     = new CAssetDBCache(pAssetDb);

        pContractDb     = new CDBAccess(DBNameType::CONTRACT, nContractDBCache, false, fReIndex);
        pContractCache  = new CContractDBCache(pContractDb);

        pDelegateDb     = new CDBAccess(DBNameType::DELEGATE, nDelegateDBCache, false, fReIndex);
        pDelegateCache  = new CDelegateDBCache(pDelegateDb);

        pCdpDb          = new CDBAccess(DBNameType::CDP, nAccountDBCache, false, fReIndex); //TODO fix cache size
        pCdpCache       = new CCDPDBCache(pCdpDb);

        pDexDb          = new CDBAccess(DBNameType::DEX, nAccountDBCache, false, fReIndex); //TODO fix cache size
        pDexCache       = new CDexDBCache(pDexDb);

        pBlockTreeDb    = new CBlockTreeDB(nBlockTreeDBCache, false, fReIndex);

        pLogDb          = new CDBAccess(DBNameType::LOG, nAccountDBCache, false, fReIndex); //TODO fix cache size
        pLogCache       = new CLogDBCache(pLogDb);

        pTxReceiptDb    = new CDBAccess(DBNameType::RECEIPT, nAccountDBCache, false, fReIndex); //TODO fix cache size
        pTxReceiptCache = new CTxReceiptDBCache(pTxReceiptDb);

        // memory-only cache
        pTxCache        = new CTxMemCache();
        pPpCache        = new CPricePointMemCache();
    }

    ~CCacheDBManager() {
        delete pSysParamCache;  pSysParamCache = nullptr;
        delete pAccountCache;   pAccountCache = nullptr;
        delete pAssetCache;     pAssetCache = nullptr;
        delete pContractCache;  pContractCache = nullptr;
        delete pDelegateCache;  pDelegateCache = nullptr;
        delete pCdpCache;       pCdpCache = nullptr;
        delete pDexCache;       pDexCache = nullptr;
        delete pLogCache;       pLogCache = nullptr;
        delete pTxReceiptCache; pTxReceiptCache = nullptr;

        delete pSysParamDb;     pSysParamDb = nullptr;
        delete pAccountDb;      pAccountDb = nullptr;
        delete pAssetDb;        pAssetDb = nullptr;
        delete pContractDb;     pContractDb = nullptr;
        delete pDelegateDb;     pDelegateDb = nullptr;
        delete pBlockTreeDb;    pBlockTreeDb = nullptr;
        delete pCdpDb;          pCdpDb = nullptr;
        delete pDexDb;          pDexDb = nullptr;
        delete pLogDb;          pLogDb = nullptr;
        delete pTxReceiptDb;    pTxReceiptDb = nullptr;

        // memory-only cache
        delete pTxCache;        pTxCache = nullptr;
        delete pPpCache;        pPpCache = nullptr;
    }

    bool Flush() {
        if (pSysParamCache) pSysParamCache->Flush();

        if (pAccountCache) pAccountCache->Flush();

        if (pAssetCache) pAssetCache->Flush();

        if (pContractCache) pContractCache->Flush();

        if (pDelegateCache) pDelegateCache->Flush();

        if (pCdpCache) pCdpCache->Flush();

        if (pDexCache) pDexCache->Flush();

        if (pBlockTreeDb) pBlockTreeDb->Flush();

        if (pLogCache) pLogCache->Flush();

        if (pTxReceiptCache) pTxReceiptCache->Flush();

        // Memory only cache, not bother to flush.
        // if (pTxCache)
        //     pTxCache->Flush();
        // if (pPpCache)
        //     pPpCache->Flush();

        return true;
    }
};  // CCacheDBManager

bool IsInitialBlockDownload();

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);

/** Undo information for a CBlock */
class CBlockUndo {
public:
    vector<CTxUndo> vtxundo;

    IMPLEMENT_SERIALIZE(
        READWRITE(vtxundo);
    )

    bool WriteToDisk(CDiskBlockPos &pos, const uint256 &blockHash) {
        // Open history file to append
        CAutoFile fileout = CAutoFile(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
        if (!fileout)
            return ERRORMSG("CBlockUndo::WriteToDisk : OpenUndoFile failed");

        // Write index header
        uint32_t nSize = fileout.GetSerializeSize(*this);
        fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

        // Write undo data
        long fileOutPos = ftell(fileout);
        if (fileOutPos < 0)
            return ERRORMSG("CBlockUndo::WriteToDisk : ftell failed");
        pos.nPos = (uint32_t)fileOutPos;
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
    uint32_t nTransactions;

    // node-is-parent-of-matched-txid bits
    vector<bool> vBits;

    // txids and internal hashes
    vector<uint256> vHash;

    // flag set when encountering invalid data
    bool fBad;

    // helper function to efficiently calculate the number of nodes at given height in the merkle tree
    uint32_t CalcTreeWidth(int32_t height) {
        return (nTransactions + (1 << height) - 1) >> height;
    }

    // calculate the hash of a node in the merkle tree (at leaf level: the txid's themself)
    uint256 CalcHash(int32_t height, uint32_t pos, const vector<uint256> &vTxid);

    // recursive function that traverses tree nodes, storing the data as bits and hashes
    void TraverseAndBuild(int32_t height, uint32_t pos, const vector<uint256> &vTxid, const vector<bool> &vMatch);

    // recursive function that traverses tree nodes, consuming the bits and hashes produced by TraverseAndBuild.
    // it returns the hash of the respective node.
    uint256 TraverseAndExtract(int32_t height, uint32_t pos, uint32_t &nBitsUsed, uint32_t &nHashUsed, vector<uint256> &vMatch);

public:
    // serialization implementation
    IMPLEMENT_SERIALIZE(
        READWRITE(nTransactions);
        READWRITE(vHash);
        vector<uint8_t> vBytes;
        if (fRead) {
            READWRITE(vBytes);
            CPartialMerkleTree &us = *(const_cast<CPartialMerkleTree *>(this));
            us.vBits.resize(vBytes.size() * 8);
            for (uint32_t p = 0; p < us.vBits.size(); p++)
                us.vBits[p] = (vBytes[p / 8] & (1 << (p % 8))) != 0;
            us.fBad = false;
        } else {
            vBytes.resize((vBits.size() + 7) / 8);
            for (uint32_t p = 0; p < vBits.size(); p++)
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
    int32_t nDoS;
    string rejectReason;
    uint8_t rejectCode;
    bool corruptionPossible;

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
};

/** The currently best known chain of headers (some of which may be invalid). */
extern CChain chainMostWork;
extern CCacheDBManager *pCdMan;
/** nSyncTipHight  */
extern int32_t nSyncTipHeight;
extern std::tuple<bool, boost::thread *> RunCoin(int32_t argc, char *argv[]);

bool EraseBlockIndexFromSet(CBlockIndex *pIndex);

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
    vector<pair<uint32_t, uint256> > vMatchedTxn;

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


bool ReadBaseTxFromDisk(const CTxCord txCord, std::shared_ptr<CBaseTx> &pTx);

template<typename TxType>
bool ReadTxFromDisk(const CTxCord txCord, std::shared_ptr<TxType> &pTx) {
    std::shared_ptr<CBaseTx> pBaseTx;
    if (!ReadBaseTxFromDisk(txCord, pBaseTx)) {
        return ERRORMSG("ReadTxFromDisk failed! txcord(%s)", txCord.ToString());
    }
    assert(pBaseTx);
    pTx = dynamic_pointer_cast<TxType>(pBaseTx);
    if (!pTx) {
        return ERRORMSG("The expected tx(%s) type is %s, but read tx type is %s",
            txCord.ToString(), typeid(TxType).name(), typeid(*pBaseTx).name());
    }
    return true;
}

// global overloadding fun
inline uint32_t GetSerializeSize(const std::shared_ptr<CBaseTx> &pa, int32_t nType, int32_t nVersion) {
    return pa->GetSerializeSize(nType, nVersion) + 1;
}

template <typename Stream>
void Serialize(Stream &os, const std::shared_ptr<CBaseTx> &pa, int32_t nType, int32_t nVersion) {
    uint8_t nTxType = pa->nTxType;
    Serialize(os, nTxType, nType, nVersion);
    switch (pa->nTxType) {
        case BLOCK_REWARD_TX:
            Serialize(os, *((CBlockRewardTx *)(pa.get())), nType, nVersion); break;
        case ACCOUNT_REGISTER_TX:
            Serialize(os, *((CAccountRegisterTx *)(pa.get())), nType, nVersion); break;
        case BCOIN_TRANSFER_TX:
            Serialize(os, *((CBaseCoinTransferTx *)(pa.get())), nType, nVersion); break;
        case LCONTRACT_INVOKE_TX:
            Serialize(os, *((CLuaContractInvokeTx *)(pa.get())), nType, nVersion); break;
        case LCONTRACT_DEPLOY_TX:
            Serialize(os, *((CLuaContractDeployTx *)(pa.get())), nType, nVersion); break;
        case DELEGATE_VOTE_TX:
            Serialize(os, *((CDelegateVoteTx *)(pa.get())), nType, nVersion); break;

        case BCOIN_TRANSFER_MTX:
            Serialize(os, *((CMulsigTx *)(pa.get())), nType, nVersion); break;
        case FCOIN_STAKE_TX:
            Serialize(os, *((CFcoinStakeTx *)(pa.get())), nType, nVersion); break;
        case ASSET_ISSUE_TX:
            Serialize(os, *((CAssetIssueTx *)(pa.get())), nType, nVersion); break;
        case ASSET_UPDATE_TX:
            Serialize(os, *((CAssetUpdateTx *)(pa.get())), nType, nVersion); break;

        case UCOIN_TRANSFER_TX:
            Serialize(os, *((CCoinTransferTx *)(pa.get())), nType, nVersion); break;
        case UCOIN_REWARD_TX:
            Serialize(os, *((CCoinRewardTx *)(pa.get())), nType, nVersion); break;
        case UCOIN_BLOCK_REWARD_TX:
            Serialize(os, *((CUCoinBlockRewardTx *)(pa.get())), nType, nVersion); break;
        case PRICE_FEED_TX:
            Serialize(os, *((CPriceFeedTx *)(pa.get())), nType, nVersion); break;
        case PRICE_MEDIAN_TX:
            Serialize(os, *((CBlockPriceMedianTx *)(pa.get())), nType, nVersion); break;

        case CDP_STAKE_TX:
            Serialize(os, *((CCDPStakeTx *)(pa.get())), nType, nVersion); break;
        case CDP_REDEEM_TX:
            Serialize(os, *((CCDPRedeemTx *)(pa.get())), nType, nVersion); break;
        case CDP_LIQUIDATE_TX:
            Serialize(os, *((CCDPLiquidateTx *)(pa.get())), nType, nVersion); break;

        case DEX_TRADE_SETTLE_TX:
            Serialize(os, *((CDEXSettleTx *)(pa.get())), nType, nVersion); break;
        case DEX_CANCEL_ORDER_TX:
            Serialize(os, *((CDEXCancelOrderTx *)(pa.get())), nType, nVersion); break;
        case DEX_LIMIT_BUY_ORDER_TX:
            Serialize(os, *((CDEXBuyLimitOrderTx *)(pa.get())), nType, nVersion); break;
        case DEX_LIMIT_SELL_ORDER_TX:
            Serialize(os, *((CDEXSellLimitOrderTx *)(pa.get())), nType, nVersion); break;
        case DEX_MARKET_BUY_ORDER_TX:
            Serialize(os, *((CDEXBuyMarketOrderTx *)(pa.get())), nType, nVersion); break;
        case DEX_MARKET_SELL_ORDER_TX:
            Serialize(os, *((CDEXSellMarketOrderTx *)(pa.get())), nType, nVersion); break;

        default:
            throw ios_base::failure(strprintf("Serialize: nTxType(%d:%s) error.",
                pa->nTxType, GetTxType(pa->nTxType)));
            break;
    }

}


template <typename Stream>
void Unserialize(Stream &is, std::shared_ptr<CBaseTx> &pa, int32_t nType, int32_t nVersion) {
    uint8_t nTxType;
    is.read((char *)&(nTxType), sizeof(nTxType));
    switch((TxType)nTxType) {
        case BLOCK_REWARD_TX: {
            pa = std::make_shared<CBlockRewardTx>();
            Unserialize(is, *((CBlockRewardTx *)(pa.get())), nType, nVersion);
            break;
        }
        case ACCOUNT_REGISTER_TX: {
            pa = std::make_shared<CAccountRegisterTx>();
            Unserialize(is, *((CAccountRegisterTx *)(pa.get())), nType, nVersion);
            break;
        }
        case BCOIN_TRANSFER_TX: {
            pa = std::make_shared<CBaseCoinTransferTx>();
            Unserialize(is, *((CBaseCoinTransferTx *)(pa.get())), nType, nVersion);
            break;
        }
        case LCONTRACT_INVOKE_TX: {
            pa = std::make_shared<CLuaContractInvokeTx>();
            Unserialize(is, *((CLuaContractInvokeTx *)(pa.get())), nType, nVersion);
            break;
        }
        case LCONTRACT_DEPLOY_TX: {
            pa = std::make_shared<CLuaContractDeployTx>();
            Unserialize(is, *((CLuaContractDeployTx *)(pa.get())), nType, nVersion);
            break;
        }
        case DELEGATE_VOTE_TX: {
            pa = std::make_shared<CDelegateVoteTx>();
            Unserialize(is, *((CDelegateVoteTx *)(pa.get())), nType, nVersion);
            break;
        }

        case BCOIN_TRANSFER_MTX: {
            pa = std::make_shared<CMulsigTx>();
            Unserialize(is, *((CMulsigTx *)(pa.get())), nType, nVersion);
            break;
        }

        case FCOIN_STAKE_TX: {
            pa = std::make_shared<CFcoinStakeTx>();
            Unserialize(is, *((CFcoinStakeTx *)(pa.get())), nType, nVersion);
            break;
        }

        case ASSET_ISSUE_TX: {
            pa = std::make_shared<CAssetIssueTx>();
            Unserialize(is, *((CAssetIssueTx *)(pa.get())), nType, nVersion);
            break;
        }

        case ASSET_UPDATE_TX: {
            pa = std::make_shared<CAssetUpdateTx>();
            Unserialize(is, *((CAssetUpdateTx *)(pa.get())), nType, nVersion);
            break;
        }

        case UCOIN_TRANSFER_TX: {
            pa = std::make_shared<CCoinTransferTx>();
            Unserialize(is, *((CCoinTransferTx *)(pa.get())), nType, nVersion);
            break;
        }
        case UCOIN_REWARD_TX: {
            pa = std::make_shared<CCoinRewardTx>();
            Unserialize(is, *((CCoinRewardTx *)(pa.get())), nType, nVersion);
            break;
        }
        case UCOIN_BLOCK_REWARD_TX: {
            pa = std::make_shared<CUCoinBlockRewardTx>();
            Unserialize(is, *((CUCoinBlockRewardTx *)(pa.get())), nType, nVersion);
            break;
        }
        case PRICE_FEED_TX: {
            pa = std::make_shared<CPriceFeedTx>();
            Unserialize(is, *((CPriceFeedTx *)(pa.get())), nType, nVersion);
            break;
        }
        case PRICE_MEDIAN_TX: {
            pa = std::make_shared<CBlockPriceMedianTx>();
            Unserialize(is, *((CBlockPriceMedianTx *)(pa.get())), nType, nVersion);
            break;
        }

        case CDP_STAKE_TX: {
            pa = std::make_shared<CCDPStakeTx>();
            Unserialize(is, *((CCDPStakeTx *)(pa.get())), nType, nVersion);
            break;
        }
        case CDP_REDEEM_TX: {
            pa = std::make_shared<CCDPRedeemTx>();
            Unserialize(is, *((CCDPRedeemTx *)(pa.get())), nType, nVersion);
            break;
        }
        case CDP_LIQUIDATE_TX: {
            pa = std::make_shared<CCDPLiquidateTx>();
            Unserialize(is, *((CCDPLiquidateTx *)(pa.get())), nType, nVersion);
            break;
        }

        case DEX_TRADE_SETTLE_TX: {
            pa = std::make_shared<CDEXSettleTx>();
            Unserialize(is, *((CDEXSettleTx *)(pa.get())), nType, nVersion);
            break;
        }
        case DEX_CANCEL_ORDER_TX: {
            pa = std::make_shared<CDEXCancelOrderTx>();
            Unserialize(is, *((CDEXCancelOrderTx *)(pa.get())), nType, nVersion);
            break;
        }
        case DEX_LIMIT_BUY_ORDER_TX: {
            pa = std::make_shared<CDEXBuyLimitOrderTx>();
            Unserialize(is, *((CDEXBuyLimitOrderTx *)(pa.get())), nType, nVersion);
            break;
        }
        case DEX_LIMIT_SELL_ORDER_TX: {
            pa = std::make_shared<CDEXSellLimitOrderTx>();
            Unserialize(is, *((CDEXSellLimitOrderTx *)(pa.get())), nType, nVersion);
            break;
        }
        case DEX_MARKET_BUY_ORDER_TX: {
            pa = std::make_shared<CDEXBuyMarketOrderTx>();
            Unserialize(is, *((CDEXBuyMarketOrderTx *)(pa.get())), nType, nVersion);
            break;
        }
        case DEX_MARKET_SELL_ORDER_TX: {
            pa = std::make_shared<CDEXSellMarketOrderTx>();
            Unserialize(is, *((CDEXSellMarketOrderTx *)(pa.get())), nType, nVersion);
            break;
        }
        default:
            throw ios_base::failure(strprintf("Unserialize: nTxType(%d:%s) error.",
                nTxType, GetTxType((TxType)nTxType)));
    }
    pa->nTxType = TxType(nTxType);
}

#endif
