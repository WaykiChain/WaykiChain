// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef CHAIN_CHAIN_H
#define CHAIN_CHAIN_H

#include "persistence/block.h"

class CBlockConfirmMessage ;
class CBlockFinalityMessage ;

/** An in-memory indexed chain of blocks. */
class CChain {
private:
    vector<CBlockIndex *> vChain;
    CBlockIndex* localFinIndex = nullptr ;
    int64_t localFinLastUpdate = 0 ;
    CBlockIndex* globalFinIndex = nullptr ;
    CCriticalSection cs_finblock ;
public:
    /** Returns the index entry for the genesis block of this chain, or nullptr if none. */
    CBlockIndex *Genesis() const;

    CBlockIndex *GetLocalFinIndex();
    CBlockIndex *GetGlobalFinIndex() ;

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



    bool UpdateFinalityBlock() ;
    bool SetLocalFinTimeout() ;
    bool UpdateLocalFinBlock(const CBlockIndex* pIndex);
    bool UpdateLocalFinBlock(const CBlockConfirmMessage& msg);
    bool UpdateGlobalFinBlock(const CBlockIndex* pIndex);
    bool UpdateGlobalFinBlock(const CBlockFinalityMessage& msg);
    int64_t  GetLocalFinLastUpdate() const ;

    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndex *SetTip(CBlockIndex *pIndex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pIndex = nullptr) const;

    /** Find the last common block between this chain and a locator. */
    CBlockIndex *FindFork(map<uint256, CBlockIndex *> &mapBlockIndex, const CBlockLocator &locator) const;

private:
    bool UpdateLocalFinBlock(const uint32_t height);
    bool UpdateGlobalFinBlock(const uint32_t height);
}; //end of CChain


#endif //CHAIN_CHAIN_H