// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include <algorithm>



bool CParamsGovernProposal::ExecuteProposal(CCacheWrapper &cw, CValidationState& state){

    for( auto pa: param_values){
        if(!cw.sysParamCache.SetParam(pa.first, pa.second)){
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

    return true ;
}
 bool CGovernerUpdateProposal::CheckProposal(CCacheWrapper &cw, CValidationState& state){
    return true ;
}