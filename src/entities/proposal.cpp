// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include <algorithm>
#include "main.h"



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


bool CGovernerUpdateProposal::ExecuteProposal(CCacheWrapper &cw, CValidationState& state){


    if(operateType == OperateType::DISABLE){
        vector<CRegID> governers ;
        if(cw.sysGovernCache.GetGoverners(governers)){

            for(auto itr = governers.begin();itr !=governers.end();){
                if(*itr == governerRegId){
                    governers.erase(itr);
                    break ;
                }else
                    itr++ ;
            }
            return cw.sysGovernCache.SetGoverners(governers) ;
        }
        return false ;

    }else if(operateType == OperateType::ENABLE){

        vector<CRegID> governers ;
        cw.sysGovernCache.GetGoverners(governers);

        if(find( governers.begin(),governers.end(),governerRegId) == governers.end()){
            return false ;
        }

        governers.push_back(governerRegId) ;
        return cw.sysGovernCache.SetGoverners(governers) ;
    }

    return false  ;

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
 bool CGovernerUpdateProposal::CheckProposal(CCacheWrapper &cw, CValidationState& state){
    return true ;
}