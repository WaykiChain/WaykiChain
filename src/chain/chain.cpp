// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "p2p/protocol.h"
#include "miner/pbftcontext.h"
#include "persistence/cachewrapper.h"

extern CPBFTContext pbftContext;
extern CCacheDBManager *pCdMan;
//////////////////////////////////////////////////////////////////////////////
//class CChain implementation

/** Returns the index entry for the genesis block of this chain, or nullptr if none. */
CBlockIndex* CChain::Genesis() const {
    return vChain.size() > 0 ? vChain[0] : nullptr;
}

CBlockIndex* CChain::GetLocalFinIndex(){

    if(!localFinIndex) {
        if (vChain.size() > 0)
            return vChain[0];
        else
            return nullptr;
    }
    return localFinIndex ;
}


CBlockIndex* CChain::GetGlobalFinIndex(){

    if(!globalFinIndex) {
        if (vChain.size() > 0)
            return vChain[0];
        else
            return nullptr;
    }
    return globalFinIndex ;
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

bool CChain::SetLocalFinTimeout(){
    LOCK(cs_finblock);
    localFinIndex = operator[](0) ;
    return true ;
}
bool CChain::UpdateLocalFinBlock(const uint32_t height) {
    {
        LOCK(cs_finblock);
        CBlockIndex* oldFinblock = GetLocalFinIndex();
        if(oldFinblock != nullptr && (uint32_t)oldFinblock->height >= height)
            return false;
        CBlockIndex* pTemp = operator[](height) ;
        if(pTemp== nullptr)
            return false ;

        localFinIndex = pTemp;
        localFinLastUpdate = GetTime();
        return true ;
    }

}

bool CChain::UpdateGlobalFinBlock(const uint32_t height) {
    {
        LOCK(cs_finblock);
        CBlockIndex* oldFinblock = GetGlobalFinIndex();
        CBlockIndex* localFinblock = GetLocalFinIndex() ;
        if(localFinblock== nullptr ||height > (uint32_t)localFinblock->height)
            return false ;
        if(oldFinblock != nullptr && (uint32_t)oldFinblock->height >= height)
            return false;
        CBlockIndex* pTemp = operator[](height) ;
        if(pTemp== nullptr)
            return false ;
        globalFinIndex = pTemp;
        pCdMan->pBlockCache->WriteGlobalFinBlock(pTemp->height, pTemp->GetBlockHash()) ;
        return true ;
    }

}

bool CChain::UpdateLocalFinBlock(const CBlockIndex* pIndex){

    if(pIndex == nullptr|| pIndex->height==0)
        return false ;
    int32_t height = pIndex->height;

    while(height > GetLocalFinIndex()->height&& height>0 &&height > pIndex->height-10){

        CBlockIndex* pTemp = operator[](height) ;

        set<CBlockConfirmMessage> messageSet ;
        set<CRegID> miners;

        if(pbftContext.confirmMessageMan.GetMessagesByBlockHash(pTemp->GetBlockHash(), messageSet)
           && pbftContext.GetMinerListByBlockHash(pTemp->pprev->GetBlockHash(),miners)){

            if(messageSet.size() >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
                int count =0;
                for(auto msg: messageSet){
                    if(miners.count(msg.miner))
                        count++ ;
                    if(count >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
                        return UpdateLocalFinBlock( height) ;
                    }
                }
            }

        }

        height--;

    }
    return false ;
}

bool CChain::UpdateLocalFinBlock(const CBlockConfirmMessage& msg){

    CBlockIndex* fi = GetLocalFinIndex();

    if(fi == nullptr ||(uint32_t)fi->height >= msg.height)
        return false;

    CBlockIndex* pIndex = operator[](msg.height) ;
    if(pIndex == nullptr || pIndex->pprev== nullptr)
        return false;

    if(pIndex->GetBlockHash() != msg.blockHash)
        return false;

    set<CBlockConfirmMessage> messageSet ;
    set<CRegID> miners;

    if(pbftContext.confirmMessageMan.GetMessagesByBlockHash(pIndex->GetBlockHash(), messageSet)
                                   && pbftContext.GetMinerListByBlockHash(pIndex->pprev->GetBlockHash(),miners)) {
        if(messageSet.size() >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
            int count =0;
            for(auto msg: messageSet){
                if(miners.count(msg.miner))
                    count++ ;
                if(count >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
                    return UpdateLocalFinBlock(pIndex->height) ;
                }
            }
        }

    }
    return false;
}

bool CChain::UpdateGlobalFinBlock(const CBlockIndex* pIndex){

    if(pIndex == nullptr|| pIndex->height==0)
        return false ;
    int32_t height = pIndex->height;

    while(height > GetGlobalFinIndex()->height&& height>0 &&height > pIndex->height-50){

        CBlockIndex* pTemp = operator[](height) ;

        set<CBlockFinalityMessage> messageSet ;
        set<CRegID> miners;

        if(pbftContext.finalityMessageMan.GetMessagesByBlockHash(pTemp->GetBlockHash(), messageSet)
           && pbftContext.GetMinerListByBlockHash(pTemp->pprev->GetBlockHash(),miners)){

            if(messageSet.size() >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
                int count =0;
                for(auto msg: messageSet){
                    if(miners.count(msg.miner))
                        count++ ;
                    if(count >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
                        return UpdateGlobalFinBlock( height) ;
                    }
                }
            }

        }

        height--;

    }
    return false ;
}

bool CChain::UpdateGlobalFinBlock(const CBlockFinalityMessage& msg){

    CBlockIndex* fi = GetGlobalFinIndex();

    if(fi == nullptr ||(uint32_t)fi->height >= msg.height)
        return false;

    CBlockIndex* pIndex = operator[](msg.height) ;
    if(pIndex == nullptr || pIndex->pprev== nullptr)
        return false;

    if(pIndex->GetBlockHash() != msg.blockHash)
        return false;

    set<CBlockFinalityMessage> messageSet ;
    set<CRegID> miners;

    if(pbftContext.finalityMessageMan.GetMessagesByBlockHash(pIndex->GetBlockHash(), messageSet)
       && pbftContext.GetMinerListByBlockHash(pIndex->pprev->GetBlockHash(),miners)) {
        if(messageSet.size() >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
            int count =0;
            for(auto msg: messageSet){
                if(miners.count(msg.miner))
                    count++ ;
                if(count >= FINALITY_BLOCK_CONFIRM_MINER_COUNT){
                    return UpdateGlobalFinBlock(pIndex->height) ;
                }
            }
        }

    }
    return false;
}


bool CChain::UpdateFinalityBlock(){
    set<CRegID> minerSet ;
    uint32_t confirmMiners = FINALITY_BLOCK_CONFIRM_MINER_COUNT ;

    if(SysCfg().NetworkID() == MAIN_NET && Height()< 3880000){
        confirmMiners = 0 ;
    }

    if(SysCfg().NetworkID() == TEST_NET && Height() < (int32_t)SysCfg().GetStableCoinGenesisHeight()){
        confirmMiners = 0 ;
    }
    auto pBlockIndex = Tip() ;
    while(pBlockIndex->height > 0){

        if(minerSet.size() >=confirmMiners ){
            if( (localFinIndex && localFinIndex->height< pBlockIndex->height) || !localFinIndex ){
                if(localFinIndex)
                    assert(Contains(localFinIndex));
                localFinIndex = pBlockIndex ;
            }
            return true;
        }
        minerSet.insert(pBlockIndex->miner) ;
        pBlockIndex = pBlockIndex->pprev ;
    }
    if(localFinIndex == nullptr )
        localFinIndex = vChain[0] ;

    return true ;
}

int64_t  CChain::GetLocalFinLastUpdate() const {
    return localFinLastUpdate ;
}