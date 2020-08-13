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
    CBlockIndex* global_fin_index = nullptr;
    CPBFTMessageMan<CBlockConfirmMessage> confirmMessageMan;
    CPBFTMessageMan<CBlockFinalityMessage> finalityMessageMan;
    CCriticalSection cs_finblock;
    bool SaveGlobalFinBlock(CBlockIndex *pNewIndex);
    CBlockIndex* GetNewLocalFinIndex(const CBlockConfirmMessage& msg);
    CBlockIndex* GetNewGlobalFinIndex(const CBlockFinalityMessage& msg);
    bool CheckPBFTMessage(CNode *pFrom, const int32_t msgType ,const CPBFTMessage& msg);
public:
    void InitFinIndex(CBlockIndex *globalFinIndex);
    void ClearFinIndex();

    CBlockIndex *GetLocalFinIndex();
    CBlockIndex *GetGlobalFinIndex();
    bool UpdateLocalFinBlock(CBlockIndex* pTipIndex);
    bool UpdateGlobalFinBlock(CBlockIndex* pIndex);
    bool ProcessBlockConfirmMessage(CNode *pFrom, const CBlockConfirmMessage& msg);
    bool ProcessBlockFinalityMessage(CNode *pFrom, const CBlockFinalityMessage& msg);

    bool BroadcastBlockConfirm(const CBlockIndex* pTipIndex);
    bool BroadcastBlockFinality(const CBlockIndex* pTipIndex);

    bool IsBlockReversible(HeightType height, const uint256 &hash);
    bool IsBlockReversible(const CBlock &block);
    bool IsBlockReversible(CBlockIndex *pIndex);

    void AfterAcceptBlock(CBlockIndex* pTipIndex);
    void AfterDisconnectTip(CBlockIndex* pTipIndex);
};

bool RelayBlockConfirmMessage(const CBlockConfirmMessage& msg);

bool RelayBlockFinalityMessage(const CBlockFinalityMessage& msg);

#endif //MINER_PBFTMANAGER_H
