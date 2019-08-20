// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_BLOCK_H
#define PERSIST_BLOCK_H

#include "commons/base58.h"
#include "commons/serialize.h"
#include "commons/uint256.h"
#include "config/configuration.h"
#include "config/const.h"
#include "entities/key.h"
#include "sync.h"
#include "tx/tx.h"
#include "disk.h"

#include <stdint.h>
#include <memory>


class CDiskBlockPos;
class CNode;
class CAccountDBCache;

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




/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader {
public:
    // header
    static const int32_t CURRENT_VERSION = INIT_BLOCK_VERSION;
protected:
    int32_t nVersion;
    uint256 prevBlockHash;
    uint256 merkleRootHash;
    uint32_t nTime;
    uint32_t nNonce;
    uint32_t height;
    uint64_t nFuel;
    uint32_t nFuelRate;
    vector<unsigned char> vSignature;

public:
    CBlockHeader() { SetNull(); }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(prevBlockHash);
        READWRITE(merkleRootHash);
        READWRITE(nTime);
        READWRITE(nNonce);
        READWRITE(height);
        READWRITE(nFuel);
        READWRITE(nFuelRate);
        READWRITE(vSignature);
    )

    void SetNull() {
        nVersion       = CBlockHeader::CURRENT_VERSION;
        prevBlockHash  = uint256();
        merkleRootHash = uint256();
        nTime          = 0;
        nNonce         = 0;
        height         = 0;
        nFuel          = 0;
        nFuelRate      = 100;
        vSignature.clear();
    }

    uint256 GetHash() const;
    TxID ComputeSignatureHash() const;
    int64_t GetBlockTime() const { return (int64_t)nTime; }
    int32_t GetVersion() const { return nVersion; }
    void SetVersion(int32_t nVersion) { this->nVersion = nVersion; }
    uint256 GetPrevBlockHash() const { return prevBlockHash; }
    void SetPrevBlockHash(uint256 prevBlockHash) { this->prevBlockHash = prevBlockHash; }
    uint256 GetMerkleRootHash() const { return merkleRootHash; }
    void SetMerkleRootHash(uint256 merkleRootHash) { this->merkleRootHash = merkleRootHash; }
    uint32_t GetTime() const { return nTime; }
    void SetTime(uint32_t time) { this->nTime = time; }
    uint32_t GetNonce() const { return nNonce; }
    void SetNonce(uint32_t nonce) { this->nNonce = nonce; }
    uint32_t GetHeight() const { return height; }
    void SetHeight(uint32_t height);
    uint32_t GetFuel() const { return nFuel; }
    void SetFuel(uint64_t fuel) { this->nFuel = fuel; }
    uint32_t GetFuelRate() const { return nFuelRate; }
    void SetFuelRate(uint32_t fuelRate) { this->nFuelRate = fuelRate; }
    const vector<unsigned char> &GetSignature() const { return vSignature; }
    void SetSignature(const vector<unsigned char> &signature) { this->vSignature = signature; }
    void ClearSignature() { this->vSignature.clear(); }
};

class CBlock : public CBlockHeader {
public:
    // network and disk
    vector<std::shared_ptr<CBaseTx> > vptx;

    // memory only
    mutable vector<uint256> vMerkleTree;

    CBlock() { SetNull(); }

    CBlock(const CBlockHeader &header) {
        SetNull();
        *((CBlockHeader *)this) = header;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(*(CBlockHeader *)this);
        READWRITE(vptx);
    )

    void SetNull() {
        CBlockHeader::SetNull();
        vptx.clear();
        vMerkleTree.clear();
    }

    CBlockHeader GetBlockHeader() const {
        CBlockHeader block;
        block.SetVersion(nVersion);
        block.SetPrevBlockHash(prevBlockHash);
        block.SetMerkleRootHash(merkleRootHash);
        block.SetTime(nTime);
        block.SetNonce(nNonce);
        block.SetHeight(height);
        block.SetFuel(nFuel);
        block.SetFuelRate(nFuelRate);
        block.SetSignature(vSignature);

        return block;
    }

    uint256 BuildMerkleTree() const;

    std::tuple<bool, int32_t> GetTxIndex(const uint256 &txid) const;

    const uint256 &GetTxid(uint32_t index) const {
        assert(vMerkleTree.size() > 0);  // BuildMerkleTree must have been called first
        assert(index < vptx.size());
        return vMerkleTree[index];
    }

    vector<uint256> GetMerkleBranch(int32_t index) const;
    static uint256 CheckMerkleBranch(uint256 hash, const vector<uint256> &vMerkleBranch, int32_t index);

    map<TokenSymbol, uint64_t> GetFees() const;
    map<CoinPricePair, uint64_t> GetBlockMedianPrice() const;

    void Print(CAccountDBCache &accountCache) const;
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
    int32_t height;

    // Which # file this block is stored in (blk?????.dat)
    int32_t nFile;

    // Byte offset within blk?????.dat where this block's data is stored
    uint32_t nDataPos;

    // Byte offset within rev?????.dat where this block's undo data is stored
    uint32_t nUndoPos;

    // (memory only) Total amount of work (expected number of hashes) in the chain up to and including this block
    arith_uint256 nChainWork;

    // Number of transactions in this block.
    // Note: in a potential headers-first mode, this number cannot be relied upon
    uint32_t nTx;

    // (memory only) Number of transactions in the chain up to and including this block
    uint32_t nChainTx;  // change to 64-bit type when necessary; won't happen before 2030

    // Verification status of this block. See enum BlockStatus
    uint32_t nStatus;

    // (memory only) Sequencial id assigned to distinguish order in which blocks are received.
    uint32_t nSequenceId;

    // block header
    int32_t nVersion;
    uint256 merkleRootHash;
    uint256 hashPos;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    uint64_t nFuel;
    uint32_t nFuelRate;
    vector<unsigned char> vSignature;


    CBlockIndex() {
        pBlockHash       = nullptr;
        pprev            = nullptr;
        pskip            = nullptr;
        height           = 0;
        nFile            = 0;
        nDataPos         = 0;
        nUndoPos         = 0;
        nChainWork       = 0;
        nTx              = 0;
        nChainTx         = 0;
        nStatus          = 0;
        nSequenceId      = 0;

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
        pBlockHash       = nullptr;
        pprev            = nullptr;
        pskip            = nullptr;
        height           = 0;
        nFile            = 0;
        nDataPos         = 0;
        nUndoPos         = 0;
        nChainWork       = 0;
        nTx              = 0;
        nChainTx         = 0;
        nStatus          = 0;
        nSequenceId      = 0;

        int64_t nTxSize = 0;
        for (auto &pTx : block.vptx) {
            nTxSize += pTx->GetSerializeSize(SER_DISK, PROTOCOL_VERSION);
        }

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
        block.SetHeight(height);
        block.SetSignature(vSignature);

        return block;
    }

    uint256 GetBlockHash() const { return *pBlockHash; }
    int64_t GetBlockTime() const { return (int64_t)nTime; }
    bool CheckIndex() const { return true; }

    enum { nMedianTimeSpan = 11 };

    int64_t GetMedianTimePast() const {
        int64_t pmedian[nMedianTimeSpan];
        int64_t *pbegin = &pmedian[nMedianTimeSpan];
        int64_t *pend   = &pmedian[nMedianTimeSpan];

        const CBlockIndex *pIndex = this;
        for (int32_t i = 0; i < nMedianTimeSpan && pIndex; i++, pIndex = pIndex->pprev)
            *(--pbegin) = pIndex->GetBlockTime();

        sort(pbegin, pend);
        return pbegin[(pend - pbegin) / 2];
    }

    int64_t GetMedianTime() const;

    /**
     * Returns true if there are nRequired or more blocks of minVersion or above
     * in the last nToCheck blocks, starting at pstart and going backwards.
     */
    static bool IsSuperMajority(int32_t minVersion, const CBlockIndex *pstart,
                                uint32_t nRequired, uint32_t nToCheck);

    string ToString() const {
        return strprintf("CBlockIndex(pprev=%p, height=%d, merkle=%s, blockHash=%s, chainWork=%s)", pprev, height,
                         merkleRootHash.ToString(), GetBlockHash().ToString(), nChainWork.ToString());
    }

    void Print() const { LogPrint("INFO", "%s\n", ToString()); }

    // Build the skiplist pointer for this entry.
    void BuildSkip();

    // Efficiently find an ancestor of this block.
    CBlockIndex *GetAncestor(int32_t heightIn);
    const CBlockIndex *GetAncestor(int32_t heightIn) const;
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

        READWRITE(VARINT(height));
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
        READWRITE(vSignature);)
        // TODO: Fees
        // READWRITE(dFeePerKb);)

    uint256 GetBlockHash() const {
        CBlockHeader block;
        block.SetVersion(nVersion);
        block.SetPrevBlockHash(hashPrev);
        block.SetMerkleRootHash(merkleRootHash);
        block.SetTime(nTime);
        block.SetNonce(nNonce);
        block.SetHeight(height);
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

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator {
    vector<uint256> vHave;

    CBlockLocator() {}
    CBlockLocator(const vector<uint256> &vHaveIn) { vHave = vHaveIn; }

    IMPLEMENT_SERIALIZE(
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void SetNull() { vHave.clear(); }
    bool IsNull() { return vHave.empty(); }
};

#endif  // PERSIST_BLOCK_H