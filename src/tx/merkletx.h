// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MERKLETX_H
#define COIN_MERKLETX_H

#include "persistence/block.h"

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx {
private:
    int GetDepthInMainChainINTERNAL(CBlockIndex *&pIndexRet) const;

public:
    uint256 blockHash;
    vector<uint256> vMerkleBranch;
    int index;
    std::shared_ptr<CBaseTx> pTx;
    int height;
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
        index          = -1;
        fMerkleVerified = false;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(blockHash);
        READWRITE(vMerkleBranch);
        READWRITE(index);
        READWRITE(pTx);
        READWRITE(height);)

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(CBlockIndex *&pIndexRet) const;
    int GetDepthInMainChain() const {
        CBlockIndex *pIndexRet;
        return GetDepthInMainChain(pIndexRet);
    }
    bool IsInMainChain() const {
        CBlockIndex *pIndexRet;
        return GetDepthInMainChainINTERNAL(pIndexRet) > 0;
    }
    int GetBlocksToMaturity() const;

    int SetMerkleBranch(const CBlock *pBlock) {
        AssertLockHeld(cs_main);
        CBlock blockTmp;

        if (pBlock) {
            // Update the tx's blockHash
            blockHash = pBlock->GetHash();

            // Locate the transaction
            for (index = 0; index < (int)pBlock->vptx.size(); index++)
                if ((pBlock->vptx[index])->GetHash() == pTx->GetHash())
                    break;
            if (index == (int)pBlock->vptx.size()) {
                vMerkleBranch.clear();
                index = -1;
                LogPrint("INFO", "ERROR: SetMerkleBranch() : couldn't find tx in block\n");
                return 0;
            }

            // Fill in merkle branch
            vMerkleBranch = pBlock->GetMerkleBranch(index);
        }

        // Is the tx in a block that's in the main chain
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end())
            return 0;
        CBlockIndex *pIndex = (*mi).second;
        if (!pIndex || !chainActive.Contains(pIndex))
            return 0;

        return chainActive.Height() - pIndex->height + 1;
    }
};

#endif  //COIN_MERKLETX_H
