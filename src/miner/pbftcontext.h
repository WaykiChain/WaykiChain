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
#include "miner/pbftlimitmap.h"
#include "commons/mruset.h"
#include "entities/vote.h"

class CRegID;
class CBlockConfirmMessage;
class CBlockFinalityMessage;

template <typename MsgType>
class CPBFTMessageMan {

private:
    CCriticalSection cs_pbftmessage;
    CPbftLimitmap<uint256, set<MsgType>> blockMessagesMap;
    mruset<uint256> broadcastedBlockHashSet;
    mruset<MsgType> messageKnown;

public:
    CPBFTMessageMan(){
            blockMessagesMap.max_size(500);
            broadcastedBlockHashSet.max_size(500);
            messageKnown.max_size(500);
    }

    CPBFTMessageMan(const int maxSize) {
        blockMessagesMap.max_size(maxSize);
        broadcastedBlockHashSet.max_size(maxSize);
        messageKnown.max_size(maxSize);
    }

    bool IsBroadcastedBlock(uint256 blockHash) {
        return broadcastedBlockHashSet.count(blockHash) > 0;
    }

    bool SaveBroadcastedBlock(uint256 blockHash) {
        broadcastedBlockHashSet.insert(blockHash);
        return true;
    }

    bool IsKnown(const MsgType msg) {
        return messageKnown.count(msg) != 0;
    }

    bool AddMessageKnown(const MsgType msg) {
            LOCK(cs_pbftmessage);
            messageKnown.insert(msg);
            return true;
    }

    int  SaveMessageByBlock(const uint256 blockHash,const MsgType& msg) {

            LOCK(cs_pbftmessage);
            auto it = blockMessagesMap.find(blockHash);
            if (it == blockMessagesMap.end()) {
                    set<MsgType> messages;
                    messages.insert(msg);
                    blockMessagesMap.insert(std::make_pair(blockHash, messages));
                    return 1;
            } else {
                    set<MsgType> v = blockMessagesMap[blockHash];
                    v.insert(msg);
                    blockMessagesMap.update(it, v);
                    return v.size();
            }
    }

    bool GetMessagesByBlockHash(const uint256 hash, set<MsgType>& msgs) {
            auto it = blockMessagesMap.find(hash);
            if (it == blockMessagesMap.end())
                return false;
            msgs = it->second;
            return true;
    }

};

class CPBFTContext {
public:
    CPBFTMessageMan<CBlockConfirmMessage> confirmMessageMan;
    CPBFTMessageMan<CBlockFinalityMessage> finalityMessageMan;
    CPbftLimitmap<uint256, set<CRegID>> blockMinerListMap;

    CPBFTContext(){
        blockMinerListMap.max_size(500);
    }

    bool GetMinerListByBlockHash(const uint256 blockHash, set<CRegID>& delegates);

    bool SaveMinersByHash(uint256 blockhash, VoteDelegateVector delegates);
};

#endif //MINER_PBFTCONTEXT_H
