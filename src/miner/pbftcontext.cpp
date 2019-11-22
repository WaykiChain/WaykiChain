// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pbftcontext.h"
#include "p2p/protocol.h"

CPBFTContext pbftContext ;

bool CPBFTContext::SaveConfirmMessageByBlock(const CBlockConfirmMessage& msg) {

    LOCK(cs_pbftcontext);
    auto it = blockConfirmedMessagesMap.find(msg.blockHash) ;
    if(it == blockConfirmedMessagesMap.end()) {
        set<CBlockConfirmMessage> messages ;
        messages.insert(msg) ;
        blockConfirmedMessagesMap.insert(std::make_pair(msg.blockHash, messages)) ;
    } else {
        set<CBlockConfirmMessage> v = blockConfirmedMessagesMap[msg.blockHash] ;
        v.insert(msg);
        blockConfirmedMessagesMap.update(it, v );
        /*if(v.size()==8){
            LogPrint(BCLog::NET, "received 8 confirmMessages, blockHash(%s)", msg.blockHash.GetHex()) ;
            return true ;
        }*/
    }
    return true ;
}

bool CPBFTContext::IsKownConfirmMessage(const CBlockConfirmMessage msg){
    return setConfirmMessageKnown.count(msg.GetHash()) != 0  ;
}

bool CPBFTContext::AddConfirmMessageKnown(const CBlockConfirmMessage msg){
    setConfirmMessageKnown.insert(msg.GetHash()) ;
    return true;
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

bool CPBFTContext::SaveMinersByHash(uint256 blockhash, VoteDelegateVector delegates) {
    set<CRegID> miners ;
    for(auto delegate: delegates){
        miners.insert(delegate.regid);
    }
    blockMinerListMap.insert(std::make_pair(blockhash, miners));
    return true ;
}