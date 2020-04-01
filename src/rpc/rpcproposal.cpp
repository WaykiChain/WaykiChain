// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
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
                + HelpExampleRpc("getproposal", "\"02sov0efs3ewdsxcfresdfdsadfgdsasdfdsadfdsdfsdfsddfge32ewsrewsowekdsx\"")

        );
    }
    uint256 proposalId = uint256S(params[0].get_str());
    std::shared_ptr<CProposal> pp;
    if(pCdMan->pSysGovernCache->GetProposal(proposalId, pp)){
        auto proposalObject  = pp->ToJson();
        vector<CRegID> approvalList;
        pCdMan->pSysGovernCache->GetApprovalList(proposalId, approvalList);

        proposalObject.push_back(Pair("approvaled_count", (uint64_t)approvalList.size()));

            Array a;
            for ( CRegID i: approvalList) {
              a.push_back(i.ToString());
            }
        proposalObject.push_back(Pair("approvaled_governors", a));

        return proposalObject;

    }
    return Object();
}

Value getswapcoindetail(const Array& params, bool fHelp) {

    if(fHelp || params.size() != 1){
        throw runtime_error(
                "getswapcoin \"peer_chain_coin_symbol\"\n"
                "\n"
                "\nArguments:\n"
                "1.\"peer_chain_coin_symbol\": (string,     required) the coin symbol of peer chain\n"
                "\nExamples:\n"
                + HelpExampleCli("getswapcoin", "BTC")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getswapcoin", R"("BTC")")

        );
    }

    string peerSymbol = params[0].get_str();

    AxcSwapCoinPair p;
    if (!pCdMan->pAssetCache->GetAxcCoinPairByPeerSymbol(TokenSymbol(peerSymbol), p)){
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("don't find swap coin detail by peer coin (%s)",peerSymbol));
    }

    return p.ToJson();
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


    Object o;
    set<CRegID> governors;
    pCdMan->pSysGovernCache->GetGovernors(governors);
    Array arr;
    for(auto id: governors) {
        arr.push_back(id.ToString());
    }
    o.push_back(Pair("governors", arr));

    return o;

}


Value submitparamgovernproposal(const Array& params, bool fHelp){

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitparamgovernproposal \"addr\" \"param_name\" \"param_value\" [\"fee\"]\n"
                "create proposal about system param govern\n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"param_name\":       (string,     required) the name of param, the param list can be found in document \n"
                "3.\"param_value\":      (numberic,   required) the param value that will be updated to \n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitparamgovernproposal", "0-1 ASSET_ISSUE_FEE  10000 WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitparamgovernproposal", R"("0-1","ASSET_ISSUE_FEE"  10000 "WICC:1:WI")")

                );

    }


    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string paramName = params[1].get_str();
    uint64_t paramValue = RPC_PARAM::GetUint64(params[2]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovSysParamProposal proposal;


    SysParamType  type = GetSysParamType(paramName);
    if(type == SysParamType::NULL_SYS_PARAM_TYPE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("system param type(%s) is not exist",paramName));

    string errorInfo = CheckSysParamValue(type, paramValue );
    if(errorInfo != EMPTY_STRING)
        throw JSONRPCError(RPC_INVALID_PARAMETER, errorInfo);

    proposal.param_values.push_back(std::make_pair(type, paramValue));

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovSysParamProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}


Value submitcdpparamgovernproposal(const Array& params, bool fHelp){

    if(fHelp || params.size() < 5 || params.size() > 6){

        throw runtime_error(
                "submitcdpparamgovernproposal \"addr\" \"param_name\" \"param_value\" \"bcoin_symbol\" \"scoin_symbol\" [\"fee\"]\n"
                "create proposal about cdp  param govern\n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"param_name\":       (string,     required) the name of param, the param list can be found in document \n"
                "3.\"param_value\":      (numberic,   required) the param value that will be updated to \n"
                "4.\"bcoin_symbol\":     (string,     required) the base coin symbol\n"
                "5.\"scoin_symbol\":     (string,     required) the stable coin symbol\n"
                "6.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitcdpparamgovernproposal", "0-1 CDP_INTEREST_PARAM_A  10000 WICC WUSD WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitcdpparamgovernproposal",
                                 R"("0-1", "CDP_INTEREST_PARAM_A",  "10000", "WICC", "WUSD", "WICC:1:WI")")

        );

    }


    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string paramName = params[1].get_str();
    uint64_t paramValue = RPC_PARAM::GetUint64(params[2]);
    string bcoinSymbol = params[3].get_str();
    string scoinSymbol = params[4].get_str();
    ComboMoney fee          = RPC_PARAM::GetFee(params, 5, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CdpParamType  type = GetCdpParamType(paramName);
    if(type == CdpParamType::NULL_CDP_PARAM_TYPE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("system param type(%s) is not exist",paramName));

    string errMsg;
    if(!CheckCdpParamValue(type, paramValue, errMsg))
        throw JSONRPCError(RPC_INVALID_PARAMETER, errMsg);

    CGovCdpParamProposal proposal;
    proposal.param_values.push_back(std::make_pair(type, paramValue));
    proposal.coin_pair = CCdpCoinPair(bcoinSymbol, scoinSymbol);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovCdpParamProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}

Value submitgovernorupdateproposal(const Array& params , bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitgovernorupdateproposal \"addr\" \"governor_uid\" \"operate_type\" [\"fee\"]\n"
                "create proposal about  add/remove a governor \n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"governor_uid\":     (string,     required) the governor's uid\n"
                "3.\"operate_type\":     (numberic,   required) the operate type \n"
                "                         1 stand for add\n"
                "                         2 stand for remove\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitgovernorupdateproposal", "0-1 100-2 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitgovernorupdateproposal", R"("0-1", "100-2", 1,  "WICC:1:WI")")

        );

    }

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    CUserID governorId = RPC_PARAM::GetUserId(params[1]);
    if(!governorId.is<CRegID>())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the price governor(%s) must be registered(have regid)",
                                                            governorId.ToString() ));
    uint64_t operateType = RPC_PARAM::GetUint64(params[2]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovBpMcListProposal proposal;
    proposal.gov_bp_regid = governorId.get<CRegID>();
    proposal.op_type = ProposalOperateType(operateType);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovBpMcListProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}


bool ParsePerms(const Array& permsArr, uint64_t& permSum) {

    vector<pair<uint8_t, uint8_t>> vPerms;

    if(permsArr.size() == 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "the perm array must be not empty");
    }
    for(auto obj:permsArr){
        const Value& permCodeObj = JSON::GetObjectFieldValue(obj,"perm_code");
        uint8_t permCode = AmountToRawValue(permCodeObj);
        const Value& opTypeObj = JSON::GetObjectFieldValue(obj,"op_type");
        uint8_t opType = AmountToRawValue(opTypeObj);
        if (opType !=0 && opType != 1) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "op type is error, it must be 0 or 1");
        }
        if ( permCode > 63)
            throw  JSONRPCError(RPC_INVALID_PARAMETER, " perm code is error, it must be less than 64");

        vPerms.push_back(make_pair(permCode, opType));

    }

    for(auto p: vPerms){
        if (p.second == 0) {
            permSum = permSum & (~(1 << p.first));
        }
        if (p.second == 1) {
            permSum = permSum | (1 << p.first);
        }
    }

    return true;

}

Value submitaccountpermproposal(const Array& params , bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitaccountpermproposal \"addr\" \"account_uid\" \"proposed_perms_sum\" [\"fee\"]\n"
                "create proposal about  updating account's permits \n"
                "\nArguments:\n"
                "1.\"addr\":               (string,     required) the tx submitor's address\n"
                "2.\"account_uid\":        (string,     required) the account uid that need to update\n"
                "3.\"proposed_perms_sum\": (jsonArray,  required) the proposed perms update iterm\n"
                "                          [\n"
                "                            {\n"
                "                              \"perm_code\": (numberic,required) the account perms code[see document]\n"
                "                              \"op_type\"  : (numberic,required) the operate type ,0 stand for revoke, 1 stand for grant\n"
                "                            }\n"
                "                          ]\n"
                "4.\"fee\":                (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitaccountpermproposal", R"(0-1 100-2 "[{"perm_code":3, "op_type": 0}]"  WICC:1:WI)")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitaccountpermproposal", R"("0-1", "100-2", "[{"perm_code":3, "op_type": 0}]",  "WICC:1:WI")")

        );

    }

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    CUserID accountUid = RPC_PARAM::GetUserId(params[1]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());


    Array permsArr = params[2].get_array();
    CAccount targetAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, accountUid);
    uint64_t permSum = targetAccount.perms_sum;

    ParsePerms(permsArr, permSum);

    CGovAccountPermProposal proposal(accountUid,permSum);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovAccountPermProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}


Value submitassetpermproposal(const Array& params , bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitassetpermproposal \"addr\" \"asset_symbol\" \"proposed_perms_sum\" [\"fee\"]\n"
                "create proposal about  updating asset permits \n"
                "\nArguments:\n"
                "1.\"addr\":               (string,     required) the tx submitor's address\n"
                "2.\"asset_symbol\":       (string,     required) the asset that need to update\n"
                "3.\"proposed_perms_sum\": (jsonArray,  required) the proposed perms update iterm\n"
                "                          [\n"
                "                            {\n"
                "                              \"perm_code\": (numberic,required) the asset perms code[see document]\n"
                "                              \"op_type\"  : (numberic,required) the operate type ,0 stand for revoke, 1 stand for grant\n"
                "                            }\n"
                "                          ]\n"
                "4.\"fee\":                (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitassetpermproposal", "0-1 WICC 3  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitassetpermproposal", R"("0-1", "WICC", 1,  "WICC:1:WI")")

        );

    }

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    const string assetSymbol = params[1].get_str();
    Array permsArr = params[2].get_array();
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CAsset asset;

    if(pCdMan->pAssetCache->GetAsset(assetSymbol, asset)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("not find asset that named %s", assetSymbol));
    }

    uint64_t permSum = asset.perms_sum;
    ParsePerms(permsArr, permSum);
    CGovAssetPermProposal proposal;
    proposal.asset_symbol = assetSymbol;
    proposal.proposed_perms_sum = permSum;
    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovAssetPermProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}

Value submitfeedcoinpairproposal(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
                "submitfeedcoinpairproposal \"addr\" \"base_symbol\" \"quote_symbol\" \"operate_type\" [\"fee\"]\n"
                "request proposal about add/remove feed price coin pair \n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"base_symbol\":      (string,     required) the base symbol\n"
                "3.\"quote_symbol\":      (string,     required) the quote symbol\n"
                "4.\"op_type\":          (numberic,   required) the operate type \n"
                "                         1 stand for add\n"
                "                         2 stand for remove\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitfeedcoinpairproposal", "0-1 WICC WUSD 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitfeedcoinpairproposal", R"("0-1", "WICC", "WUSD", 1,  "WICC:1:WI")")

        );
    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string baseSymbol = params[1].get_str();
    string quoteSymbol = params[2].get_str();
    uint64_t operateType = RPC_PARAM::GetUint64(params[3]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 4, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovFeedCoinPairProposal proposal;
    proposal.base_symbol = baseSymbol;
    proposal.quote_symbol = quoteSymbol;
    proposal.op_type = ProposalOperateType(operateType);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovFeedCoinPairProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}

Value submitaxccoinproposal(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 4 || params.size() > 5){
        throw runtime_error(
                "submitaxccoinproposal \"addr\" \"peer_coin_symbol\" \"peer_chain_type\" \"operate_type\" [\"fee\"]\n"
                "create a proposal about add or delete a swap coin\n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"peer_coin_symbol\": (string,     required) the peer chain coin symbol\n"
                "3.\"peer_chain_type\":  (numberic,   required) the peer chain type\n"
                "                          1 stand for BITCOIN\n"
                "                          2 stand for ETHEREUM\n"
                "                          3 stand for EOS\n"
                "4.\"operate_type\":     (numberic,   required) the operate type \n"
                "                          1 stand for enable\n"
                "                          2 stand for disable\n"
                "5.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitaxccoinproposal", "0-1 BTC 1 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitaxccoinproposal", R"("0-1", BTC, 1 ,1, "WICC:1:WI")")

        );

    }


    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    string peerCoinSymbol = params[1].get_str();
    int chainType = RPC_PARAM::GetUint64(params[2]);
    int opType = params[3].get_int();
    ComboMoney fee = RPC_PARAM::GetFee(params, 4, PROPOSAL_REQUEST_TX);
    int32_t validHeight = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());


    CGovAxcCoinProposal proposal;
    proposal.peer_chain_coin_symbol = peerCoinSymbol;
    proposal.peer_chain_type = ChainType(chainType);
    proposal.op_type = ProposalOperateType(opType);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovAxcCoinProposal>(proposal));
    return SubmitTx(account.keyid, tx);

}

Value submitdexswitchproposal(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitdexswitchproposal \"addr\" \"dexid\" \"operate_type\" [\"fee\"]\n"
                "create proposal about enable/disable dexoperator\n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"dexid\":            (numberic,   required) the dexoperator's id\n"
                "3.\"operate_type\":     (numberic,   required) the operate type \n"
                "                          1 stand for enable\n"
                "                          2 stand for disable\n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitdexswitchproposal", "0-1 1 1  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitdexswitchproposal", R"("0-1", 1 ,1, "WICC:1:WI")")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint64_t dexId = RPC_PARAM::GetUint64(params[1]);
    uint64_t operateType = RPC_PARAM::GetUint64(params[2]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovDexOpProposal proposal;
    proposal.dexid = dexId;
    proposal.operate_type = ProposalOperateType(operateType);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovDexOpProposal>(proposal));
    return SubmitTx(account.keyid, tx);
}

Value submitproposalapprovaltx(const Array& params, bool fHelp){

    if(fHelp || params.size() < 2 || params.size() > 4){
        throw runtime_error(
                "submitproposalapprovaltx \"addr\" \"proposalid\" [\"fee\"] [\"axc_out_signature\"]\n"
                "approval a proposal\n"
                "\nArguments:\n"
                "1.\"addr\":             (string,   required) the tx submitor's address\n"
                "2.\"proposalid\":       (string,   required) the proposal's id\n"
                "3.\"fee\":              (combomoney, optional) the tx fee \n"
                "4.\"axc_out_signature   (string, optional) the axc out proposal peer chain signature，is "
                                         "required when proposal type is GOV_AXC_OUT(13) \n"
                "\nExamples:\n"
                + HelpExampleCli("submitproposalapprovaltx", "0-1 2390ewdosd0wfsdi0wfwefweiojwofwe0fw212wasewd  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitproposalapprovaltx", R"("0-1", "2390ewdosd0wfsdi0wfwefweiojwofwe0fw212wasewd", 1, "WICC:1:WI")")

        );
    }


    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint256 proposalId   = uint256S(params[1].get_str()) ;
    ComboMoney fee       = RPC_PARAM::GetFee(params, 2, PROPOSAL_APPROVAL_TX);
    UnsignedCharArray axcOutSignature;
    if (params.size() > 3) {
        axcOutSignature = ParseHex(params[3].get_str());
    }

    int32_t validHegiht  = chainActive.Height();
    CAccount account     = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());






    CProposalApprovalTx tx ;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHegiht;
    tx.proposal_id = proposalId;
    tx.axc_signature = axcOutSignature;
    return SubmitTx(account.keyid, tx);

}

Value submittotalbpssizeupdateproposal(const Array& params,bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4){
        throw runtime_error(
                "submittotalbpssizeupdateproposal \"addr\" \"total_bps_size\" \"effective_height\"  [\"fee\"]\n"
                "create proposal about update total delegate(bp) count\n"
                "\nArguments:\n"
                "1.\"addr\":                (string,     required) the tx submitor's address\n"
                "2.\"total_bps_size\":      (numberic,   required) the count of block producer(miner) range is [1,255] \n"
                "3.\"effective_height\":    (numberic,   required) the effective height of the totalbpssize  \n"
                "4.\"fee\":                 (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submittotalbpssizeupdateproposal", "0-1 21 450020202  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submittotalbpssizeupdateproposal", R"("0-1", 21, 433222223, "WICC:1:WI")")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true );
    uint32_t totalBpsSize = RPC_PARAM::GetUint32(params[1]);
    uint32_t effectiveHeight = RPC_PARAM::GetUint32(params[2]);
    ComboMoney fee = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    if(totalBpsSize <=0 || totalBpsSize > BP_MAX_COUNT)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the range of total_bps_size is (0,%d]", BP_MAX_COUNT));

    CGovBpSizeProposal proposal;
    proposal.total_bps_size = totalBpsSize;
    proposal.effective_height = effectiveHeight;

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovBpSizeProposal>(proposal));

    return SubmitTx(account.keyid, tx);


}
Value submitminerfeeproposal(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 3 || params.size() > 4){

        throw runtime_error(
                "submitminerfeeproposal \"addr\" \"tx_type\" \"fee_info\"  [\"fee\"]\n"
                "create proposal about updating the min miner fee\n"
                "\nArguments:\n"
                "1.\"addr\":             (string,     required) the tx submitor's address\n"
                "2.\"tx_type\":          (numberic,   required) the tx type you can get the list by command \"listmintxfees\" \n"
                "3.\"fee_info\":         (combomoney, required) the miner fee symbol,amount,unit, example:WICC:10000:SAWI \n"
                "4.\"fee\":              (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitminerfeeproposal", "0-1 1 WICC:1:WI  WICC:1:WI")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitminerfeeproposal", R"("0-1", 1, "WICC:1:WI", "WICC:1:WI")")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    uint8_t txType = params[1].get_int();
    ComboMoney feeInfo = RPC_PARAM::GetComboMoney(params[2],SYMB::WICC);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 3, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovMinerFeeProposal proposal;
    proposal.tx_type = TxType(txType);
    proposal.fee_symbol = feeInfo.symbol;
    proposal.fee_sawi_amount = feeInfo.GetAmountInSawi();

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovMinerFeeProposal>(proposal));


    return SubmitTx(account.keyid, tx);

}


Value submitaxcinproposal(const Array& params, bool fHelp) {


    if(fHelp || params.size() < 6 || params.size() > 7){

        throw runtime_error(
                "submitaxcinproposal \"addr\" \"peer_chain_type\" \"peer_chain_token_symbol\" \"self_chain_token_symbol\" \"peer_chain_addr\""
                " \"peer_chain_txid\" \"self_chain_uid\" \"swap_amount\" [\"fee\"]\n"
                "create proposal about transfer coin from other chain to waykichain \n"
                "\nArguments:\n"
                "1.\"addr\":                        (string, required) the tx submitor's address\n"
                "2.\"peer_chain_token_symbol\":     (string, required) the coin symbol that swap from, such as BTC,ETH,EOS \n"
                "3.\"peer_chain_addr\":             (string, required) initiator's address at peer chain \n"
                "4.\"peer_chain_txid\":             (string, required) a proof from the peer chain (non-HTLC version), such as wisvisof932wq392wospal230ewopdsxl\n"
                "5.\"self_chain_uid\":              (string, required) initiator's uid at waykichain \n"
                "6.\"swap_amount\":                 (numberic, required) the coin amount that swap in, the unit is sa(0.00000001), \n"
                "7.\"fee\":                         (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitaxcinproposal", " 0-1 2 ETH  29okf0efodfredfedsedsfdscsfds ewsdcxesasdsadfsad 0-1  1000000")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitaxcinproposal", R"("0-1", "ETH", "29okf0efodfredfedsedsfdscsfds", "ewsdcxesasdsadfsad", "0-1", 1000000)")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    TokenSymbol peerTokenSymbol = TokenSymbol(params[1].get_str());
    string peerAddr = params[2].get_str();
    string peerTxid =params[3].get_str();
    CUserID selfUid = RPC_PARAM::GetUserId(params[4]);
    uint64_t swapCoinAmount = RPC_PARAM::GetUint64(params[5]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 6, PROPOSAL_REQUEST_TX);
    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovAxcInProposal proposal(peerTokenSymbol,peerAddr,peerTxid,selfUid,swapCoinAmount);
    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovAxcInProposal>(proposal));


    return SubmitTx(account.keyid, tx);

}

Value submitaxcoutproposal(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 4 || params.size() > 5){

        throw runtime_error(
                "submitaxcoutproposal \"addr\" \"self_chain_token_symbol\" \"peer_chain_addr\" \"swap_amount\" [\"fee\"]\n"
                "create proposal about transfer coins from waykichain to other chain\n"
                "\nArguments:\n"
                "1.\"addr\":                    (string,   required) the tx submitor's address\n"
                "2.\"self_chain_token_symbol\": (string, required) the coin symbol that swap out, such as WBTC,WETC,WEOS \n"
                "3.\"peer_chain_addr\":         (string, optional) initiator's address at peer chain \n"
                "4.\"swap_amount\":             (numberic,   required) the coin amount that swap out \n"
                "5.\"fee\":                     (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitaxcoutproposal", "0-1 mETH sfdv9efkwdscokedscsx 100000")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitaxcoutproposal", R"("0-1", "mETH", "sfdv9efkwdscokedscsx", 100000)")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    TokenSymbol selfChainTokenSymbol(params[1].get_str());
    string peerAddr = params[2].get_str();
    uint64_t swapCoinAmount = RPC_PARAM::GetUint64(params[3]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 4, PROPOSAL_REQUEST_TX);

    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovAxcOutProposal proposal(selfChainTokenSymbol,peerAddr,swapCoinAmount);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovAxcOutProposal>(proposal));


    return SubmitTx(account.keyid, tx);

}


Value submitdiaissueproposal(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 4 || params.size() > 5){

        throw runtime_error(
                "submitdiaissueproposal \"addr\"  \"asset_symbol\" \"owner_uid\" \"total_supply\" [\"fee\"]\n"
                "issue a dia asset\n"
                "\nArguments:\n"
                "1.\"addr\":                    (string,   required) the tx submitor's address\n"
                "2.\"asset_symbol\":            (string,   required) the dia asset symbol \n"
                "3.\"owner_uid\":               (string,   required) the asset's owner uid \n"
                "4.\"total_supply\":            (numberic, required) the total supply amount of this asset, the unit is \"sawi\" \n"
                "5.\"fee\":                     (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitdiaissueproposal", "0-1 0-1 BTCC 100000000000000000")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitdiaissueproposal", R"("0-1", "0-1", "BTCC", 10000000000000000)")

        );

    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    TokenSymbol assetSymbol(params[1].get_str());
    const CUserID& ownerUid = RPC_PARAM::GetUserId(params[2]);
    uint64_t totalSupply = RPC_PARAM::GetUint64(params[3]);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 4, PROPOSAL_REQUEST_TX);

    int32_t validHeight  = chainActive.Height();
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CAccount ownerAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, ownerUid);

    CGovDiaIssueProposal proposal(assetSymbol, totalSupply, ownerUid);

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol    = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal = CProposalStorageBean(std::make_shared<CGovDiaIssueProposal>(proposal));


    return SubmitTx(account.keyid, tx);

}

Value submitcointransferproposal( const Array& params, bool fHelp) {
    if(fHelp || params.size() < 4 || params.size() > 5){
        throw runtime_error(
                "submitcointransferproposal $tx_uid $from_uid $to_uid $transfer_amount [$fee]\n"
                "create proposal about trans coin from an account to another account\n"
                "\nArguments:\n"
                "1.$tx_uid:                (string,     required) the submitor's address\n"
                "2.$from_uid:              (string,     required) the address that transfer from\n"
                "3.$to_uid:                (string,     required) the address that tranfer to \n"
                "4.$transfer_amount:       (combomoney, required) the tansfer amount\n"
                "5.$fee:                   (combomoney, optional) the tx fee \n"
                "\nExamples:\n"
                + HelpExampleCli("submitcointransferproposal", "0-1 100-1 200-1 WICC:1000:wi WICC:0.001:wi")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitcointransferproposal", R"("0-1", "100-1", "200-1", "WICC:1000:wi", "WICC:0.001:wi")")

        );
    }

    EnsureWalletIsUnlocked();
    const CUserID& txUid    = RPC_PARAM::GetUserId(params[0], true);
    const CUserID& fromUid  = RPC_PARAM::GetUserId(params[1]);
    const CUserID& toUid    = RPC_PARAM::GetUserId(params[2]);

    ComboMoney transferInfo = RPC_PARAM::GetComboMoney(params[3],SYMB::WICC);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 4, PROPOSAL_REQUEST_TX);
    int32_t validHeight     = chainActive.Height();

    if (!pCdMan->pAssetCache->CheckAsset(transferInfo.symbol))
        throw JSONRPCError(REJECT_INVALID, strprintf("Invalid coin symbol=%s!", transferInfo.symbol));

    if (transferInfo.GetAmountInSawi() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Coins is zero!");

    CAccount fromAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, fromUid);
    RPC_PARAM::CheckAccountBalance(fromAccount, transferInfo.symbol, SUB_FREE, transferInfo.GetAmountInSawi());

    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CGovCoinTransferProposal proposal;
    proposal.from_uid   = fromUid;
    proposal.to_uid     = toUid;
    proposal.token      = transferInfo.symbol;
    proposal.amount     = transferInfo.GetAmountInSawi();

    CProposalRequestTx tx;
    tx.txUid        = txUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.fee_symbol   = fee.symbol;
    tx.valid_height = validHeight;
    tx.proposal     = CProposalStorageBean(std::make_shared<CGovCoinTransferProposal>(proposal));

    return SubmitTx(txAccount.keyid, tx);

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
        string paramName = params[0].get_str();
        SysParamType st;
        auto itr = paramNameToSysParamTypeMap.find(paramName);
        if (itr == paramNameToSysParamTypeMap.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "param name is illegal");

        st = itr->second;
        uint64_t pv;
        if(!pCdMan->pSysParamCache->GetParam(st, pv))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "get param error");

        Object obj;
        if(itr->second == SysParamType::AXC_SWAP_GATEWAY_REGID){
            obj.push_back(Pair(paramName, CRegID(pv).ToString()));
        } else if (itr->second == SysParamType::DEX_MATCH_SVC_REGID) {
            obj.push_back(Pair(paramName, pCdMan->pSysParamCache->GetDexMatchSvcRegId().ToString()));
        } else {
            obj.push_back(Pair(paramName, pv));
        }

        return obj;

    } else {
        Object obj;
        for(auto kv : paramNameToSysParamTypeMap) {
            auto paramName = kv.first;
            uint64_t pv = 0;
            pCdMan->pSysParamCache->GetParam(kv.second, pv);

            if(kv.second == SysParamType::AXC_SWAP_GATEWAY_REGID){
                obj.push_back(Pair(paramName, CRegID(pv).ToString()));
            }
            else if (kv.second == SysParamType::DEX_MATCH_SVC_REGID) {
                obj.push_back(Pair(paramName, pCdMan->pSysParamCache->GetDexMatchSvcRegId().ToString()));
            } else {
                obj.push_back(Pair(paramName, pv));
            }


        }
        return obj;
    }
}

Value getcdpparam(const Array& params, bool fHelp) {
    if(fHelp || params.size() < 1 || params.size() > 2){
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
        string paramName = params[1].get_str();
        CdpParamType cpt;
        auto itr = paramNameToCdpParamTypeMap.find(paramName);
        if( itr == paramNameToCdpParamTypeMap.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "param name is illegal");

        cpt = itr->second;

        uint64_t pv;
        if(!pCdMan->pSysParamCache->GetCdpParam(coinPair,cpt, pv)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "get param error or coin pair error");
        }

        Object obj;
        obj.push_back(Pair(paramName, pv));
        return obj;
    } else {
        Object obj;
        for(auto kv:paramNameToCdpParamTypeMap){
            auto paramName = kv.first;
            uint64_t pv = 0;
            pCdMan->pSysParamCache->GetCdpParam(coinPair,kv.second, pv);
            obj.push_back(Pair(paramName, pv));
        }
        return obj;
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

    return arr;
}

Value getdexbaseandquotecoins(const Array& params, bool fHelp) {

    if(fHelp || params.size() !=0){
        throw runtime_error(
                "getdexbaseandquotecoins\n"
                "\nget all dex base coins and quote coins.\n"
                "\nExamples:\n" +
                HelpExampleCli("getdexbaseandquotecoins", "") + "\nAs json rpc\n" + HelpExampleRpc("getdexbaseandquotecoins", ""));
    }
    vector<pair<AssetPermType, string>> vPerms = { pair(AssetPermType::PERM_DEX_BASE, "dex_base_coins"),
                                                  pair(AssetPermType::PERM_DEX_QUOTE, "dex_quote_coins") };
    Object obj;
    for( auto perm: vPerms) {
        set<TokenSymbol> quoteSymbolSet;
        pCdMan->pAssetCache->GetAssetTokensByPerm(perm.first, quoteSymbolSet);
        Array arr;
        for(TokenSymbol token: quoteSymbolSet)
            arr.push_back(token);
        obj.push_back(Pair(perm.second, arr));
    }

    return obj;
}


Value gettotalbpssize(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0 ){
        throw runtime_error(
                "gettotalbpssize\n"
                "\nget the total size of bps(delegates).\n"
                "\nExamples:\n" +
                HelpExampleCli("gettotalbpssize", "") + "\nAs json rpc\n" + HelpExampleRpc("gettotalbpssize", ""));
    }
    auto co =  pCdMan->pSysParamCache->GetTotalBpsSize(chainActive.Height());
    Object o;
    o.push_back(Pair("total_bps_size", co));
    return o;

}

Value getfeedcoinpairs(const Array& params, bool fHelp) {

    if(fHelp || params.size() != 0) {
        throw runtime_error(
                "getfeedcoinpairs\n"
                "\nget all price feed coin pairs.\n"
                "\nExamples:\n" +
                HelpExampleCli("getfeedcoinpairs", "") + "\nAs json rpc\n" + HelpExampleRpc("getfeedcoinpairs", ""));
    }

    set<pair<TokenSymbol, TokenSymbol>> feedPairs;
    pCdMan->pPriceFeedCache->GetFeedCoinPairs(feedPairs);

    Array feedPairArray;
    for (auto feedPair: feedPairs) {
        feedPairArray.push_back(strprintf("%s-%s",feedPair.first,feedPair.second));
    }
    Object o;
    o.push_back(Pair("feed_coin_pairs", feedPairArray));
    return o;

}