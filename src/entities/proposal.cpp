// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include <algorithm>
#include "main.h"
#include <set>
#include "config/txbase.h"

extern bool CheckIsGoverner(CRegID account, ProposalType proposalType,CCacheWrapper&cw );
extern uint8_t GetNeedGovernerCount(ProposalType proposalType, CCacheWrapper& cw );
bool CParamsGovernProposal::ExecuteProposal(CTxExecuteContext& context){

    CCacheWrapper &cw       = *context.pCw;
    for( auto pa: param_values){
        auto itr = SysParamTable.find(SysParamType(pa.first));
        if(itr == SysParamTable.end())
            return false ;

        if(!cw.sysParamCache.SetParam(SysParamType(pa.first), pa.second)){
            return false ;
        }

    }

    return true ;

}

 bool CParamsGovernProposal::CheckProposal(CCacheWrapper &cw, CValidationState& state){

       if(param_values.size() == 0)
            return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, params list is empty"), REJECT_INVALID,
                        "params-empty");
       for(auto pa: param_values){
           if(SysParamTable.count(SysParamType(pa.first)) == 0){
               return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                       REJECT_INVALID, "params-error");
           }
       }

     return true ;
}

bool CCdpParamGovernProposal::ExecuteProposal(CTxExecuteContext& context){

    CCacheWrapper &cw       = *context.pCw;
    for( auto pa: param_values){
        auto itr = CdpParamTable.find(CdpParamType(pa.first));
        if(itr == CdpParamTable.end())
            return false ;

        if(!cw.sysParamCache.SetCdpParam(coinPair,CdpParamType(pa.first), pa.second)){
            return false ;
        }
        if(pa.first == CdpParamType ::CDP_INTEREST_PARAM_A
           || pa.first == CdpParamType::CDP_INTEREST_PARAM_B){

            if(!cw.sysParamCache.SetCdpInterestParam(coinPair, CdpParamType(pa.first), context.height, pa.second)){
                return false ;
            }

        }
    }

    return true ;

}

bool CCdpParamGovernProposal::CheckProposal(CCacheWrapper &cw, CValidationState& state) {

    if (param_values.size() == 0)
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, params list is empty"), REJECT_INVALID,
                         "params-empty");
    for (auto pa: param_values) {
        if (SysParamTable.count(SysParamType(pa.first)) == 0) {
            return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, parameter name (%s) is not in sys params list ",
                                           pa.first),
                             REJECT_INVALID, "params-error");
        }
    }

    return true;
}

bool CGovernerUpdateProposal::ExecuteProposal(CTxExecuteContext& context){

    CCacheWrapper &cw       = *context.pCw;
    if(operate_type == ProposalOperateType::DISABLE){
        vector<CRegID> governers ;
        if(cw.sysGovernCache.GetGoverners(governers)){

            for(auto itr = governers.begin();itr !=governers.end();){
                if(*itr == governer_regid){
                    governers.erase(itr);
                    break ;
                }else
                    itr++ ;
            }
            return cw.sysGovernCache.SetGoverners(governers) ;
        }
        return false ;

    }else if(operate_type == ProposalOperateType::ENABLE){

        vector<CRegID> governers ;
        cw.sysGovernCache.GetGoverners(governers);

        if(find( governers.begin(),governers.end(),governer_regid) != governers.end()){
            return false ;
        }

        governers.push_back(governer_regid) ;
        return cw.sysGovernCache.SetGoverners(governers) ;
    }

    return false  ;

}

 bool CGovernerUpdateProposal::CheckProposal(CCacheWrapper &cw, CValidationState& state){

     if(operate_type != ProposalOperateType::ENABLE && operate_type != ProposalOperateType::DISABLE){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, operate type is illegal!"), REJECT_INVALID,
                          "operate_type-illegal");
     }

     CAccount governer_account ;
     if(!cw.accountCache.GetAccount(governer_regid,governer_account)){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, governer regid(%s) is not exist!", governer_regid.ToString()), REJECT_INVALID,
                          "governer-not-exist");
     }
     vector<CRegID> governers ;

     if(operate_type == ProposalOperateType ::DISABLE&&!cw.sysGovernCache.CheckIsGoverner(governer_regid)){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, regid(%s) is not a governer!", governer_regid.ToString()), REJECT_INVALID,
                          "regid-not-governer");
     }
    return true ;
}

bool CDexSwitchProposal::ExecuteProposal(CTxExecuteContext& context) {

    IMPLEMENT_DEFINE_CW_STATE
    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is not a governer!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
       (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE)){
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");
    }

    dexOperator.activated = !dexOperator.activated ;

    DexOperatorDetail newOperator  {
        dexOperator.owner_regid,
        dexOperator.fee_receiver_regid,
        dexOperator.name,
        dexOperator.portal_url,
        dexOperator.maker_fee_ratio,
        dexOperator.taker_fee_ratio,
        dexOperator.memo,
        dexOperator.activated
    };



    if (!cw.dexCache.UpdateDexOperator(dexid, dexOperator, newOperator))
        return state.DoS(100, ERRORMSG("%s, save updated dex operator error! dex_id=%u", __func__, dexid),
                         UPDATE_ACCOUNT_FAIL, "save-updated-operator-error");

    return true ;
}
bool CDexSwitchProposal::CheckProposal(CCacheWrapper &cw, CValidationState& state) {


    if(operate_type != ProposalOperateType::ENABLE && operate_type != ProposalOperateType::DISABLE){
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, operate type error!"), REJECT_INVALID,
                         "operate-type-error");
    }

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is not a governer!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
        (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE)){
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");
    }

    return true ;
}


bool CMinerFeeProposal:: CheckProposal(CCacheWrapper &cw, CValidationState& state) {

  if(!kFeeSymbolSet.count(fee_symbol)) {
      return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, fee symbol(%s) is invalid!", fee_symbol),
                       REJECT_INVALID,
                       "feesymbol-error");
  }

  auto itr = kTxFeeTable.find(tx_type);
  if(itr == kTxFeeTable.end()){
      return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, the tx type (%d) is invalid!", tx_type),
                       REJECT_INVALID,
                       "txtype-error");
  }

  if(!std::get<5>(itr->second)){
      return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, the tx type (%d) miner fee can't be updated!", tx_type),
                       REJECT_INVALID,
                       "can-not-update");
  }

  if(fee_sawi_amount == 0 ){
      return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, the tx type (%d) miner fee can't be zero", tx_type),
                       REJECT_INVALID,
                       "can-not-be-zero");
  }
  return true ;
}

bool CMinerFeeProposal:: ExecuteProposal(CTxExecuteContext& context) {
    CCacheWrapper &cw       = *context.pCw;
    return cw.sysParamCache.SetMinerFee(tx_type,fee_symbol,fee_sawi_amount);
}



bool CCoinTransferProposal:: ExecuteProposal(CTxExecuteContext& context) {


    IMPLEMENT_DEFINE_CW_STATE;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t minusValue = amount;
    if (!srcAccount.OperateBalance(token, BalanceOpType::SUB_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(to_uid, desAccount)) {
        if (to_uid.is<CKeyID>()) {  // first involved in transaction
            desAccount.keyid = to_uid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAccount.OperateBalance(token, BalanceOpType::ADD_FREE, amount)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(to_uid, desAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account error, kyeId=%s",
                                       desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");


    return true ;
}
bool CCoinTransferProposal:: CheckProposal(CCacheWrapper &cw, CValidationState& state) {

    if (amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, dust amount, %llu < %llu", amount,
                                       DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");



    return true ;
}



bool CBPCountUpdateProposal:: ExecuteProposal(CTxExecuteContext& context) {

    return true ;

}
bool CBPCountUpdateProposal:: CheckProposal(CCacheWrapper &cw, CValidationState& state) {


    return true  ;
}

