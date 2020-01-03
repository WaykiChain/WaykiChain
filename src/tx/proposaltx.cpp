// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "proposaltx.h"
#include "main.h"
#include "scoin.h"
#include "entities/proposal.h"

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
     if(params.size() == 0)
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, params list is empty"), REJECT_INVALID,
                          "params-empty");
     for(auto pa: params){
         if(paramNameToKeyMap.count(pa.first) == 0){
             return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                     REJECT_INVALID, "params-error");
         }
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

     CProposal proposal ;
     proposal.expire_block_height = context.height+1200;
     for(auto pa: params){
         auto param = paramNameToKeyMap.find(pa.first) ;
         proposal.paramValues.push_back({param->second , pa.second}) ;
     }

     if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
         return state.DoS(100, ERRORMSG("CProposalCreateTx::ExecuteTx, set account info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

     if(!cw.sysGovernCache.SetProposal(ComputeSignatureHash(), proposal)){
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



     if(!cw.sysGovernCache.CheckIsGoverner(txUid.get<CRegID>())){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::CheckTx, txUid(regid=%s)  is not a governer", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-expired");
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

     CProposal proposal ;
     if(!cw.sysGovernCache.GetProposal(txid,proposal)){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, proposal(id=%s)  not found", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-not-found");
     }

     if(proposal.expire_block_height < context.height){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, proposal(id=%s)  is expired", txid.ToString()),
                          WRITE_ACCOUNT_FAIL, "proposal-expired");
     }

     if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, set account info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

     if(!cw.sysGovernCache.SetAssention(ComputeSignatureHash(), txUid.get<CRegID>())){
         return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, set proposal assention info error"),
                          WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
     }



     if(cw.sysGovernCache.GetAssentionCount(txid) == proposal.needGovernerCount){
         if(!proposal.ExecuteProposal(cw)){
             return state.DoS(100, ERRORMSG("CProposalAssentTx::ExecuteTx, proposal execute error"),
                              WRITE_ACCOUNT_FAIL, "proposal-execute-error");
         }
     }

     return true ;
}