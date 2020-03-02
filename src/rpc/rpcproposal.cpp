// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "tx/proposaltx.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"


SysParamType  GetParamType(const string  paramName){
    auto itr = paramNameToSysParamTypeMap.find(paramName);
    if(itr == paramNameToSysParamTypeMap.end())
        return NULL_SYS_PARAM_TYPE;
    else
        return std::get<1>(itr->second);

}

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
        auto proposalObject  = pp->ToJson() ;
        vector<CRegID> approvalList ;
        pCdMan->pSysGovernCache->GetApprovalList(proposalId, approvalList);

        proposalObject.push_back(Pair("approvaled_count", (uint64_t)approvalList.size()));

            Array a ;
            for ( CRegID i: approvalList) {
              a.push_back(i.ToString()) ;
            }
        proposalObject.push_back(Pair("approvaled_governors", a));

        return proposalObject ;

    }
    return Object();
}

Value getgovernors(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0 ) {
        throw runtime_error(
                "getgovernors \n"
                " get thee governors list \n"
                "\nArguments:\n "
                "\nExamples:\n"
                + HelpExampleCli("getgovernors", "")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getgovernors", "")

        );
    }


    Object o ;
    vector<CRegID> governors ;
    pCdMan->pSysGovernCache->GetGovernors(governors) ;
    Array arr ;
    for(auto id: governors) {
        arr.push_back(id.ToString()) ;
    }
    o.push_back(Pair("governors", arr)) ;

    return o ;

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
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CParamsGovernProposal proposal ;


    SysParamType  type = GetParamType(paramName) ;
    if(type == SysParamType::NULL_SYS_PARAM_TYPE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("system param type(%s) is not exist",paramName));

    proposal.param_values.push_back(std::make_pair(type, paramValue));

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CParamsGovernProposal>(proposal)) ;
    return SubmitTx(account.keyid, tx) ;

}


Value submitcdpparamgovernproposal(const Array& params, bool fHelp){

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitcdpparamgovernproposal \"addr\" \"param_name\" \"param_value\" \"bcoin_symbole\" \"scoin_symbol\" [\"fee\"]\n"
                "create proposal about cdp  param govern\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"param_name\":       (string, required) the name of param, the param list can be found in document \n"
                "3.\"param_value\":      (numberic, required) the param value that will be updated to \n"
                "4.\"bcoin_symbo\":      (string,required) the base coin symbol\n"
                "5.\"scoin_symbo\":      (string,required) the stable coin symbol\n"
                "6.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitcdpparamgovernproposal", "0-1 ASSET_ISSUE_FEE  10000 WICC WUSD WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitcdpparamgovernproposal", "0-1 ASSET_ISSUE_FEE  10000 WICC WUSD WICC:1:WI")

        );

    }


    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string paramName = params[1].get_str() ;
    string bcoinSymbol = params[3].get_str() ;
    string scoinSymbol = params[4].get_str() ;
    uint64_t paramValue = AmountToRawValue(params[2]) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CCdpParamGovernProposal proposal ;


    SysParamType  type = GetParamType(paramName) ;
    if(type == SysParamType::NULL_SYS_PARAM_TYPE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("system param type(%s) is not exist",paramName));

    proposal.param_values.push_back(std::make_pair(type, paramValue));
    proposal.coin_pair = CCdpCoinPair(bcoinSymbol, scoinSymbol) ;

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CCdpParamGovernProposal>(proposal)) ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitgovernorupdateproposal(const Array& params , bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitgovernorupdateproposal \"addr\" \"governor_uid\" \"operate_type\" [\"fee\"]\n"
                "create proposal about  add/remove a governor \n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"governor_uid\":     (string, required) the governor's uid\n"
                "3.\"operate_type\":     (numberic, required) the operate type \n"
                "                         1 stand for add\n"
                "                         2 stand for remove\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitgovernorupdateproposal", "0-1 100-2 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitgovernorupdateproposal", "0-1 100-2 1  WICC:1:WI")

        );

    }

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    CRegID governorId = CRegID(params[1].get_str()) ;
    uint64_t operateType = AmountToRawValue(params[2]) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CGovernorUpdateProposal proposal ;
    proposal.governor_regid = governorId ;
    proposal.op_type = ProposalOperateType(operateType);

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovernorUpdateProposal>(proposal)) ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitdexquotecoinproposal(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
                "submitdexquotecoinproposal \"addr\" \"token_symbol\" \"operate_type\" [\"fee\"]\n"
                "request proposal about add/remove dex quote coin\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"token_symbol\":     (string, required) the dex quote coin symbol\n"
                "3.\"op_type\":          (numberic, required) the operate type \n"
                "                         1 stand for add\n"
                "                         2 stand for remove\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitdexquotecoinproposal", "0-1 WUSD 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitdexquotecoinproposal", "0-1 WUSD 1  WICC:1:WI")

                );
    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string token = params[1].get_str();
    uint64_t operateType = params[2].get_int();
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CDexQuoteCoinProposal proposal ;
    proposal.coin_symbol = token ;
    proposal.op_type = ProposalOperateType(operateType);

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CDexQuoteCoinProposal>(proposal)) ;
    return SubmitTx(account.keyid, tx) ;


}


Value submitfeedcoinpairproposal(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
                "submitfeedcoinpairproposal \"addr\" \"feed_symbol\" \"base_symbol\" \"operate_type\" [\"fee\"]\n"
                "request proposal about add/remove feed price coin pair \n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"feed_symbol\":     (string, required) the feed coin symbol\n"
                "3.\"base_symbol\":     (string, required) the feed base coin symbol\n"
                "4.\"op_type\":          (numberic, required) the operate type \n"
                "                         1 stand for add\n"
                "                         2 stand for remove\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitfeedcoinpairproposal", "0-1 WICC WUSD 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitfeedcoinpairproposal", "0-1 WICC WUSD 1  WICC:1:WI")

        );
    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string feedSymbol = params[1].get_str();
    string baseSymbol = params[2].get_str();
    uint64_t operateType = params[3].get_int();
    ComboMoney fee          = RPC_PARAM::GetFee(params, 4, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CFeedCoinPairProposal proposal ;
    proposal.feed_symbol = feedSymbol ;
    proposal.base_symbol = baseSymbol ;
    proposal.op_type = ProposalOperateType(operateType);

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CFeedCoinPairProposal>(proposal)) ;
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
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CDexSwitchProposal proposal ;
    proposal.dexid = dexId ;
    proposal.operate_type = ProposalOperateType(operateType);

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CDexSwitchProposal>(proposal)) ;
    return SubmitTx(account.keyid, tx) ;
}

Value submitproposalapprovaltx(const Array& params, bool fHelp){

    if(fHelp || params.size() < 2 || params.size() > 3){
        throw runtime_error(
                "submitproposalapprovaltx \"addr\" \"proposalid\" [\"fee\"]\n"
                "assent a proposal\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"proposalid\":       (numberic, required) the dexoperator's id\n"
                "3.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitproposalapprovaltx", "0-1 1 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitproposalapprovaltx", "0-1 1 1  WICC:1:WI")

        );
    }


    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint256 proposalId = uint256S(params[1].get_str()) ;
    ComboMoney fee          = RPC_PARAM::GetFee(params, 2, PROPOSAL_REQUEST_TX);
    int32_t validHegiht  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CProposalApprovalTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHegiht;
    tx.txid = proposalId ;
    return SubmitTx(account.keyid, tx) ;

}

Value submitbpcountupdateproposal(const Array& params,bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4){
        throw runtime_error(
                "submitbpcountupdateproposal \"addr\" \"bp_count\" \"effective_height\"  [\"fee\"]\n"
                "create proposal about enable/disable dexoperator\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"bp_count\":         (numberic, required) the count of block producer(miner)  \n"
                "3.\"effective_height\":    (numberic, required) the height of the proposal launch \n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitbpcountupdateproposal", "0-1 21 45002020202  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitbpcountupdateproposal", "0-1 4332222233223  WICC:1:WI")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true ) ;
    uint8_t bpCount = params[1].get_int() ;
    uint32_t effectiveHeight = params[2].get_int() ;
    ComboMoney fee = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CBPCountUpdateProposal proposal ;
    proposal.bp_count = bpCount ;
    proposal.effective_height = effectiveHeight ;

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CBPCountUpdateProposal>(proposal)) ;

    return SubmitTx(account.keyid, tx) ;


}
Value submitminerfeeproposal(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitminerfeeproposal \"addr\" \"tx_type\" \"fee_info\"  [\"fee\"]\n"
                "create proposal about enable/disable dexoperator\n"
                "\nArguments:\n"
                "1.\"addr\":             (string, required) the tx submitor's address\n"
                "2.\"tx_type\":          (numberic, required) the tx type you can get the list by command \"listmintxfees\" \n"
                "3.\"fee_info\":         (combomoney, required) the miner fee symbol, example:WICC, WUSD \n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitminerfeeproposal", "0-1 1 WICC:1:WI  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitminerfeeproposal", "0-1 1 1  WICC:1:WI")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint8_t txType = params[1].get_int();
    ComboMoney feeInfo = RPC_PARAM::GetComboMoney(params[2],SYMB::WICC);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CMinerFeeProposal proposal ;
    proposal.tx_type = TxType(txType)  ;
    proposal.fee_symbol = feeInfo.symbol;
    proposal.fee_sawi_amount = feeInfo.GetSawiAmount();

    CProposalRequestTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol    = fee.symbol ;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CMinerFeeProposal>(proposal)) ;


    return SubmitTx(account.keyid, tx) ;

}

Value submitcointransferproposal( const Array& params, bool fHelp) {
    if(fHelp || params.size() < 4 || params.size() > 5){
        throw runtime_error(
                "submitcointransferproposal $tx_uid $from_uid $to_uid $transfer_amount [$fee]\n"
                "create proposal about enable/disable dexoperator\n"
                "\nArguments:\n"
                "1.$tx_uid:                (string, required) the submitor's address\n"
                "2.$from_uid:              (string, required) the address that transfer from\n"
                "3.$to_uid:                (string, required) the address that tranfer to \n"
                "4.$transfer_amount:       (combomoney, required) the tansfer amount\n"
                "5.$fee:                   (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitminerfeeproposal", "0-1 100-1 200-1 WICC:1000:wi WICC:0.001:wi")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitminerfeeproposal", "0-1 100-1 200-1 WICC:1000:wi WICC:0.001:wi")

        );
    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid    = RPC_PARAM::GetUserId(params[0], true);
    const CUserID& fromUid  = RPC_PARAM::GetUserId(params[1]) ;
    const CUserID& toUid    = RPC_PARAM::GetUserId(params[2]) ;

    ComboMoney transferInfo = RPC_PARAM::GetComboMoney(params[3],SYMB::WICC);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight     = chainActive.Height();

    auto pSymbolErr = pCdMan->pAssetCache->CheckTransferCoinSymbol(transferInfo.symbol);
    if (pSymbolErr)
        throw JSONRPCError(REJECT_INVALID, strprintf("Invalid coin symbol=%s! %s", transferInfo.symbol, *pSymbolErr));

    if (transferInfo.GetSawiAmount() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Coins is zero!");

    CAccount fromAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, fromUid);
    RPC_PARAM::CheckAccountBalance(fromAccount, transferInfo.symbol, SUB_FREE, transferInfo.GetSawiAmount());

    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CCoinTransferProposal proposal;
    proposal.from_uid   = fromUid;
    proposal.to_uid     = toUid;
    proposal.token      = transferInfo.symbol;
    proposal.amount     = transferInfo.GetSawiAmount();

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetSawiAmount();
    tx.fee_symbol   = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal     = CProposalStorageBean(std::make_shared<CCoinTransferProposal>(proposal)) ;

    return SubmitTx(txAccount.keyid, tx) ;

}

Value getsysparam(const Array& params, bool fHelp){
    if(fHelp || params.size() > 1){
        throw runtime_error(
                "getsysparam $param_name\n"
                "get system param info\n"
                "\nArguments:\n"
                "1.$param_name:      (string, optional) param name, list all parameters when omitted \n"

                "\nExamples:\n"
                + HelpExampleCli("getsysparam", "")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getsysparam", "")
        );
    }

    if (params.size() == 1) {
        string paramName = params[0].get_str() ;
        SysParamType st ;
        auto itr = paramNameToSysParamTypeMap.find(paramName) ;
        if (itr == paramNameToSysParamTypeMap.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "param name is illegal");

        st = std::get<1>(itr->second) ;
        uint64_t pv ;
        if(!pCdMan->pSysParamCache->GetParam(st, pv))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "get param error");

        Object obj ;
        obj.push_back(Pair(paramName, pv));
        return obj;
    } else {
        Object obj;
        for(auto kv : paramNameToSysParamTypeMap) {
            auto paramName = kv.first ;
            uint64_t pv = 0;
            pCdMan->pSysParamCache->GetParam(std::get<1>(kv.second), pv);

            obj.push_back(Pair(paramName, pv)) ;

        }
        return obj ;
    }
}


Value getcdpparam(const Array& params, bool fHelp) {
    if(fHelp || params.size() > 1){
        throw runtime_error(
                "getcdpparam $bcoin_scoin_pair $param_name \n"
                "get its param info about a given CDP type by its coinpair key\n"
                "\nArguments:\n"
                "1.$bcoin_scoin_pair: (string,required) a CDP type denoted by boin:scoin symbol pair\n"
                "2.$param_name:       (string, optional)a param name. list all parameters when omitted\n"

                "\nExamples:\n"
                + HelpExampleCli("getcdpparam", "WICC:WUSD")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getcdpparam", "WICC:WUSD")
        );
    }

    string strBCoinScoin = params[0].get_str();
    auto vCoinPair = split(strBCoinScoin, ":");
    if (vCoinPair.size() != 2)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "ill-formatted CDP coin-pair: " + strBCoinScoin);

    CCdpCoinPair coinPair = CCdpCoinPair(vCoinPair[0], vCoinPair[1]);

    if (params.size() == 2) {
        string paramName = params[1].get_str() ;
        CdpParamType cpt ;
        auto itr = paramNameToCdpParamTypeMap.find(paramName) ;
        if( itr == paramNameToCdpParamTypeMap.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "param name is illegal");

        cpt = std::get<1>(itr->second) ;

        uint64_t pv ;
        if(!pCdMan->pSysParamCache->GetCdpParam(coinPair,cpt, pv)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "get param error or coin pair error");
        }

        Object obj ;
        obj.push_back(Pair(paramName, pv));
        return obj;
    } else {
        Object obj;
        for(auto kv:paramNameToCdpParamTypeMap){
            auto paramName = kv.first ;
            uint64_t pv ;
            pCdMan->pSysParamCache->GetCdpParam(coinPair,std::get<1>(kv.second), pv);
            obj.push_back(Pair(paramName, pv)) ;
        }
        return obj ;
    }
}

Value listmintxfees(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0){
        throw runtime_error(
                "listmintxfees\n"
                "\nget all tx minimum fee.\n"
                "\nExamples:\n" +
                HelpExampleCli("listmintxfees", "") + "\nAs json rpc\n" + HelpExampleRpc("listmintxfees", ""));
    }

    Array arr;
    for(auto kv: kTxFeeTable){
        Object o;
        o.push_back(Pair("txtype_name", std::get<0>(kv.second)));
        o.push_back(Pair("txtype_code", kv.first));
        uint64_t feeOut;
        if(GetTxMinFee(kv.first,chainActive.Height(), SYMB::WICC,feeOut))
            o.push_back(Pair("minfee_in_wicc", feeOut));

        if(GetTxMinFee(kv.first,chainActive.Height(), SYMB::WUSD,feeOut))
            o.push_back(Pair("minfee_in_wusd", feeOut));

        o.push_back(Pair("modifiable", std::get<5>(kv.second)));
        arr.push_back(o);
    }

    return arr ;
}

Value getdexquotecoins(const Array& params, bool fHelp) {

    if(fHelp || params.size() !=0){
        throw runtime_error("") ;
    }

    set<TokenSymbol> coins ;
    pCdMan->pDexCache->GetDexQuoteCoins(coins) ;

    Object o ;
    Array arr ;
    for(TokenSymbol token: coins)
        arr.push_back(token) ;
    o.push_back(Pair("dex_quote_coins", arr)) ;
    return o ;


}


Value getbpcount(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0 ){
        throw runtime_error("") ;
    }
    auto co =  pCdMan->pSysParamCache->GetBpCount(chainActive.Height());
    Object o ;
    o.push_back(Pair("bp_count", co)) ;
    return o;

}