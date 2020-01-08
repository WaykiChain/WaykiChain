// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcproposal.h"
#include "main.h"
#include "entities/proposalserializer.h"
#include "tx/proposaltx.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"


Value getproposal(const Array& params, bool fHelp){
    uint256 proposalId = uint256S(params[0].get_str()) ;
    std::shared_ptr<CProposal> pp ;
    if(pCdMan->pSysGovernCache->GetProposal(proposalId, pp)){
        return pp->ToJson() ;
    }
    return Object();
}


Value submitparamgovernproposal(const Array& params, bool fHelp){

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string paramName = params[1].get_str() ;
    uint64_t paramValue = AmountToRawValue(params[2]) ;
    int64_t fee          = RPC_PARAM::GetWiccFee(params, 3, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee);

    CParamsGovernProposal proposal ;
    proposal.param_values.push_back(std::make_pair(paramName, paramValue));

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee;
    tx.fee_symbol    = SYMB::WICC ;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CParamsGovernProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitgovernerupdateproposal(const Array& params , bool fHelp) {

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    CRegID governerId = CRegID(params[1].get_str()) ;
    uint64_t operateType = AmountToRawValue(params[2]) ;
    int64_t fee          = RPC_PARAM::GetWiccFee(params, 3, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee);

    CGovernerUpdateProposal proposal ;
    proposal.governer_regid = governerId ;
    proposal.operate_type = OperateType(operateType);

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee;
    tx.fee_symbol    = SYMB::WICC ;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CGovernerUpdateProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitdexswitchproposal(const Array& params, bool fHelp) {

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint64_t dexId = params[1].get_int();
    uint64_t operateType = params[2].get_int();
    int64_t fee          = RPC_PARAM::GetWiccFee(params, 3, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee);

    CDexSwitchProposal proposal ;
    proposal.dexid = dexId ;
    proposal.operate_type = OperateType(operateType);

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee;
    tx.fee_symbol    = SYMB::WICC ;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CDexSwitchProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;
}

Value submitproposalassenttx(const Array& params, bool fHelp){

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint256 proposalId = uint256S(params[1].get_str()) ;
    int64_t fee          = RPC_PARAM::GetWiccFee(params, 2, PROPOSAL_CREATE_TX);
    int32_t validHegiht  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee);

    CProposalAssentTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee;
    tx.fee_symbol    = SYMB::WICC ;
    tx.valid_height = validHegiht;
    tx.txid = proposalId ;
    return SubmitTx(account.keyid, tx) ;

}

Value getsysparam(const Array& params, bool fHelp){

    string paramName = params[0].get_str() ;
    SysParamType st ;
    auto itr = paramNameToSysParamTypeMap.find(paramName) ;
    if( itr == paramNameToSysParamTypeMap.end()){
        throw JSONRPCError(RPC_INVALID_PARAMETER, "param name is illegal");
    }
    st = itr->second ;
    uint64_t pv ;
    if(!pCdMan->pSysParamCache->GetParam(st, pv)){
        throw JSONRPCError(RPC_INVALID_PARAMETER, "get param error");
    }

    Object obj ;
    obj.push_back(Pair(paramName, pv));
    return obj;

}
