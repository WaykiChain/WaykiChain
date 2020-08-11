// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pbftmanager.h"
#include "miner/pbftcontext.h"
#include "persistence/cachewrapper.h"
#include "p2p/protocol.h"
#include "miner/miner.h"
#include "wallet/wallet.h"
#include "net.h"
#include "p2p/node.h"

CPBFTMan pbftMan;
extern CPBFTContext pbftContext;
extern CWallet *pWalletMain;
extern CCacheDBManager *pCdMan;

static inline const VoteDelegateVector& GetBpListByHeight(ActiveDelegatesStore &activeDelegatesStore, HeightType height) {
    // make sure the height > tip height - 100
    return (height > activeDelegatesStore.active_delegates.update_height || activeDelegatesStore.last_delegates.IsEmpty())
            ? activeDelegatesStore.active_delegates.delegates
            : activeDelegatesStore.last_delegates.delegates;
}

static inline set<CRegID> GetBpSetByHeight(ActiveDelegatesStore &activeDelegatesStore, HeightType height) {
    auto &bpList = GetBpListByHeight(activeDelegatesStore, height);
    set<CRegID> bpSet;
    for (auto &bp : bpList) {
        bpSet.insert(bp.regid);
    }
    return bpSet;
}

static inline uint32_t GetMinConfirmBpCount(uint32_t bpCount) {
    return bpCount - bpCount/3;
}

uint32_t GetFinalBlockMinerCount(const uint256& blockHash = uint256()) {
    set<CRegID> bpSet;
    if(!blockHash.IsEmpty() && pbftContext.GetMinerListByBlockHash(blockHash, bpSet)) {
        uint32_t bpsize = bpSet.size();
        return bpsize - bpsize/3;

    }
    uint32_t totalBpsSize =  pCdMan->pDelegateCache->GetActivedDelegateNum();
    return totalBpsSize - totalBpsSize/3;

}

void CPBFTMan::InitFinIndex(CBlockIndex *globalFinIndexIn) {
    globalFinIndex = globalFinIndexIn;
    localFinIndex = globalFinIndexIn;
}

CBlockIndex* CPBFTMan::GetLocalFinIndex(){
    LOCK(cs_finblock);
    return localFinIndex ? localFinIndex : chainActive[0];
}


CBlockIndex* CPBFTMan::GetGlobalFinIndex(){
    LOCK(cs_finblock);
    return globalFinIndex ? globalFinIndex : chainActive[0];
}

bool CPBFTMan::SetLocalFinTimeout() {
    LOCK(cs_finblock);
    localFinIndex = chainActive[0];
    return true;
}
bool CPBFTMan::SaveLocalFinBlock(const uint32_t height) {
    {
        LOCK(cs_finblock);
        CBlockIndex* oldFinblock = GetLocalFinIndex();
        if (oldFinblock != nullptr && (uint32_t)oldFinblock->height >= height)
            return false;
        CBlockIndex* pTemp = chainActive[height];
        if (pTemp== nullptr)
            return false;

        localFinIndex = pTemp;
        localFinLastUpdate = GetTime();
        return true;
    }

}

bool CPBFTMan::UpdateGlobalFinBlock(const uint32_t height) {
    {
        LOCK(cs_finblock);
        CBlockIndex* oldGlobalFinblock = GetGlobalFinIndex();
        CBlockIndex* localFinblock = GetLocalFinIndex();

        if(localFinblock== nullptr ||height > (uint32_t)localFinblock->height)
            return false;
        if(oldGlobalFinblock != nullptr && (uint32_t)oldGlobalFinblock->height >= height)
            return false;

        if(oldGlobalFinblock != nullptr ){
            CBlockIndex* chainBlock = chainActive[oldGlobalFinblock->height];
            if(chainBlock != nullptr && chainBlock->GetBlockHash() != oldGlobalFinblock->GetBlockHash()){
                return ERRORMSG("Global finality block changed");
            }
        }

        CBlockIndex* pTemp = chainActive[height];
        if(pTemp== nullptr)
            return false;
        globalFinIndex = pTemp;
        globalFinHash = pTemp->GetBlockHash();
        pCdMan->pBlockCache->SetGlobalFinBlock(pTemp->height, pTemp->GetBlockHash());
        return true;
    }

}

bool CPBFTMan::UpdateLocalFinBlock(CBlockIndex* pTipIndex){

    AssertLockHeld(cs_main);
    assert(pTipIndex == chainActive.Tip() && pTipIndex != nullptr && "tip index invalid");

    if(pTipIndex->height==0){
        return true;
    }

    LOCK(cs_finblock);
    CBlockIndex* oldLocalFinIndex = localFinIndex;
    auto oldLocalFinHeight = oldLocalFinIndex ? oldLocalFinIndex->height : 0;

    ActiveDelegatesStore activeDelegatesStore;
    if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
        return ERRORMSG("get active delegates error");
    }
    CBlockIndex* pIndex = pTipIndex;
    while (pIndex && pIndex->height > oldLocalFinHeight && pIndex->height > 0 &&
           pIndex->height + 10 > pTipIndex->height) {

        set<CBlockConfirmMessage> messageSet;
        if (pbftContext.confirmMessageMan.GetMessagesByBlockHash(pIndex->GetBlockHash(), messageSet)) {

            const set<CRegID> &bpSet = GetBpSetByHeight(activeDelegatesStore, pIndex->height);

            uint32_t minConfirmBpCount = GetMinConfirmBpCount(bpSet.size());
            if(messageSet.size() >= minConfirmBpCount){
                uint32_t count = 0;
                for(auto msg: messageSet){
                    if(bpSet.count(msg.miner))
                        count++;
                    if(count >= minConfirmBpCount){
                        localFinIndex = pIndex;
                        localFinLastUpdate = GetTime();
                        return true;
                    }
                }
            }
        }
        pIndex = pIndex->pprev;
    }

    return false;
}

bool CPBFTMan::UpdateLocalFinBlock(const CBlockConfirmMessage& msg, const uint32_t messageCount){

    ActiveDelegatesStore activeDelegatesStore;
    CBlockIndex* pIndex = nullptr;
    {
        LOCK(cs_main);
        pIndex = chainActive[msg.height];
        if(pIndex == nullptr || pIndex->pprev== nullptr) {
            return false;
        }

        if(pIndex->GetBlockHash() != msg.blockHash) {
            return false;
        }

        if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
            return ERRORMSG("get active delegates error");
        }
    }

    LOCK(cs_finblock);
    CBlockIndex* oldLocalFinIndex = GetLocalFinIndex();

    if (oldLocalFinIndex != nullptr && msg.height <= (uint32_t)oldLocalFinIndex->height ) {
        LogPrint(BCLog::PBFT, "the msg.height=%u is less than current local fin height=%d\n", msg.height, oldLocalFinIndex->height);
        return false;
    }

    set<CBlockConfirmMessage> messageSet;

    if (pbftContext.confirmMessageMan.GetMessagesByBlockHash(pIndex->GetBlockHash(), messageSet)) {

        const set<CRegID> &bpSet = GetBpSetByHeight(activeDelegatesStore, pIndex->height);
        uint32_t minConfirmBpCount = GetMinConfirmBpCount(bpSet.size());
        if (messageSet.size() >= minConfirmBpCount) {
            uint32_t count =0;
            for (auto msg: messageSet){
                if (bpSet.count(msg.miner))
                    count++;

                if (count >= minConfirmBpCount) {
                    return SaveLocalFinBlock(pIndex->height);
                }
            }
        }

    }
    return false;
}

bool CPBFTMan::UpdateGlobalFinBlock(CBlockIndex* pTipIndex){

    AssertLockHeld(cs_main);
    assert(pTipIndex == chainActive.Tip() && pTipIndex != nullptr && "tip index invalid");

    ActiveDelegatesStore activeDelegatesStore;
    if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
        return ERRORMSG("get active delegates error");
    }

    CBlockIndex *pIndex = pTipIndex;
    CBlockIndex *oldGlobalFinIndex = GetGlobalFinIndex();
    HeightType oldGlobalFinHeight = oldGlobalFinIndex ? oldGlobalFinIndex->height : 0;

    while (pIndex && (uint32_t)pIndex->height > oldGlobalFinHeight && pIndex->height > 0 && pIndex->height > pTipIndex->height-50) {

        set<CBlockFinalityMessage> messageSet;

        if (pbftContext.finalityMessageMan.GetMessagesByBlockHash(pIndex->GetBlockHash(), messageSet)) {
            const set<CRegID> &bpSet = GetBpSetByHeight(activeDelegatesStore, pIndex->height);
            uint32_t minConfirmBpCount = GetMinConfirmBpCount(bpSet.size());

            if (messageSet.size() >= minConfirmBpCount){
                uint32_t count =0;
                for (auto msg: messageSet){
                    if (bpSet.count(msg.miner))
                        count++;

                    if (count >= minConfirmBpCount) {
                        return UpdateGlobalFinBlock( pIndex->height);
                    }
                }
            }
        }
        pIndex = pIndex->pprev;
    }

    return false;
}


int64_t  CPBFTMan::GetLocalFinLastUpdate() const {
    return localFinLastUpdate;
}

bool CPBFTMan::UpdateGlobalFinBlock(const CBlockFinalityMessage& msg, const uint32_t messageCount ){

    ActiveDelegatesStore activeDelegatesStore;
    CBlockIndex* pIndex = nullptr;
    {
        LOCK(cs_main);
        pIndex = chainActive[msg.height];
        if(pIndex == nullptr || pIndex->GetBlockHash() != msg.blockHash) {
            LogPrint(BCLog::PBFT, "the block=[%u]%s of finality msg is not actived\n", msg.height, msg.blockHash.ToString());
            return false;
        }

        if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
            return ERRORMSG("get active delegates error");
        }
    }

    uint32_t needConfirmCount = GetFinalBlockMinerCount(msg.preBlockHash);
    if(needConfirmCount > messageCount)
        return false;

    LOCK(cs_finblock);
    CBlockIndex *oldGlobalFinIndex = GetGlobalFinIndex();

    if (oldGlobalFinIndex != nullptr && msg.height <= (uint32_t)oldGlobalFinIndex->height ) {
        LogPrint(BCLog::PBFT, "the msg.height=%u is less than current global fin height=%d\n", msg.height, oldGlobalFinIndex->height);
        return false;
    }

    set<CBlockFinalityMessage> messageSet;
    set<CRegID> miners;

    if (pbftContext.finalityMessageMan.GetMessagesByBlockHash(pIndex->GetBlockHash(), messageSet)) {
        const set<CRegID> &bpSet = GetBpSetByHeight(activeDelegatesStore, pIndex->height);
        uint32_t minConfirmBpCount = GetMinConfirmBpCount(bpSet.size());
        if (messageSet.size() >= minConfirmBpCount){
            uint32_t count = 0;
            for (auto msg: messageSet){
                if (bpSet.count(msg.miner))
                    count++;

                if (count >= minConfirmBpCount) {
                    return UpdateGlobalFinBlock(pIndex->height);
                }
            }
        }
    }

    return false;
}

bool CPBFTMan::AddBlockConfirmMessage(CNode *pFrom, const CBlockConfirmMessage& msg) {
    CPBFTMessageMan<CBlockConfirmMessage>& msgMan = pbftContext.confirmMessageMan;
    if(msgMan.IsKnown(msg)){
        LogPrint(BCLog::NET, "duplicate confirm message! miner_id=%s, blockhash=%s \n",msg.miner.ToString(), msg.blockHash.GetHex());
        return false;
    }

    HeightType localFinHeight = 0;
    {
        LOCK(cs_finblock);
        if (localFinIndex != nullptr)
            localFinHeight = localFinIndex->height;
    }

    if(!CheckPBFTMessage(pFrom, PBFTMsgType::CONFIRM_BLOCK, msg, localFinHeight)){
        LogPrint(BCLog::NET, "confirm message check failed,miner_id=%s, blockhash=%s \n",msg.miner.ToString(), msg.blockHash.GetHex());
        return false;
    }

    msgMan.AddMessageKnown(msg);
    int messageCount = msgMan.SaveMessageByBlock(msg.blockHash, msg);

    bool updateFinalitySuccess = UpdateLocalFinBlock(msg,  messageCount);

    RelayBlockConfirmMessage(msg);

    if(updateFinalitySuccess){
        BroadcastBlockFinality(GetLocalFinIndex());
    }
    return true;
}

bool CPBFTMan::AddBlockFinalityMessage(CNode *pFrom, const CBlockFinalityMessage& msg) {
    CPBFTMessageMan<CBlockFinalityMessage>& msgMan = pbftContext.finalityMessageMan;
    if(msgMan.IsKnown(msg)){
        LogPrint(BCLog::NET, "duplicated finality message, miner=%s, block=%s \n",
                 msg.miner.ToString(), msg.GetBlockId());
        return false;
    }

    HeightType globalFinHeight = 0;
    {
        LOCK(cs_finblock);
        if (globalFinIndex != nullptr)
            globalFinHeight = globalFinIndex->height;
    }

    if(!CheckPBFTMessage(pFrom, PBFTMsgType::FINALITY_BLOCK, msg, globalFinHeight)){
        LogPrint(BCLog::NET, "finality block message check failed, miner=%s, block=%s \n",
                 msg.miner.ToString(), msg.GetBlockId());
        return false;
    }

    msgMan.AddMessageKnown(msg);
    int messageCount = msgMan.SaveMessageByBlock(msg.blockHash, msg);
    pbftMan.UpdateGlobalFinBlock(msg, messageCount);

    RelayBlockFinalityMessage(msg);
    return true;
}

static bool PbftFindMiner(CRegID delegate, Miner &miner){
    {
        LOCK(cs_main);
        if (!pCdMan->pAccountCache->GetAccount(delegate, miner.account)) {
            LogPrint(BCLog::MINER, "PbftFindMiner() : fail to get miner account! regid=%s\n",
                     miner.delegate.regid.ToString());
            return false;
        }
    }

    {
        LOCK(pWalletMain->cs_wallet);
        if (miner.account.miner_pubkey.IsValid() && pWalletMain->GetKey(miner.account.keyid, miner.key, true)) {
            return true;
        } else if (!pWalletMain->GetKey(miner.account.keyid, miner.key)) {
            return false;
        }
    }

    return true;
}

bool BroadcastBlockFinality(const CBlockIndex* block){


    if(!SysCfg().GetBoolArg("-genblock", false))
        return false;

    if(IsInitialBlockDownload())
        return false;

    CPBFTMessageMan<CBlockFinalityMessage>& msgMan = pbftContext.finalityMessageMan;

    if(msgMan.IsBroadcastedBlock(block->GetBlockHash()))
        return true;

    //查找上一个区块执行过后的矿工列表
    set<CRegID> delegates;

    if(block->pprev == nullptr)
        return false;
    pbftContext.GetMinerListByBlockHash(block->pprev->GetBlockHash(), delegates);

    uint256 preHash = block->pprev == nullptr? uint256(): block->pprev->GetBlockHash();


    CBlockFinalityMessage msg(block->height, block->GetBlockHash(), preHash);

    {

        for(auto delegate: delegates){

            Miner miner;
            if(!PbftFindMiner(delegate, miner))
                continue;
            msg.miner = miner.account.regid;
            vector<unsigned char > vSign;
            uint256 messageHash = msg.GetHash();

            miner.key.Sign(messageHash, vSign);
            msg.SetSignature(vSign);

            {
                LOCK(cs_vNodes);
                for (auto pNode : vNodes) {
                    pNode->PushBlockFinalityMessage(msg);
                }
            }

            msgMan.SaveMessageByBlock(msg.blockHash, msg);

        }
    }

    msgMan.SaveBroadcastedBlock(block->GetBlockHash());
    return true;

}
bool BroadcastBlockConfirm(const CBlockIndex* pTipIndex) {

    AssertLockHeld(cs_main);

    if(!SysCfg().GetBoolArg("-genblock", false))
        return false;


    if(GetTime() - pTipIndex->GetBlockTime() > 60)
        return false;

    if(IsInitialBlockDownload())
        return false;

    CPBFTMessageMan<CBlockConfirmMessage>& msgMan = pbftContext.confirmMessageMan;

    if(msgMan.IsBroadcastedBlock(pTipIndex->GetBlockHash())){
        return true;
    }

    VoteDelegateVector activeDelegates;
    if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegates)) {
        return ERRORMSG("get active delegates error");
    }

    if(pTipIndex->pprev == nullptr) {
        return false;
    }

    CBlockConfirmMessage msg(pTipIndex->height, pTipIndex->GetBlockHash(), pTipIndex->pprev->GetBlockHash());
    {
        for(auto delegate: activeDelegates){
            Miner miner;
            if(!PbftFindMiner(delegate.regid, miner))
                continue;
            msg.miner = miner.account.regid;
            vector<unsigned char > vSign;
            uint256 messageHash = msg.GetHash();

            miner.key.Sign(messageHash, vSign);
            msg.SetSignature(vSign);

            {
                LOCK(cs_vNodes);
                for (auto pNode : vNodes) {
                    pNode->PushBlockConfirmMessage(msg);
                }
            }
            LogPrint(BCLog::PBFT, "generate and broadcast pbft confirm msg! block=%s, bp=%s\n",
                pTipIndex->GetIdString(), delegate.regid.ToString());
            msgMan.SaveMessageByBlock(msg.blockHash,msg);
        }
    }

    msgMan.SaveBroadcastedBlock(pTipIndex->GetBlockHash());
    return true;
}

bool CheckPBFTMessage(CNode *pFrom, const int32_t msgType ,const CPBFTMessage& msg, HeightType finHeight) {

    //check message type;
    if(msg.msgType != msgType ) {
        LogPrint(BCLog::INFO, "Misbehaving: invalid pbft_msg_type=%d, expected=%d, Misbehavior add 100\n", msg.msgType, msgType);
        Misbehaving(pFrom->GetId(), 100);
        return false;
    }

    CAccount account;
    {
        LOCK(cs_main);

        //check height
        uint32_t tipHeight = chainActive.Height();
        finHeight = min(tipHeight, finHeight); // make sure the finHeight <= tipHeight
        if( msg.height < finHeight || msg.height > tipHeight + PBFT_LATEST_BLOCK_COUNT ) {
            LogPrint(BCLog::PBFT, "messages height is out of valid range[finHeight, tipHeight]:[%u, %u]\n",
                finHeight, chainActive.Height() + PBFT_LATEST_BLOCK_COUNT);
            return false; // ignore the msg
        }

        // check whether in chainActive
        if (msg.height > tipHeight) {
            CBlockIndex* pIndex = chainActive[msg.height];
            if(pIndex == nullptr || pIndex->GetBlockHash() != msg.blockHash) {
                LogPrint(BCLog::PBFT, "msg_block=%s not in chainActive! miner=%s\n",
                    msg.GetBlockId(), msg.miner.ToString());
                return false; // ignore the msg
            }
        }

        ActiveDelegatesStore activeDelegatesStore;
        if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
            return ERRORMSG("get active delegates error");
        }

        const auto &bpList = GetBpListByHeight(activeDelegatesStore, msg.height);
        if (std::find(bpList.begin(), bpList.end(), msg.miner) == bpList.end()) {
            LogPrint(BCLog::INFO, "the miner=%s of msg is not in bp list! Misbehavior add %d, msg_block=%s\n",
                msg.miner.ToString(), 2, msg.GetBlockId());
            Misbehaving(pFrom->GetId(), 2);
        }

        //check signature
        if(!pCdMan->pAccountCache->GetAccount(msg.miner, account)) {
            LogPrint(BCLog::INFO, "the miner=%s of msg is not found! Misbehavior add %d, msg_block=%s\n",
                msg.miner.ToString(), 10, msg.GetBlockId());
            Misbehaving(pFrom->GetId(), 10);
            return false;
        }
    }
    uint256 messageHash = msg.GetHash();
    // TODO: add verify signature cache for pBFT messages
    if (!VerifySignature(messageHash, msg.vSignature, account.owner_pubkey)) {
        if (!VerifySignature(messageHash, msg.vSignature, account.miner_pubkey)) {
            LogPrint(BCLog::INFO, "verify signature error! Misbehavior add %d, miner=%s, msg_block=%s\n",
                10, msg.miner.ToString(), msg.GetBlockId());
            Misbehaving(pFrom->GetId(), 10);
            return false;
        }
    }

    return true;

}

bool RelayBlockConfirmMessage(const CBlockConfirmMessage& msg){

    LOCK(cs_vNodes);
    for(auto node:vNodes){
        node->PushBlockConfirmMessage(msg);
    }
    return true;
}

bool RelayBlockFinalityMessage(const CBlockFinalityMessage& msg){

    LOCK(cs_vNodes);
    for(auto node:vNodes){
        node->PushBlockFinalityMessage(msg);
    }
    return true;
}
