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
    CCriticalSection cs_pbftmessage;
private:
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

     const BpMsgMap *InsertMessage(const MsgType& msg) {
        AssertLockHeld(cs_pbftmessage);
        messageKnown.insert(msg);
        auto pBpMsgMap = blockMessagesMap.Touch(msg.blockHash);
        assert(pBpMsgMap && "Touch() must return a valid pBpMsgMap");
        auto ret = pBpMsgMap->emplace(msg.miner, msg);
        if (!ret.second) {
            LogPrint(BCLog::PBFT, "duplicated msg of bp=%s, block=%s, type=%d\n",
                msg.miner.ToString(), msg.GetBlockId(), msg.msgType);
        }
        return pData->size();
    }

    static inline uint32_t GetMinConfirmBpCount(uint32_t bpCount) {
        return bpCount - bpCount/3;
    }

    bool CheckBlockConfirm(const BpMsgMap *pBpMsgMap, const VoteDelegateVector& delegates) {

        uint32_t minConfirmBpCount = GetMinConfirmBpCount(delegates.size());
        if(pBpMsgMap->size() < minConfirmBpCount)
            return false;

        uint32_t count = 0;
        for(auto &bp : delegates){
            if(pBpMsgMap->count(bp.regid))
                count++;
            if(count >= minConfirmBpCount){
                return true;
            }
        }
        return false;
    }



};

#endif //MINER_PBFTCONTEXT_H
