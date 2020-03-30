// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "tx/proposaltx.h"
#include "main.h"
#include "entities/proposal.h"
#include <algorithm>

bool CheckIsGovernor(CRegID account, ProposalType proposalType, CCacheWrapper& cw ){

   if(proposalType == ProposalType::GOV_BPMC_LIST
                || proposalType == ProposalType::GOV_COIN_TRANSFER
                || proposalType == ProposalType::GOV_BP_SIZE){
        VoteDelegateVector delegateList;
        if (!cw.delegateCache.GetActiveDelegates(delegateList)) {
            return false;
        }
        for(auto miner: delegateList){
            if(miner.regid == account)
                return true;
        }
        return false;

    } else{
        return cw.sysGovernCache.CheckIsGovernor(account);
    }

}

uint8_t GetGovernorApprovalMinCount(ProposalType proposalType, CCacheWrapper& cw) {

    if(proposalType == ProposalType::GOV_BPMC_LIST
       || proposalType == ProposalType::GOV_COIN_TRANSFER
       || proposalType == ProposalType::GOV_BP_SIZE){

        VoteDelegateVector delegateList;
        if (!cw.delegateCache.GetActiveDelegates(delegateList))
            return 8;
        return static_cast<uint8_t>(delegateList.size() - (delegateList.size() / 3));

    } else{
        return cw.sysGovernCache.GetGovernorApprovalMinCount();
    }

}


string CProposalRequestTx::ToString(CAccountDBCache &accountCache) {
    string proposalString = proposal.sp_proposal->ToString();
    return strprintf("txType=%s, hash=%s, ver=%d, %s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, proposalString, llFees,
                     txUid.ToString(), valid_height);
}          // logging usage

Object CProposalRequestTx::ToJson(const CAccountDBCache &accountCache) const {

    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("proposal", proposal.sp_proposal->ToJson()));

    return result;
}  // json-rpc usage

 bool CProposalRequestTx::CheckTx(CTxExecuteContext &context) {
     return proposal.sp_proposal->CheckProposal(context, *this);
 }


bool CProposalRequestTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (!txAccount.OperateBalance(fee_symbol, SUB_FREE, llFees, ReceiptCode::BLOCK_REWARD_TO_MINER, receipts))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, account has insufficient funds"),
                    UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, set account info error"),
                    WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    uint64_t expiryBlockCount;
    if(!cw.sysParamCache.GetParam(PROPOSAL_EXPIRE_BLOCK_COUNT, expiryBlockCount))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx,get proposal expire block count error"),
                    WRITE_ACCOUNT_FAIL, "get-expire-block-count-error");

    auto proposalToSave = proposal.sp_proposal->GetNewInstance();
    proposalToSave->expiry_block_height = context.height + expiryBlockCount;
    proposalToSave->approval_min_count = GetGovernorApprovalMinCount(proposal.sp_proposal->proposal_type, cw);


    if (!cw.sysGovernCache.SetProposal(GetHash(), proposalToSave))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::ExecuteTx, set proposal info error"),
                    WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");

    if (proposalToSave->proposal_type == ProposalType::GOV_AXC_OUT) {
        auto axcOutProposal = dynamic_cast<CGovAxcOutProposal&>(*proposalToSave);
        axcOutProposal.self_chain_uid = txUid;
        auto savedSp = axcOutProposal.GetNewInstance();
        if (!cw.sysGovernCache.SetProposal(GetHash(), savedSp))
            return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, set proposal info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
    }

    return true;
}

string CProposalApprovalTx::ToString(CAccountDBCache &accountCache) {

    return strprintf("txType=%s, hash=%s, ver=%d, proposalid=%s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, proposal_id.GetHex(), llFees,
                     txUid.ToString(), valid_height);
}

Object CProposalApprovalTx::ToJson(const CAccountDBCache &accountCache) const {

     Object result = CBaseTx::ToJson(accountCache);
     result.push_back(Pair("proposal_id", proposal_id.ToString()));
     if(axc_signature.size() > 0)
         result.push_back(Pair("axc_signatur", HexStr(axc_signature)));
     return result;
} // json-rpc usage

bool CProposalApprovalTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    shared_ptr<CProposal> proposal;
    if (!cw.sysGovernCache.GetProposal(proposal_id, proposal)) {
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, proposal(id=%s)  not found", proposal_id.ToString()),
                          READ_ACCOUNT_FAIL, "proposal-not-found");
    }

    if(!CheckIsGovernor(txUid.get<CRegID>(), proposal->proposal_type, cw)){
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, the tx commiter(%s) is not a governor", proposal_id.ToString()),
                          READ_ACCOUNT_FAIL, "permission-deney");
    }

    if (proposal->proposal_type == ProposalType::GOV_AXC_OUT &&
        axc_signature.size() < 64 ) {
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, AXC signature invalid"),
                          READ_ACCOUNT_FAIL, "axc-signature-invalid");
    }

    return true;
}

bool CProposalApprovalTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    shared_ptr<CProposal> spProposal;
    if (!cw.sysGovernCache.GetProposal(proposal_id, spProposal))
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::CheckTx, proposal(id=%s)  not found", proposal_id.ToString()),
                        WRITE_ACCOUNT_FAIL, "proposal-not-found");

    auto assentedCount = cw.sysGovernCache.GetApprovalCount(proposal_id);
    if (assentedCount >= spProposal->approval_min_count)
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, proposal executed already"),
                        WRITE_ACCOUNT_FAIL, "proposal-executed-already");

    if (spProposal->expiry_block_height < (uint32_t)context.height)
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, proposal(id=%s)  is expired", proposal_id.ToString()),
                        WRITE_ACCOUNT_FAIL, "proposal-expired");

    if (!cw.sysGovernCache.SetApproval(proposal_id, txUid.get<CRegID>()))
        return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, set proposal approval info error"),
                        WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");

    if ((assentedCount + 1 == spProposal->approval_min_count)) {

        if (!spProposal->CheckProposal(context, *this) || !spProposal->ExecuteProposal(context, *this)) {
            return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, proposal execute error"),
                             WRITE_ACCOUNT_FAIL, "proposal-execute-error");
        }
    }

    if (spProposal->proposal_type == ProposalType::GOV_AXC_OUT) {
        auto axcOutProposal = dynamic_cast<CGovAxcOutProposal&>(*spProposal);
        axcOutProposal.peer_chain_tx_multisigs.push_back(axc_signature);
        auto savedSp = axcOutProposal.GetNewInstance();
        if (!cw.sysGovernCache.SetProposal(proposal_id, savedSp))
            return state.DoS(100, ERRORMSG("CProposalApprovalTx::ExecuteTx, set proposal info error"),
                        WRITE_ACCOUNT_FAIL, "bad-write-proposaldb");
    }

    return true;
}