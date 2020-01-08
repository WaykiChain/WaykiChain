// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include <algorithm>
#include "main.h"

extern bool CheckIsGoverner(CRegID account, ProposalType proposalType,CCacheWrapper&cw );
extern uint8_t GetNeedGovernerCount(ProposalType proposalType, CCacheWrapper& cw );
bool CParamsGovernProposal::ExecuteProposal(CCacheWrapper &cw, CValidationState& state){

    for( auto pa: param_values){
        auto itr = paramNameToKeyMap.find(pa.first);
        if(itr == paramNameToKeyMap.end())
            return false ;

        if(!cw.sysParamCache.SetParam(itr->second, pa.second)){
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
           if(paramNameToKeyMap.count(pa.first) == 0){
               return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                       REJECT_INVALID, "params-error");
           }
       }

     return true ;
}

bool CGovernerUpdateProposal::ExecuteProposal(CCacheWrapper &cw, CValidationState& state){


    if(operate_type == OperateType::DISABLE){
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

    }else if(operate_type == OperateType::ENABLE){

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

     if(operate_type != OperateType::ENABLE && operate_type != OperateType::DISABLE){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, operate type is illegal!"), REJECT_INVALID,
                          "operate_type-illegal");
     }

     CAccount governer_account ;
     if(!cw.accountCache.GetAccount(governer_regid,governer_account)){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, governer regid(%s) is not exist!", governer_regid.ToString()), REJECT_INVALID,
                          "governer-not-exist");
     }

     vector<CRegID> governers ;
     if(operate_type == OperateType ::DISABLE&&!CheckIsGoverner(governer_regid,ProposalType(proposal_type),cw)){
         return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, regid(%s) is not a governer!", governer_regid.ToString()), REJECT_INVALID,
                          "regid-not-governer");
     }
    return true ;
}

bool CDexSwitchProposal::ExecuteProposal(CCacheWrapper &cw, CValidationState& state) {
    //uint32_t dexid ;
    //OperateType operate_type = OperateType ::ENABLE;

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is not a governer!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if((dexOperator.activated && operate_type == OperateType::ENABLE)||
       (!dexOperator.activated && operate_type == OperateType::DISABLE)){
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


    if(operate_type != OperateType::ENABLE && operate_type != OperateType::DISABLE){
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, operate type error!"), REJECT_INVALID,
                         "operate-type-error");
    }

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is not a governer!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if((dexOperator.activated && operate_type == OperateType::ENABLE)||
        (!dexOperator.activated && operate_type == OperateType::DISABLE)){
        return state.DoS(100, ERRORMSG("CProposalCreateTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");
    }



    return true ;
}




