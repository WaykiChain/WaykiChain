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



class CPBFTContext {

private:
    CCriticalSection cs_pbftcontext ;
public:

    limitedmap<uint256, set<CBlockConfirmMessage>> blockConfirmedMessagesMap ;
    limitedmap<uint256, set<CRegID>> blockMinerListMap ;
    mruset<uint256> confirmedBlockHashSet ;
    mruset<uint256> setConfirmMessageKnown ;
    CPBFTContext(){
        confirmedBlockHashSet.max_size(500);
        setConfirmMessageKnown.max_size(500);
    }

    bool IsKownConfirmMessage(const CBlockConfirmMessage msg);

    bool AddConfirmMessageKnown(const CBlockConfirmMessage msg);

    bool SaveConfirmMessageByBlock(const CBlockConfirmMessage& msg) ;

    bool GetMinerListByBlockHash(const uint256 blockHash, set<CRegID>& delegates) ;

    bool GetMessagesByBlockHash(const uint256 hash, set<CBlockConfirmMessage>& msgs) ;

    bool SaveMinersByHash(uint256 blockhash, VoteDelegateVector delegates) ;
};


#endif //MINER_PBFTCONTEXT_H
