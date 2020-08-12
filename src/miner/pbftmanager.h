// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef MINER_PBFTMANAGER_H
#define MINER_PBFTMANAGER_H

#include "chain/chain.h"
#include "miner/pbftcontext.h"

class CBlockConfirmMessage;
class CBlockFinalityMessage;
class CPBFTMessage;

class CPBFTMan {

private:
    CBlockIndex* local_fin_index = nullptr;
    int64_t local_fin_last_update = 0;
    CBlockIndex* global_fin_index = nullptr;
    CPBFTMessageMan<CBlockConfirmMessage> confirmMessageMan;
    CPBFTMessageMan<CBlockFinalityMessage> finalityMessageMan;
    CCriticalSection cs_finblock;
    bool UpdateGlobalFinBlock(const uint32_t height);

public:
    void InitFinIndex(CBlockIndex *globalFinIndexIn);

    CBlockIndex *GetLocalFinIndex();
    CBlockIndex *GetGlobalFinIndex();
    bool SetLocalFinTimeout();
    bool UpdateLocalFinBlock(CBlockIndex* pTipIndex);
    bool UpdateLocalFinBlock(const CBlockConfirmMessage& msg, const uint32_t messageCount);
    bool UpdateGlobalFinBlock(CBlockIndex* pIndex);
    bool UpdateGlobalFinBlock(const CBlockFinalityMessage& msg, const uint32_t messageCount);
    int64_t  GetLocalFinLastUpdate() const;
    bool AddBlockConfirmMessage(CNode *pFrom, const CBlockConfirmMessage& msg);
    bool AddBlockFinalityMessage(CNode *pFrom, const CBlockFinalityMessage& msg);

    bool BroadcastBlockConfirm(const CBlockIndex* pTipIndex);
    bool BroadcastBlockFinality(const CBlockIndex* pTipIndex);
    bool CheckPBFTMessage(CNode *pFrom, const int32_t msgType ,const CPBFTMessage& msg);
};

bool RelayBlockConfirmMessage(const CBlockConfirmMessage& msg);

bool RelayBlockFinalityMessage(const CBlockFinalityMessage& msg);
#endif //MINER_PBFTMANAGER_H
