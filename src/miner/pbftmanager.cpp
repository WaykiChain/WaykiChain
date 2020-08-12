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
extern CWallet *pWalletMain;
extern CCacheDBManager *pCdMan;

static inline const VoteDelegateVector& GetBpListByHeight(ActiveDelegatesStore &activeDelegatesStore, HeightType height) {
    // make sure the height > tip height - 100
    return (height > activeDelegatesStore.active_delegates.update_height || activeDelegatesStore.last_delegates.IsEmpty())
            ? activeDelegatesStore.active_delegates.delegates
            : activeDelegatesStore.last_delegates.delegates;
}

void CPBFTMan::InitFinIndex(CBlockIndex *globalFinIndexIn) {
    global_fin_index = globalFinIndexIn;
    local_fin_index = globalFinIndexIn;
}

CBlockIndex* CPBFTMan::GetLocalFinIndex(){
    LOCK(cs_finblock);
    return local_fin_index ? local_fin_index : chainActive[0];
}


CBlockIndex* CPBFTMan::GetGlobalFinIndex(){
    LOCK(cs_finblock);
    return global_fin_index ? global_fin_index : chainActive[0];
}

bool CPBFTMan::SetLocalFinTimeout() {
    LOCK(cs_finblock);
    local_fin_index = chainActive[0];
    return true;
}

bool CPBFTMan::SaveGlobalFinBlock(CBlockIndex *pNewIndex) {

    AssertLockHeld(cs_main);
    AssertLockHeld(cs_finblock);
    if (!pCdMan->pBlockCache->SetGlobalFinBlock(pNewIndex->height, pNewIndex->GetBlockHash())) {
        return ERRORMSG("save global fin block to db error");
    }

    global_fin_index = pNewIndex;
    return true;
}

bool CPBFTMan::UpdateLocalFinBlock(CBlockIndex* pTipIndex){

    AssertLockHeld(cs_main);
    assert(pTipIndex == chainActive.Tip() && pTipIndex != nullptr && "tip index invalid");

    if(pTipIndex->height==0){
        return true;
    }

    ActiveDelegatesStore activeDelegatesStore;
    if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
        return ERRORMSG("get active delegates error");
    }

    LOCK(cs_finblock);
    uint32_t localFinHeight = local_fin_index ? local_fin_index->height : 0;
    uint32_t minHeight = pTipIndex->height > 50 ? pTipIndex->height - 50 : 0;
    minHeight = std::max(minHeight, localFinHeight);

    CBlockIndex* pIndex = pTipIndex;
    while (pIndex && (uint32_t)pIndex->height > minHeight) {

        const auto &bpList = GetBpListByHeight(activeDelegatesStore, pIndex->height);
        if (confirmMessageMan.CheckConfirmByBlock(pIndex->GetBlockHash(), bpList)) {
            local_fin_index = pIndex;
            local_fin_last_update = GetTime();
            return true;
        }
        pIndex = pIndex->pprev;
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

    LOCK(cs_finblock);
    HeightType globalFinHeight = global_fin_index ? global_fin_index->height : 0;

    // make sure the local_fin_height <= global_fin_height
    if (local_fin_index && chainActive.Contains(local_fin_index)) {
        pIndex = local_fin_index;
    }

    HeightType minHeight = pTipIndex->height > 50 ? pTipIndex->height - 50 : 0;
    minHeight = max(globalFinHeight, minHeight);

    while (pIndex && (uint32_t)pIndex->height > minHeight) {
        const auto &bpList = GetBpListByHeight(activeDelegatesStore, pIndex->height);
        if (finalityMessageMan.CheckConfirmByBlock(pIndex->GetBlockHash(), bpList)) {
            return SaveGlobalFinBlock(pIndex);
        }

        pIndex = pIndex->pprev;
    }

    return false;
}

int64_t  CPBFTMan::GetLocalFinLastUpdate() const {
    return local_fin_last_update;
}

CBlockIndex* CPBFTMan::GetNewLocalFinIndex(const CBlockConfirmMessage& msg) {

    AssertLockHeld(cs_main);
    AssertLockHeld(cs_finblock);
    CBlockIndex* pIndex = chainActive[msg.height];
    if(pIndex == nullptr || pIndex->GetBlockHash() != msg.blockHash) {
        LogPrint(BCLog::PBFT, "the block=[%u]%s of confirm msg is not active\n", msg.height, msg.blockHash.ToString());
        return nullptr;
    }
    uint32_t localFinHeight = local_fin_index ? local_fin_index->height : 0;
    if (msg.height <= localFinHeight ) {
        LogPrint(BCLog::PBFT, "the msg.height=%u is <= local_fin_height=%d\n", msg.height, localFinHeight);
        return nullptr;
    }
    return pIndex;
}

CBlockIndex* CPBFTMan::GetNewGlobalFinIndex(const CBlockFinalityMessage& msg) {

    AssertLockHeld(cs_main);
    AssertLockHeld(cs_finblock);
    CBlockIndex* pIndex = chainActive[msg.height];
    if(pIndex == nullptr || pIndex->GetBlockHash() != msg.blockHash) {
        LogPrint(BCLog::PBFT, "the block=[%u]%s of finality msg is not active\n", msg.height, msg.blockHash.ToString());
        return nullptr;
    }
    uint32_t globalFinHeight = global_fin_index ? global_fin_index->height : 0;
    if (msg.height <= globalFinHeight ) {
        LogPrint(BCLog::PBFT, "msg.height=%u <= global_fin_height=%d\n", msg.height, globalFinHeight);
        return nullptr;
    }
    uint32_t localFinHeight = local_fin_index ? local_fin_index->height : 0;
    if (msg.height > localFinHeight ) {
        LogPrint(BCLog::PBFT, "new_global_fin_height=%u > local_fin_height=%d\n", msg.height, localFinHeight);
        return nullptr;
    }
    return pIndex;
}

bool CPBFTMan::ProcessBlockConfirmMessage(CNode *pFrom, const CBlockConfirmMessage& msg) {

    if(confirmMessageMan.IsKnown(msg)){
        LogPrint(BCLog::NET, "duplicate confirm message! miner_id=%s, blockhash=%s \n",msg.miner.ToString(), msg.blockHash.GetHex());
        return false;
    }

    if(!CheckPBFTMessage(pFrom, PBFTMsgType::CONFIRM_BLOCK, msg)){
        LogPrint(BCLog::NET, "confirm message check failed,miner_id=%s, blockhash=%s \n",msg.miner.ToString(), msg.blockHash.GetHex());
        return false;
    }

    LOCK(cs_main);
    CBlockIndex* pNewIndex = nullptr;
    {
        LOCK2(cs_finblock, confirmMessageMan.cs_pbftmessage);
        auto pBpMsgMap = confirmMessageMan.InsertMessageNoLock(msg);

        pNewIndex = GetNewLocalFinIndex(msg);
        if (pNewIndex != nullptr) {
            ActiveDelegatesStore activeDelegatesStore;
            if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
                return ERRORMSG("get active delegates error");
            }
            const auto &bpList = GetBpListByHeight(activeDelegatesStore, pNewIndex->height);
            if (confirmMessageMan.CheckConfirm(pBpMsgMap, bpList)) {
                local_fin_index = pNewIndex;
                local_fin_last_update = GetTime();
            } else {
                pNewIndex = nullptr;
            }
        }
    }

    RelayBlockConfirmMessage(msg);

    if(pNewIndex != nullptr){
        BroadcastBlockFinality(GetLocalFinIndex());
    }
    return true;
}

bool CPBFTMan::ProcessBlockFinalityMessage(CNode *pFrom, const CBlockFinalityMessage& msg) {
    CPBFTMessageMan<CBlockFinalityMessage>& msgMan = finalityMessageMan;
    if(msgMan.IsKnown(msg)){
        LogPrint(BCLog::NET, "duplicated finality message, miner=%s, block=%s \n",
                 msg.miner.ToString(), msg.GetBlockId());
        return false;
    }

    if(!CheckPBFTMessage(pFrom, PBFTMsgType::FINALITY_BLOCK, msg)){
        LogPrint(BCLog::NET, "finality block message check failed, miner=%s, block=%s \n",
                 msg.miner.ToString(), msg.GetBlockId());
        return false;
    }

    LOCK(cs_main);
    {
        LOCK2(cs_finblock, finalityMessageMan.cs_pbftmessage);

        auto pBpMsgMap = finalityMessageMan.InsertMessageNoLock(msg);

        CBlockIndex* pNewIndex = GetNewGlobalFinIndex(msg);
        if (pNewIndex != nullptr) {
            ActiveDelegatesStore activeDelegatesStore;
            if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegatesStore)) {
                return ERRORMSG("get active delegates error");
            }
            const auto &bpList = GetBpListByHeight(activeDelegatesStore, pNewIndex->height);
            if (finalityMessageMan.CheckConfirm(pBpMsgMap, bpList)) {
                UpdateGlobalFinBlock(pNewIndex);
            }
        }
    }

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

bool CPBFTMan::BroadcastBlockFinality(const CBlockIndex* pTipIndex){

    AssertLockHeld(cs_main);

    if(!SysCfg().GetBoolArg("-genblock", false))
        return false;

    if(IsInitialBlockDownload())
        return false;

    if(finalityMessageMan.IsBroadcastedBlock(pTipIndex->GetBlockHash()))
        return true;

    if(pTipIndex->pprev == nullptr)
        return false;

    VoteDelegateVector activeDelegates;
    if (!pCdMan->pDelegateCache->GetActiveDelegates(activeDelegates)) {
        return ERRORMSG("get active delegates error");
    }

    CBlockFinalityMessage msg(pTipIndex->height, pTipIndex->GetBlockHash(), pTipIndex->pprev->GetBlockHash());
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
                    pNode->PushBlockFinalityMessage(msg);
                }
            }

            LogPrint(BCLog::PBFT, "generate and broadcast pbft finality msg! block=%s, bp=%s\n",
                pTipIndex->GetIdString(), delegate.regid.ToString());
            finalityMessageMan.InsertMessage(msg);

        }
    }

    finalityMessageMan.SaveBroadcastedBlock(pTipIndex->GetBlockHash());
    return true;

}

bool CPBFTMan::BroadcastBlockConfirm(const CBlockIndex* pTipIndex) {

    AssertLockHeld(cs_main);

    if(!SysCfg().GetBoolArg("-genblock", false))
        return false;


    if(GetTime() - pTipIndex->GetBlockTime() > 60)
        return false;

    if(IsInitialBlockDownload())
        return false;

    CPBFTMessageMan<CBlockConfirmMessage>& msgMan = confirmMessageMan;

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
            msgMan.InsertMessage(msg);
        }
    }

    msgMan.SaveBroadcastedBlock(pTipIndex->GetBlockHash());
    return true;
}

bool CPBFTMan::CheckPBFTMessage(CNode *pFrom, const int32_t msgType ,const CPBFTMessage& msg) {

    //check message type;
    if(msg.msgType != msgType ) {
        LogPrint(BCLog::INFO, "Misbehaving: invalid pbft_msg_type=%d, expected=%d, Misbehavior add 100\n", msg.msgType, msgType);
        Misbehaving(pFrom->GetId(), 100);
        return false;
    }

    CAccount account;
    {
        LOCK(cs_main);
        uint32_t tipHeight = chainActive.Height();
        uint32_t minHeight = (tipHeight > PBFT_LATEST_BLOCK_COUNT) ? tipHeight - PBFT_LATEST_BLOCK_COUNT : 0;
        CBlockIndex *pFinIndex = nullptr;
        {
            LOCK(cs_finblock);
            if (msgType == PBFTMsgType::CONFIRM_BLOCK) {
                pFinIndex = local_fin_index;
            } else {
                pFinIndex = global_fin_index;
            }
        }
        if (pFinIndex != nullptr && chainActive.Contains(pFinIndex)) {
            minHeight = max(minHeight, (uint32_t)pFinIndex->height);
        }

        uint32_t maxHeight = tipHeight + PBFT_LATEST_BLOCK_COUNT;
        if( msg.height < minHeight || msg.height > maxHeight ) {
            LogPrint(BCLog::PBFT, "messages height=%u is out of valid range[%u, %u]\n",
                msg.height, minHeight, maxHeight);
            return false; // ignore the msg
        }

        // check whether in chainActive
        if (msg.height <= tipHeight) {
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
