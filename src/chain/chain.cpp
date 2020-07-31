// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"

//////////////////////////////////////////////////////////////////////////////
//class CChain implementation

/** Returns the index entry for the genesis block of this chain, or nullptr if none. */
CBlockIndex* CChain::Genesis() const {
    return vChain.size() > 0 ? vChain[0] : nullptr;
}


/** Returns the index entry for the tip of this chain, or nullptr if none. */
CBlockIndex* CChain::Tip() const {
    return vChain.size() > 0 ? vChain[Height()] : nullptr;
}

/** Returns the index entry at a particular height in this chain, or nullptr if no such height exists. */
CBlockIndex* CChain::operator[](int32_t height) const {
    if (height < 0 || height >= (int)vChain.size())
        return nullptr;
    return vChain[height];
}

/** Efficiently check whether a block is present in this chain. */
bool CChain::Contains(const CBlockIndex *pIndex) const {
    return (*this)[pIndex->height] == pIndex;
}

/** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
CBlockIndex* CChain::Next(const CBlockIndex *pIndex) const {
    if (Contains(pIndex))
        return (*this)[pIndex->height + 1];
    else
        return nullptr;
}

/** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->height : -1. */
int32_t CChain::Height() const {
    return vChain.size() - 1;
}

CBlockIndex *CChain::SetTip(CBlockIndex *pIndex) {
    if (pIndex == nullptr) {
        vChain.clear();
        return nullptr;
    }
    vChain.resize(pIndex->height + 1);
    while (pIndex && vChain[pIndex->height] != pIndex) {
        vChain[pIndex->height] = pIndex;
        pIndex                 = pIndex->pprev;
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

CBlockIndex* CChain::FindFork(map<uint256, CBlockIndex *> &mapBlockIndex, const CBlockLocator &locator) const {
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

////////////////////////////////////////////////////////////////////////////////
// class CChainActive
CBlockIndex* CChainActive::SetTip(CBlockIndex *index, const CBlock *block) {
    auto ret = CChain::SetTip(index);
    if (ret != nullptr) {
        tip_block = *block;
    } else {
        tip_block = CBlock();
    }
    return ret;
}