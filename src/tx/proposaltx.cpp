// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "tx/proposaltx.h"
#include "main.h"
#include "entities/proposal.h"
#include <algorithm>

bool CheckIsGovernor(CRegID account, ProposalType proposalType, CCacheWrapper& cw ){

   if(proposalType == ProposalType::GOVERNOR_UPDATE || proposalType == ProposalType::COIN_TRANSFER){
        VoteDelegateVector delegateList;
        if (!cw.delegateCache.GetActiveDelegates(delegateList)) {
            return false;
        }
        for(auto miner: delegateList){
            if(miner.regid == account)
                return true ;
        }
        return false ;

    } else{
        return cw.sysGovernCache.CheckIsGovernor(account) ;
    }

}

uint8_t GetGovernorApprovalMinCount(ProposalType proposalType, CCacheWrapper& cw ) {

    switch (proposalType) {
        case ProposalType::GOVERNOR_UPDATE : {
            VoteDelegateVector delegateList;
            if (!cw.delegateCache.GetActiveDelegates(delegateList))
                return 8;
            
            if (delegateList.size() == 11)
                return 8 ;

            return ((delegateList.size()/3)*2+1);
        }
        case ProposalType::COIN_TRANSFER : {
            VoteDelegateVector delegateList;
            if (!cw.delegateCache.GetActiveDelegates(delegateList))
                return 8 ;
            
            return delegateList.size();
        }
        default:
            return cw.sysGovernCache.GetGovernorApprovalMinCount();
    }
}


string CProposalRequestTx::ToString(CAccountDBCache &accountCache) {
    string proposalString = proposal.ptr_proposal->ToString() ;
    return strprintf("txType=%s, hash=%s, ver=%d, %s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, proposalString, llFees,
                     txUid.ToString(), valid_height);
}          // logging usage

Object CProposalRequestTx::ToJson(const CAccountDBCache &accountCache) const {

    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("proposal", proposal.ptr_proposal->ToJson()));

    return result;
}  // json-rpc usage

 bool CProposalRequestTx::CheckTx(CTxExecuteContext &context) {

     IMPLEMENT_DEFINE_CW_STATE
     IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid);
     if (!CheckFee(context)) return false;

     if(!proposal.ptr_proposal->CheckProposal(context )){
         return false ;
     }

     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount))
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, read account failed"), REJECT_INVALID,
                          "bad-getaccount");

     CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
     IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);
     return true ;
}


 bool CProposalRequestTx::ExecuteTx(CTxExecuteContext &context) {

     IMPLEMENT_DEFINE_CW_STATE

     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
         return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, read source addr account info error"),
                          READ_ACCOUNT_FAIL, "bad-read-accountdb");
     }
     if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
         return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, account has insufficient funds"),
                          UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
     }

     if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
         return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, set account info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

     uint64_t expireBlockCount ;

     if(!cw.sysParamCache.GetParam(PROPOSAL_EXPIRE_BLOCK_COUNT, expireBlockCount)) {
         return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx,get proposal expire block count error"),
                          WRITE_ACCOUNT_FAIL, "get-expire-block-count-error");
     }

     auto newProposal = proposal.ptr_proposal->GetNewInstance() ;
     newProposal->expire_block_height = context.height + expireBlockCount ;
     newProposal->need_governor_count = GetGovernorApprovalMinCount(proposal.ptr_proposal->proposal_type, cw);

     if(!cw.sysGovernCache.SetProposal(GetHash(), newProposal)){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, set proposal info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
     }
    return true ;
}


string CProposalApprovalTx::ToString(CAccountDBCache &accountCache) {

    return strprintf("txType=%s, hash=%s, ver=%d, proposalid=%s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txid.GetHex(), llFees,
                     txUid.ToString(), valid_height);
}
 Object CProposalApprovalTx::ToJson(const CAccountDBCache &accountCache) const {

     Object result = CBaseTx::ToJson(accountCache);
     result.push_back(Pair("proposal_id", txid.ToString()));
     return result;
} // json-rpc usage

 bool CProposalApprovalTx::CheckTx(CTxExecuteContext &context) {

     IMPLEMENT_DEFINE_CW_STATE
     IMPLEMENT_CHECK_TX_REGID(txUid);
     if (!CheckFee(context)) return false;

     shared_ptr<CProposal> proposal ;
     if(!cw.sysGovernCache.GetProposal(txid,proposal)){
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, proposal(id=%s)  not found", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-not-found");
     }

     if(!CheckIsGovernor(txUid.get<CRegID>(), proposal->proposal_type,cw)){
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, the tx commiter(%s) is not a governor", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "permission-deney");
     }

     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount))
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, read account failed"), REJECT_INVALID,
                          "bad-getaccount");

     CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
     IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true ;
}

bool CProposalApprovalTx::ExecuteTx(CTxExecuteContext &context) {

     IMPLEMENT_DEFINE_CW_STATE
     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, read source addr account info error"),
                          READ_ACCOUNT_FAIL, "bad-read-accountdb");
     }

     if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, account has insufficient funds"),
                          UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
     }

     shared_ptr<CProposal> proposal ;
     if(!cw.sysGovernCache.GetProposal(txid,proposal)){
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, proposal(id=%s)  not found", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-not-found");
     }

     if(proposal->expire_block_height < context.height){
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, proposal(id=%s)  is expired", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-expired");
     }

     if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, set account info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

     if(!cw.sysGovernCache.SetApproval(txid, txUid.get<CRegID>())){
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, set proposal assention info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
     }

     auto assentedCount = cw.sysGovernCache.GetApprovalCount(txid);

     if(assentedCount > proposal->need_governor_count){
         return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, proposal executed already"),
                          WRITE_ACCOUNT_FAIL, "proposal-executed-already");
     }

     if( assentedCount == proposal->need_governor_count){

         if(!proposal->ExecuteProposal(context)){
             return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, proposal execute error"),
                              WRITE_ACCOUNT_FAIL, "proposal-execute-error");
         }
     }

     return true ;
}