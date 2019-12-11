//
// Created by yehuan on 2019-12-10.
//

#ifndef MINER_PBFTMANAGER_H
#define MINER_PBFTMANAGER_H

#include "chain/chain.h"

class CBlockConfirmMessage ;
class CBlockFinalityMessage ;
class CPBFTMessage ;

class CPBFTMan {

private:
    CBlockIndex* localFinIndex = nullptr ;
    int64_t localFinLastUpdate = 0 ;
    CBlockIndex* globalFinIndex = nullptr ;
    CCriticalSection cs_finblock ;
    bool UpdateLocalFinBlock(const uint32_t height);
    bool UpdateGlobalFinBlock(const uint32_t height);

public:

    CBlockIndex *GetLocalFinIndex();
    CBlockIndex *GetGlobalFinIndex() ;
//    bool UpdateFinalityBlock() ;
    bool SetLocalFinTimeout() ;
    bool UpdateLocalFinBlock(const CBlockIndex* pIndex);
    bool UpdateLocalFinBlock(const CBlockConfirmMessage& msg);
    bool UpdateGlobalFinBlock(const CBlockIndex* pIndex);
    bool UpdateGlobalFinBlock(const CBlockFinalityMessage& msg);
    int64_t  GetLocalFinLastUpdate() const ;
};

bool BroadcastBlockConfirm(const CBlockIndex* block) ;

bool BroadcastBlockFinality(const CBlockIndex* block) ;

bool CheckPBFTMessage(const int32_t msgType ,const CPBFTMessage& msg) ;

bool CheckPBFTMessageSignaturer(const CPBFTMessage& msg) ;
bool RelayBlockConfirmMessage(const CBlockConfirmMessage& msg) ;

bool RelayBlockFinalityMessage(const CBlockFinalityMessage& msg) ;
#endif //MINER_PBFTMANAGER_H
