// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_CORE_H
#define COIN_CORE_H

#include "accounts/key.h"
#include "commons/serialize.h"
#include "commons/uint256.h"

#include "commons/base58.h"
#include <stdint.h>
#include <memory>
#include "configuration.h"

/** No amount larger than this (in sawi) is valid */
static const int64_t MAX_MONEY = IniCfg().GetCoinInitValue() * COIN;
static const int64_t MAX_MONEY_REG_NET = 5 * MAX_MONEY;
static const int g_BlockVersion = 1;

inline int64_t GetMaxMoney() { return (SysCfg().NetworkID() == REGTEST_NET ? MAX_MONEY_REG_NET : MAX_MONEY); }

inline bool CheckMoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= GetMaxMoney()); }

class CAccountViewCache;

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    static const int CURRENT_VERSION = g_BlockVersion;
protected:
    int nVersion;
    uint256 prevBlockHash;
    uint256 merkleRootHash;
    unsigned int nTime;
    unsigned int nNonce;
    unsigned int nHeight;
    int64_t    nFuel;
    int nFuelRate;
    vector<unsigned char> vSignature;

public:
    CBlockHeader()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(prevBlockHash);
        READWRITE(merkleRootHash);
        READWRITE(nTime);
        READWRITE(nNonce);
        READWRITE(nHeight);
        READWRITE(nFuel);
        READWRITE(nFuelRate);
        READWRITE(vSignature);
    )

    void SetNull()
    {
        nVersion = CBlockHeader::CURRENT_VERSION;
        prevBlockHash = uint256();
        merkleRootHash = uint256();
        nTime = 0;
        nNonce = 0;
        nHeight = 0;
        nFuel = 0;
        nFuelRate = 100;
        vSignature.clear();
    }

    uint256 GetHash() const;

    uint256 ComputeSignatureHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    int GetVersion() const  {
    	return  nVersion;
    }
    void SetVersion(int nVersion) {
    	this->nVersion = nVersion;
    }
    uint256 GetPrevBlockHash() const  {
    	return prevBlockHash;
    }
    void SetPrevBlockHash(uint256 prevBlockHash) {
    	this->prevBlockHash = prevBlockHash;
    }
    uint256 GetMerkleRootHash() const{
    	return merkleRootHash;
    }
    void SetMerkleRootHash(uint256 merkleRootHash) {
    	this->merkleRootHash = merkleRootHash;
    }

    unsigned int GetTime() const{
    	return nTime;
    }
    void SetTime(unsigned int time) {
    	this->nTime = time;
    }

    unsigned int GetNonce() const{
    	return nNonce;
    }
    void SetNonce(unsigned int nonce) {
    	this->nNonce = nonce;
    }
    unsigned int GetHeight() const{
    	return nHeight;
    }
    void SetHeight(unsigned int height);

    unsigned int GetFuel() const{
    	return nFuel;
    }
    void SetFuel(int64_t fuel) {
    	this->nFuel = fuel;
    }
    int GetFuelRate() const{
    	return nFuelRate;
    }
    void SetFuelRate(int fuelRalte) {
    	this->nFuelRate = fuelRalte;
    }
    const vector<unsigned char> &GetSignature() const{
    	return vSignature;
    }
    void SetSignature(const vector<unsigned char> &signature) {
    	this->vSignature = signature;
    }
    void ClearSignature() {
    	this->vSignature.clear();
    }
};

class CBlock : public CBlockHeader
{
public:
    // network and disk
    vector<std::shared_ptr<CBaseTx> > vptx;

    // memory only
    mutable vector<uint256> vMerkleTree;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vptx);
    )

    void SetNull()
    {
        CBlockHeader::SetNull();
        vptx.clear();
        vMerkleTree.clear();
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.SetVersion(nVersion);
        block.SetPrevBlockHash(prevBlockHash);
        block.SetMerkleRootHash(merkleRootHash);
        block.SetTime(nTime);
        block.SetNonce(nNonce);
        block.SetHeight(nHeight);
        block.SetFuel(nFuel);
        block.SetFuelRate(nFuelRate);
        block.SetSignature(vSignature);
        return block;
    }

    uint256 BuildMerkleTree() const;

    std::tuple<bool, int> GetTxIndex(const uint256& txHash) const;

    const uint256 &GetTxHash(unsigned int nIndex) const {
        assert(vMerkleTree.size() > 0); // BuildMerkleTree must have been called first
        assert(nIndex < vptx.size());
        return vMerkleTree[nIndex];
    }

    vector<uint256> GetMerkleBranch(int nIndex) const;
    static uint256 CheckMerkleBranch(uint256 hash, const vector<uint256>& vMerkleBranch, int nIndex);

    int64_t GetFee() const;

    void Print(CAccountViewCache &view) const;
};


/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull()
    {
        return vHave.empty();
    }
};

struct CDiskBlockPos {
    int nFile;
    unsigned int nPos;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nFile));
        READWRITE(VARINT(nPos));)

    CDiskBlockPos() {
        SetNull();
    }

    CDiskBlockPos(int nFileIn, unsigned int nPosIn) {
        nFile = nFileIn;
        nPos  = nPosIn;
    }

    friend bool operator==(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }

    friend bool operator!=(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return !(a == b);
    }

    void SetNull() {
        nFile = -1;
        nPos  = 0;
    }
    bool IsNull() const { return (nFile == -1); }
};

struct CDiskTxPos : public CDiskBlockPos {
    unsigned int nTxOffset;  // after header

    IMPLEMENT_SERIALIZE(
        READWRITE(*(CDiskBlockPos *)this);
        READWRITE(VARINT(nTxOffset));)

    CDiskTxPos(const CDiskBlockPos &blockIn, unsigned int nTxOffsetIn) : CDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskBlockPos::SetNull();
        nTxOffset = 0;
    }
};

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

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx {
private:
    int GetDepthInMainChainINTERNAL(CBlockIndex *&pindexRet) const;

public:
    uint256 blockHash;
    vector<uint256> vMerkleBranch;
    int nIndex;
    std::shared_ptr<CBaseTx> pTx;
    int nHeight;
    // memory only
    mutable bool fMerkleVerified;

    CMerkleTx() {
        Init();
    }

    CMerkleTx(std::shared_ptr<CBaseTx> pBaseTx) : pTx(pBaseTx) {
        Init();
    }

    void Init() {
        blockHash       = uint256();
        nIndex          = -1;
        fMerkleVerified = false;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(blockHash);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
        READWRITE(pTx);
        READWRITE(nHeight);)

    int SetMerkleBranch(const CBlock *pblock = NULL);

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(CBlockIndex *&pindexRet) const;
    int GetDepthInMainChain() const {
        CBlockIndex *pindexRet;
        return GetDepthInMainChain(pindexRet);
    }
    bool IsInMainChain() const {
        CBlockIndex *pindexRet;
        return GetDepthInMainChainINTERNAL(pindexRet) > 0;
    }
    int GetBlocksToMaturity() const;
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

#endif
