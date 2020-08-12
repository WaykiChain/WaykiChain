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
#include "commons/lrucache.hpp"

class CRegID;
class CBlockConfirmMessage;
class CBlockFinalityMessage;

template <typename MsgType>
class CPBFTMessageMan {
public:
    typedef map<CRegID, MsgType> BpMsgMap;
private:
    CCriticalSection cs_pbftmessage;
    CLruCache<uint256, BpMsgMap, CUint256Hasher> blockMessagesMap;
    mruset<uint256> broadcastedBlockHashSet;
    mruset<MsgType> messageKnown;

public:
    CPBFTMessageMan()
        : blockMessagesMap(PBFT_LATEST_BLOCK_COUNT), broadcastedBlockHashSet(PBFT_LATEST_BLOCK_COUNT),
          messageKnown(PBFT_LATEST_BLOCK_COUNT) {}

    bool IsBroadcastedBlock(uint256 blockHash) {
        LOCK(cs_pbftmessage);
        return broadcastedBlockHashSet.count(blockHash) > 0;
    }

    bool SaveBroadcastedBlock(uint256 blockHash) {
        LOCK(cs_pbftmessage);
        broadcastedBlockHashSet.insert(blockHash);
        return true;
    }

    bool IsKnown(const MsgType msg) {
        LOCK(cs_pbftmessage);
        return messageKnown.count(msg) != 0;
    }

    bool AddMessageKnown(const MsgType msg) {
        LOCK(cs_pbftmessage);
        messageKnown.insert(msg);
        return true;
    }

    int  SaveMessageByBlock(const uint256 &blockHash, const MsgType& msg) {

        LOCK(cs_pbftmessage);
        auto pData = blockMessagesMap.Touch(blockHash);
        assert(pData && "Touch() must return a valid pData");
        auto ret = pData->emplace(msg.miner, msg);
        if (!ret.second) {
            LogPrint(BCLog::PBFT, "duplicated msg of bp=%s, block=%s, type=%d\n",
                msg.miner.ToString(), msg.GetBlockId(), msg.msgType);
        }
        return pData->size();
    }

    bool GetMessagesByBlockHash(const uint256 &hash, BpMsgMap& bpMsgMap) {
        LOCK(cs_pbftmessage);
        auto pData = blockMessagesMap.Get(hash);
        if (pData == nullptr)
            return false;
        bpMsgMap = *pData;
        return true;
    }

};

class CPBFTContext {
public:
    CPBFTMessageMan<CBlockConfirmMessage> confirmMessageMan;
    CPBFTMessageMan<CBlockFinalityMessage> finalityMessageMan;
};

#endif //MINER_PBFTCONTEXT_H
