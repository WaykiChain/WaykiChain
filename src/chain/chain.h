// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef CHAIN_CHAIN_H
#define CHAIN_CHAIN_H

#include "persistence/block.h"

/** An in-memory indexed chain of blocks. */
class CChain {
protected:
    vector<CBlockIndex *> vChain;

public:
    /** Returns the index entry for the genesis block of this chain, or nullptr if none. */
    CBlockIndex *Genesis() const;


    /** Returns the index entry for the tip of this chain, or nullptr if none. */
    CBlockIndex *Tip() const;

    /** Returns the index entry at a particular height in this chain, or nullptr if no such height exists. */
    CBlockIndex *operator[](int32_t height) const;

    /** Compare two chains efficiently. */
    friend bool operator==(const CChain &a, const CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.Height()] == b.vChain[b.Height()];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex *pIndex) const;

    /** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pIndex) const;

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->height : -1. */
    int32_t Height() const;


    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndex *SetTip(CBlockIndex *pIndex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pIndex = nullptr) const;

    /** Find the last common block between this chain and a locator. */
    CBlockIndex *FindFork(map<uint256, CBlockIndex *> &mapBlockIndex, const CBlockLocator &locator) const;

}; //end of CChain

class CChainActive : public CChain {
public:
    CBlock tip_block;

    CBlockIndex *SetTip(CBlockIndex *index, const CBlock *block);

    inline CBlock *TipBlock() { return &tip_block; }
};

#endif //CHAIN_CHAIN_H