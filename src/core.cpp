// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"
#include "crypto/hash.h"
#include "main.h"
#include "util.h"

uint256 CBlockHeader::GetHash() const {
    return SignatureHash();
}

void CBlockHeader::SetHeight(unsigned int height) {
    this->nHeight = height;
}

uint256 CBlockHeader::SignatureHash() const {
    CHashWriter ss(SER_GETHASH, CLIENT_VERSION);
    ss << nVersion << prevBlockHash << merkleRootHash << nTime << nNonce << nHeight << nFuel << nFuelRate;
    return ss.GetHash();
}

uint256 CBlock::BuildMerkleTree() const {
    vMerkleTree.clear();
    for (const auto& ptx : vptx) {
        vMerkleTree.push_back(ptx->GetHash());
    }
    int j = 0;
    for (int nSize = vptx.size(); nSize > 1; nSize = (nSize + 1) / 2) {
        for (int i = 0; i < nSize; i += 2) {
            int i2 = min(i + 1, nSize - 1);
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j + i]), END(vMerkleTree[j + i]),
                                       BEGIN(vMerkleTree[j + i2]), END(vMerkleTree[j + i2])));
        }
        j += nSize;
    }
    return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
}

vector<uint256> CBlock::GetMerkleBranch(int nIndex) const {
    if (vMerkleTree.empty())
        BuildMerkleTree();
    vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = vptx.size(); nSize > 1; nSize = (nSize + 1) / 2) {
        int i = min(nIndex ^ 1, nSize - 1);
        vMerkleBranch.push_back(vMerkleTree[j + i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 CBlock::CheckMerkleBranch(uint256 hash, const vector<uint256>& vMerkleBranch, int nIndex) {
    if (nIndex == -1)
        return uint256();
    for (const auto& otherside : vMerkleBranch) {
        if (nIndex & 1)
            hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
        nIndex >>= 1;
    }
    return hash;
}

int64_t CBlock::GetFee() const {
    int64_t nFees = 0;
    for (unsigned int i = 1; i < vptx.size(); ++i) {
        nFees += vptx[i]->GetFee();
    }
    return nFees;
}

void CBlock::Print(CAccountViewCache& view) const {
    LogPrint("INFO", "CBlock(hash=%s, ver=%d, hashPrevBlock=%s, merkleRootHash=%s, nTime=%u, nNonce=%u, vtx=%u, nFuel=%d, nFuelRate=%d)\n",
             GetHash().ToString(),
             nVersion,
             prevBlockHash.ToString(),
             merkleRootHash.ToString(),
             nTime,
             nNonce,
             vptx.size(), nFuel, nFuelRate);
    // LogPrint("INFO", "list transactions:\n");
    // for (unsigned int i = 0; i < vptx.size(); i++) {
    //     LogPrint("INFO", "%s ", vptx[i]->ToString(view));
    // }
    // LogPrint("INFO", "  vMerkleTree: ");
    // for (unsigned int i = 0; i < vMerkleTree.size(); i++) {
    //     LogPrint("INFO", "%s ", vMerkleTree[i].ToString());
    // }
    // LogPrint("INFO","\n");
}

std::tuple<bool, int> CBlock::GetTxIndex(const uint256& txHash) const {
    for (size_t i = 0; i < vMerkleTree.size(); i++) {
        if (txHash == vMerkleTree[i]) {
            return std::make_tuple(true, i);
        }
    }

    return std::make_tuple(false, 0);
}
