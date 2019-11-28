// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef MINER_PBFTCONTEXT_H
#define MINER_PBFTCONTEXT_H

#include <map>
#include <set>
#include "sync.h"
#include "commons/uint256.h"
#include "commons/limitedmap.h"
#include "commons/mruset.h"
#include "entities/vote.h"

class CRegID ;
class CBlockConfirmMessage ;
class CBlockFinalityMessage;


class CPBFTContext {

private:
    CCriticalSection cs_pbftcontext ;

public:

    limitedmap<uint256, set<CBlockConfirmMessage>> blockConfirmedMessagesMap ;
    limitedmap<uint256, set<CBlockFinalityMessage>> blockFinalityMessagesMap ;
    limitedmap<uint256, set<CRegID>> blockMinerListMap ;
    mruset<uint256> confirmedBlockHashSet ;
    mruset<uint256> finalityBlockHashSet ;
    mruset<uint256> setConfirmMessageKnown ;
    mruset<uint256> setFinalityMessageKnown ;

    CPBFTContext(){
        confirmedBlockHashSet.max_size(500);
        setConfirmMessageKnown.max_size(500);
        blockMinerListMap.max_size(500) ;
        blockConfirmedMessagesMap.max_size(500) ;
        blockFinalityMessagesMap.max_size(500) ;
    }

    bool IsKownConfirmMessage(const CBlockConfirmMessage msg);

    bool AddConfirmMessageKnown(const CBlockConfirmMessage msg);

    int SaveConfirmMessageByBlock(const CBlockConfirmMessage& msg) ;

    bool IsKownFinalityMessage(const CBlockFinalityMessage msg);

    bool AddFinalityMessageKnown(const CBlockFinalityMessage msg);

    int SaveFinalityMessageByBlock(const CBlockFinalityMessage& msg) ;

    bool GetMinerListByBlockHash(const uint256 blockHash, set<CRegID>& delegates) ;

    bool GetMessagesByBlockHash(const uint256 hash, set<CBlockConfirmMessage>& msgs) ;

    bool SaveMinersByHash(uint256 blockhash, VoteDelegateVector delegates) ;

    bool GetFinalityMessagesByBlockHash(const uint256 hash, set<CBlockFinalityMessage>& msgs);

};


#endif //MINER_PBFTCONTEXT_H
