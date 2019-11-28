// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pbftcontext.h"
#include "p2p/protocol.h"

CPBFTContext pbftContext ;

int CPBFTContext::SaveConfirmMessageByBlock(const CBlockConfirmMessage& msg) {

    LOCK(cs_pbftcontext);
    auto it = blockConfirmedMessagesMap.find(msg.blockHash) ;
    if(it == blockConfirmedMessagesMap.end()) {
        set<CBlockConfirmMessage> messages ;
        messages.insert(msg) ;
        blockConfirmedMessagesMap.insert(std::make_pair(msg.blockHash, messages)) ;
        return  1 ;
    } else {
        set<CBlockConfirmMessage> v = blockConfirmedMessagesMap[msg.blockHash] ;
        v.insert(msg);
        blockConfirmedMessagesMap.update(it, v);
        return v.size();
    }

}

bool CPBFTContext::IsKownConfirmMessage(const CBlockConfirmMessage msg){
    return setConfirmMessageKnown.count(msg.GetHash()) != 0  ;
}

bool CPBFTContext::AddConfirmMessageKnown(const CBlockConfirmMessage msg){
    LOCK(cs_pbftcontext);
    setConfirmMessageKnown.insert(msg.GetHash()) ;
    return true;
}

bool CPBFTContext::IsKownFinalityMessage(const CBlockFinalityMessage msg) {
    return setFinalityMessageKnown.count(msg.GetHash()) !=0 ;
}

bool CPBFTContext::AddFinalityMessageKnown(const CBlockFinalityMessage msg) {
    LOCK(cs_pbftcontext);
    setFinalityMessageKnown.insert(msg.GetHash()) ;
    return true;
}

int CPBFTContext::SaveFinalityMessageByBlock(const CBlockFinalityMessage& msg) {

    LOCK(cs_pbftcontext);
    auto it = blockFinalityMessagesMap.find(msg.blockHash) ;
    if(it == blockFinalityMessagesMap.end()) {
        set<CBlockFinalityMessage> messages ;
        messages.insert(msg) ;
        blockFinalityMessagesMap.insert(std::make_pair(msg.blockHash, messages)) ;
        return  1 ;
    } else {
        set<CBlockFinalityMessage> v = blockFinalityMessagesMap[msg.blockHash] ;
        v.insert(msg);
        blockFinalityMessagesMap.update(it, v);
        return v.size();
    }
}

bool CPBFTContext::GetMinerListByBlockHash(const uint256 blockHash, set<CRegID>& miners) {

    auto it = blockMinerListMap.find(blockHash) ;
    if(it == blockMinerListMap.end())
        return false;
    miners = it->second ;
    return true ;
}

bool CPBFTContext::GetMessagesByBlockHash(const uint256 hash, set<CBlockConfirmMessage>& msgs) {
    auto it = blockConfirmedMessagesMap.find(hash) ;
    if(it == blockConfirmedMessagesMap.end())
        return false;
    msgs = it->second ;
    return true ;
}

bool CPBFTContext::GetFinalityMessagesByBlockHash(const uint256 hash, set<CBlockFinalityMessage>& msgs) {
    auto it = blockFinalityMessagesMap.find(hash) ;
    if(it == blockFinalityMessagesMap.end())
        return false;
    msgs = it->second ;
    return true ;
}


bool CPBFTContext::SaveMinersByHash(uint256 blockhash, VoteDelegateVector delegates) {
    set<CRegID> miners ;
    for(auto delegate: delegates){
        miners.insert(delegate.regid);
    }
    blockMinerListMap.insert(std::make_pair(blockhash, miners));
    return true ;
}