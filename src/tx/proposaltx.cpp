// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "proposaltx.h"
#include "main.h"
#include "scoin.h"
#include "entities/proposal.h"
#include <algorithm>

bool CheckIsGoverner(CRegID account, uint8_t proposalType, CCacheWrapper& cw ){

    if(proposalType == ProposalType::PARAM_GOVERN){
        return cw.sysGovernCache.CheckIsGoverner(account) ;
    } else if(proposalType == ProposalType::GOVERNER_UPDATE){
        vector<CRegID> delegateList;
        if (!pCdMan->pDelegateCache->GetTopDelegateList(delegateList)) {
            return false;
        }
        return (find(delegateList.begin(), delegateList.end(), account) != delegateList.end()) ;

    } else{
        return false ;
    }

}

string CProposalCreateTx::ToString(CAccountDBCache &accountCache) {
    return "" ;
}          // logging usage

Object CProposalCreateTx::ToJson(const CAccountDBCache &accountCache) const {
    return Object() ;
}  // json-rpc usage

 bool CProposalCreateTx::CheckTx(CTxExecuteContext &context) {
     CCacheWrapper &cw       = *context.pCw;
     CValidationState &state = *context.pState;
     IMPLEMENT_CHECK_TX_FEE;
     IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

     if(!proposal->CheckProposal(cw ,state)){
         return false ;
     }

     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount))
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, read account failed"), REJECT_INVALID,
                          "bad-getaccount");

     CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
     IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);
     return true ;
}


 bool CProposalCreateTx::ExecuteTx(CTxExecuteContext &context) {

     CCacheWrapper &cw       = *context.pCw;
     CValidationState &state = *context.pState;

     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
         return state.DoS(100, ERRORMSG("CProposalCreateTx::ExecuteTx, read source addr account info error"),
                          READ_ACCOUNT_FAIL, "bad-read-accountdb");
     }
     if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
         return state.DoS(100, ERRORMSG("CProposalCreateTx::ExecuteTx, account has insufficient funds"),
                          UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
     }

     if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
         return state.DoS(100, ERRORMSG("CProposalCreateTx::ExecuteTx, set account info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

     auto newProposal = proposal->GetNewInstance() ;
     newProposal->expire_block_height = context.height + 1200 ;
     newProposal->need_governer_count = cw.sysGovernCache.GetNeedGovernerCount();

     if(!cw.sysGovernCache.SetProposal(ComputeSignatureHash(), newProposal)){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::ExecuteTx, set proposal info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
     }
    return true ;
}


string CProposalAssentTx::ToString(CAccountDBCache &accountCache) {
    return "";
}            // logging usage
 Object CProposalAssentTx::ToJson(const CAccountDBCache &accountCache) const {
    return Object() ;
} // json-rpc usage

 bool CProposalAssentTx::CheckTx(CTxExecuteContext &context) {
     CCacheWrapper &cw       = *context.pCw;
     CValidationState &state = *context.pState;
     IMPLEMENT_CHECK_TX_FEE;
     IMPLEMENT_CHECK_TX_REGID(txUid.type());

     shared_ptr<CProposal> proposal ;
     if(!cw.sysGovernCache.GetProposal(txid,proposal)){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::CheckTx, proposal(id=%s)  not found", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-not-found");
     }

     if(!CheckIsGoverner(txUid.get<CRegID>(), proposal->proposalType,cw)){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::CheckTx, the tx commiter is not a governer", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "permission-deney");
     }

     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount))
         return state.DoS(100, ERRORMSG("CProposalAssentTx::CheckTx, read account failed"), REJECT_INVALID,
                          "bad-getaccount");

     CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey );
     IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true ;
}
 bool CProposalAssentTx::ExecuteTx(CTxExecuteContext &context) {

     CCacheWrapper &cw       = *context.pCw;
     CValidationState &state = *context.pState;
     CAccount srcAccount;
     if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, read source addr account info error"),
                          READ_ACCOUNT_FAIL, "bad-read-accountdb");
     }

     if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, account has insufficient funds"),
                          UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
     }

     shared_ptr<CProposal> proposal ;
     if(!cw.sysGovernCache.GetProposal(txid,proposal)){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::CheckTx, proposal(id=%s)  not found", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-not-found");
     }

     if(proposal->expire_block_height < context.height){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, proposal(id=%s)  is expired", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-expired");
     }

     if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, set account info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

     if(!cw.sysGovernCache.SetAssention(txid, txUid.get<CRegID>())){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, set proposal assention info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
     }

     if(cw.sysGovernCache.GetAssentionCount(txid) == proposal->need_governer_count){

         if(!proposal->ExecuteProposal(cw, state)){
             return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, proposal execute error"),
                              WRITE_ACCOUNT_FAIL, "proposal-execute-error");
         }
     }

     return true ;
}