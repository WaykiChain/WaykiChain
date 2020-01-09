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

    if(fHelp || params.size() != 1){

        throw runtime_error(
                "getproposal \"proposalid\"\n"
                "get a proposal by proposal id\n"
                "\nArguments:\n"
                "1.\"proposalid\":      (string, required) the proposal id \n"

                "\nExamples:\n"
                + HelpExampleCli("getproposal", "02sov0efs3ewdsxcfresdfdsadfgdsasdfdsadfdsdfsdfsddfge32ewsrewsowekdsx")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getproposal", "02sov0efs3ewdsxcfresdfdsadfgdsasdfdsadfdsdfsdfsddfge32ewsrewsowekdsx")

        );

    }

    uint256 proposalId = uint256S(params[0].get_str()) ;
    std::shared_ptr<CProposal> pp ;
    if(pCdMan->pSysGovernCache->GetProposal(proposalId, pp)){
        return pp->ToJson() ;
    }
    return Object();
}


Value submitparamgovernproposal(const Array& params, bool fHelp){

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitparamgovernproposal \"addr\" \"param_name\" \"param_value\" [\"fee\"]\n"
                "create proposal about param govern\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"param_name\":       (string, required) the name of param, the param list can be found in document \n"
                "3.\"param_value\":      (numberic, required) the param value that will be updated to \n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitparamgovernproposal", "0-1 ASSET_ISSUE_FEE  10000 WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitparamgovernproposal", "0-1 ASSET_ISSUE_FEE  10000 WICC:1:WI")

                );

    }


    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string paramName = params[1].get_str() ;
    uint64_t paramValue = AmountToRawValue(params[2]) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CParamsGovernProposal proposal ;
    proposal.param_values.push_back(std::make_pair(paramName, paramValue));

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CParamsGovernProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitgovernerupdateproposal(const Array& params , bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitgovernerupdateproposal \"addr\" \"governer_uid\" \"operate_type\" [\"fee\"]\n"
                "create proposal about  add/remove a governer \n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"governer_uid\":     (string, required) the governer's uid\n"
                "3.\"operate_type\":     (numberic, required) the operate type \n"
                "                         1 stand for add\n"
                "                         2 stand for remove\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitgovernerupdateproposal", "0-1 100-2 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitgovernerupdateproposal", "0-1 100-2 1  WICC:1:WI")

        );

    }

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    CRegID governerId = CRegID(params[1].get_str()) ;
    uint64_t operateType = AmountToRawValue(params[2]) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CGovernerUpdateProposal proposal ;
    proposal.governer_regid = governerId ;
    proposal.operate_type = OperateType(operateType);

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CGovernerUpdateProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitdexswitchproposal(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitdexswitchproposal \"addr\" \"dexid\" \"operate_type\" [\"fee\"]\n"
                "create proposal about enable/disable dexoperator\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"dexid\":            (numberic, required) the dexoperator's id\n"
                "3.\"operate_type\":     (numberic, required) the operate type \n"
                "                         1 stand for enable\n"
                "                         2 stand for disable\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitdexswitchproposal", "0-1 1 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitdexswitchproposal", "0-1 1 1  WICC:1:WI")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint64_t dexId = params[1].get_int();
    uint64_t operateType = params[2].get_int();
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CDexSwitchProposal proposal ;
    proposal.dexid = dexId ;
    proposal.operate_type = OperateType(operateType);

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CDexSwitchProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;
}

Value submitproposalassenttx(const Array& params, bool fHelp){

    if(fHelp || params.size() < 2 || params.size() > 3){
        throw runtime_error(
                "submitproposalassenttx \"addr\" \"proposalid\" [\"fee\"]\n"
                "assent a proposal\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"proposalid\":       (numberic, required) the dexoperator's id\n"
                "3.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitproposalassenttx", "0-1 1 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitproposalassenttx", "0-1 1 1  WICC:1:WI")

        );
    }


    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint256 proposalId = uint256S(params[1].get_str()) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 2, PROPOSAL_CREATE_TX);
    int32_t validHegiht  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CProposalAssentTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHegiht;
    tx.txid = proposalId ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitminerfeeproposal(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 4 || params.size() > 5){

        throw runtime_error(
                "submitdexswitchproposal \"addr\" \"tx_type\" \"fee_symbol\" \"fee_sawi_amount\" [\"fee\"]\n"
                "create proposal about enable/disable dexoperator\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"dexid\":            (numberic, required) the dexoperator's id\n"
                "3.\"operate_type\":     (numberic, required) the operate type \n"
                "                         1 stand for enable\n"
                "                         2 stand for disable\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitdexswitchproposal", "0-1 1 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitdexswitchproposal", "0-1 1 1  WICC:1:WI")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint8_t txType = params[1].get_int();
    string feeSymbol = params[2].get_str();
    uint64_t feeSawiAmount = AmountToRawValue(params[3]) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 4, PROPOSAL_CREATE_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CMinerFeeProposal proposal ;
    proposal.tx_type = TxType(txType)  ;
    proposal.fee_symbol = feeSymbol;
    proposal.fee_sawi_amount = feeSawiAmount;

    CProposalCreateTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = std::make_shared<CMinerFeeProposal>(proposal) ;
    return SubmitTx(account.keyid, tx) ;

}

Value getsysparam(const Array& params, bool fHelp){

    if(fHelp || params.size() != 1){

        throw runtime_error(
                "getsysparam \"param_name\"\n"
                "create proposal about param govern\n"
                "\nArguments:\n"
                "1.\"param_name\":      (string, required) param name \n"

                "\nExamples:\n"
                + HelpExampleCli("getsysparam", "ASSET_UPDATE_FEE")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getsysparam", "ASSET_UPDATE_FEE")
        );

    }

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
