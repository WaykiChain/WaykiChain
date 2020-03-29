// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "config/txbase.h"
#include "config/const.h"
#include "entities/asset.h"
#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include "main.h"
#include "tx/proposaltx.h"

#include <algorithm>
#include <set>

extern bool CheckIsGovernor(CRegID account, ProposalType proposalType, CCacheWrapper& cw );
extern uint8_t GetGovernorApprovalMinCount(ProposalType proposalType, CCacheWrapper& cw );

bool CGovSysParamProposal::CheckProposal(CTxExecuteContext& context) {
     CValidationState &state = *context.pState;

     if (param_values.size() == 0 || param_values.size() > 50)
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, params list is empty"), REJECT_INVALID,
                        "params-empty");
     for (auto pa: param_values){
         if(SysParamTable.count(SysParamType(pa.first)) == 0){
             return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                       REJECT_INVALID, "params-error");
         }
         string errorInfo = CheckSysParamValue(SysParamType(pa.first), pa.second);

         if (errorInfo != EMPTY_STRING)
             return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx failed: %s ", errorInfo),
                     REJECT_INVALID, "params-range-error");
     }

     return true;
}
bool CGovSysParamProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw = *context.pCw;

    for( auto pa: param_values){
        auto itr = SysParamTable.find(SysParamType(pa.first));
        if (itr == SysParamTable.end())
            return false;

        if (!cw.sysParamCache.SetParam(SysParamType(pa.first), pa.second))
            return false;
    }

    return true;
}


bool CGovBpMcListProposal::CheckProposal(CTxExecuteContext& context){
    IMPLEMENT_DEFINE_CW_STATE

    if (op_type != ProposalOperateType::ENABLE && op_type != ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, operate type is illegal!"), REJECT_INVALID,
                         "operate_type-illegal");

    CAccount govBpAccount;
    if (!cw.accountCache.GetAccount(gov_bp_regid, govBpAccount))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, governor regid(%s) is not exist!", gov_bp_regid.ToString()), REJECT_INVALID,
                         "governor-not-exist");

    if (op_type == ProposalOperateType::DISABLE && !cw.sysGovernCache.CheckIsGovernor(gov_bp_regid))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, regid(%s) is not a governor!", gov_bp_regid.ToString()), REJECT_INVALID,
                         "regid-not-governor");

    if (op_type == ProposalOperateType::ENABLE && cw.sysGovernCache.CheckIsGovernor(gov_bp_regid))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, regid(%s) is a governor already!", gov_bp_regid.ToString()), REJECT_INVALID,
                         "regid-is-governor");

    return true;
}

bool CGovBpMcListProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw       = *context.pCw;

    switch (op_type) {
        case ProposalOperateType::DISABLE : return cw.sysGovernCache.EraseGovernor(gov_bp_regid);
        case ProposalOperateType::ENABLE :  return cw.sysGovernCache.AddGovernor(gov_bp_regid);
        default: break;
    }

    return false;
}


bool CGovBpSizeProposal:: CheckProposal(CTxExecuteContext& context) {
    CValidationState &state = *context.pState;;

    if (total_bps_size == 0) //total_bps_size > BP_MAX_COUNT: always false
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::CheckProposal, total_bps_size must be between 1 and 255"),
                         REJECT_INVALID,"bad-bp-count");


    if (effective_height < (uint32_t) context.height + BPSSIZE_EFFECTIVE_AFTER_BLOCK_COUNT)
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::CheckProposal: effective_height must be >= current height + %d", BPSSIZE_EFFECTIVE_AFTER_BLOCK_COUNT),
                         REJECT_INVALID,"bad-effective-height");

    return true;
}

bool CGovBpSizeProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    auto currentTotalBpsSize = cw.sysParamCache.GetTotalBpsSize(context.height);
    if (!cw.sysParamCache.SetCurrentTotalBpsSize(currentTotalBpsSize)) {
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::ExecuteProposal, save current bp count failed!"),
                REJECT_INVALID, "save-currtotalbpssize-failed");
    }

    if (!cw.sysParamCache.SetNewTotalBpsSize(total_bps_size, effective_height)) {
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::ExecuteProposal, save new bp count failed!"),
                REJECT_INVALID, "save-newtotalbpssize-failed");
    }

    return true;

}

bool CGovMinerFeeProposal:: CheckProposal(CTxExecuteContext& context) {
    CValidationState& state = *context.pState;

    if (!kFeeSymbolSet.count(fee_symbol)) {
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, fee symbol(%s) is invalid!", fee_symbol),
                        REJECT_INVALID,
                        "feesymbol-error");
    }

    auto itr = kTxFeeTable.find(tx_type);
    if (itr == kTxFeeTable.end()){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, the tx type (%d) is invalid!", tx_type),
                        REJECT_INVALID,
                        "txtype-error");
    }

    if (!std::get<5>(itr->second)){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, the tx type (%d) miner fee can't be updated!", tx_type),
                        REJECT_INVALID,
                        "can-not-update");
    }

    if (fee_sawi_amount == 0 ){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, the tx type (%d) miner fee can't be zero", tx_type),
                        REJECT_INVALID,
                        "can-not-be-zero");
    }
    return true;
}

bool CGovMinerFeeProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw       = *context.pCw;
    return cw.sysParamCache.SetMinerFee(tx_type, fee_symbol, fee_sawi_amount);
}

bool CGovCoinTransferProposal::CheckProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE

    if (amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::CheckProposal, dust amount, %llu < %llu", amount,
                                       DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::CheckProposal, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    return true;
}

bool CGovCoinTransferProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(to_uid, desAccount)) {
        if (!to_uid.is<CKeyID>()) // first involved in transaction
            return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");

        desAccount.keyid = to_uid.get<CKeyID>();
    }

    if (!srcAccount.OperateBalance(token, BalanceOpType::SUB_FREE, amount, ReceiptCode::TRANSFER_PROPOSAL, tx.receipts, &desAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    if (!cw.accountCache.SetAccount(to_uid, desAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, save account error, kyeId=%s",
                                       desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    return true;
}

bool CGovAccountPermProposal::CheckProposal(CTxExecuteContext& context) {
    CValidationState &state = *context.pState;

    if (account_uid.IsEmpty())
        return state.DoS(100, ERRORMSG("CGovAccountPermProposal::CheckTx, target account_uid is empty"),
                        REJECT_INVALID, "account-uid-empty");

    if (proposed_perms_sum == 0 || proposed_perms_sum > kAccountAllPerms)
        return state.DoS(100, ERRORMSG("CGovAccountPermProposal::CheckTx, proposed perms is invalid: %llu",
                        proposed_perms_sum), REJECT_INVALID, "account-uid-empty");
    return true;

}

bool CGovAccountPermProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;
    CCacheWrapper &cw       = *context.pCw;

    if (proposed_perms_sum == 0)
        return state.DoS(100, ERRORMSG("CGovAccountPermProposal::CheckTx, proposed perms is invalid: %llu",
                        proposed_perms_sum), REJECT_INVALID, "asset-perms-invalid");

    CAccount acct;
    if (!cw.accountCache.GetAccount(account_uid, acct))
        return false;

    acct.perms_sum = proposed_perms_sum;
    if (!cw.accountCache.SetAccount(account_uid, acct))
        return false;

    return true;

}
bool CGovAssetPermProposal::CheckProposal(CTxExecuteContext& context) {
    CValidationState &state = *context.pState;
    CCacheWrapper &cw       = *context.pCw;

    CAsset asset;
    if (!cw.assetCache.GetAsset(asset_symbol, asset))
        return state.DoS(100, ERRORMSG("CGovAssetPermProposal::CheckTx, asset symbol not found"),
                         REJECT_INVALID, "asset-symbol-invalid");

    if (proposed_perms_sum == 0)
        return state.DoS(100, ERRORMSG("CGovAssetPermProposal::CheckTx, proposed perms is invalid: %llu",
                                       proposed_perms_sum), REJECT_INVALID, "asset-perms-invalid");
    return true;

}

bool CGovAssetPermProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;
    CCacheWrapper &cw       = *context.pCw;

    CAsset asset;
    if (!cw.assetCache.GetAsset(asset_symbol, asset))
        return state.DoS(100, ERRORMSG("%s(), asset not exist! symbol=%s",
                    __func__, asset_symbol), REJECT_INVALID, "asset-not-exist");
    // process cdp bcoin perm
    bool oldCdpBcoinPerm = asset.HasPerms(AssetPermType::PERM_CDP_BCOIN);

    asset.perms_sum = proposed_perms_sum;
    if (!cw.assetCache.SetAsset(asset))
        return state.DoS(100, ERRORMSG("%s(), save asset failed! symbol=%s",
                    __func__, asset_symbol), REJECT_INVALID, "save-asset-failed");

    bool newCdpBcoinPerm = asset.HasPerms(AssetPermType::PERM_CDP_BCOIN);
    if (newCdpBcoinPerm != oldCdpBcoinPerm) {
        CdpBcoinStatus status =
            newCdpBcoinPerm ? CdpBcoinStatus::STAKE_ON : CdpBcoinStatus::STAKE_OFF;
        if (cw.cdpCache.SetBcoinStatus(asset_symbol, status))
            return state.DoS(100, ERRORMSG("%s(), save bcoin status failed! symbol=%s",
                        __func__, asset_symbol), REJECT_INVALID, "save-bcoin-status-failed");

    }
    return true;

}

bool CGovCdpParamProposal::CheckProposal(CTxExecuteContext& context) {
    CValidationState &state = *context.pState;

    if (param_values.size() == 0 || param_values.size() > 50)
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, params list is empty or size >50"), REJECT_INVALID,
                         "params-empty");

    for (auto pa: param_values) {
        if (kCdpParamTable.count(CdpParamType(pa.first)) == 0) {
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, parameter name (%s) is not in sys params list ", 
                            pa.first), REJECT_INVALID, "params-error");
        }

        string errMsg;
        if (!CheckCdpParamValue(CdpParamType(pa.first), pa.second, errMsg))
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx failed: %s ", errMsg),
                             REJECT_INVALID, "params-range-error");
    }

    return true;
}

bool CGovCdpParamProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw       = *context.pCw;
    for (auto pa: param_values){
        auto itr = kCdpParamTable.find(CdpParamType(pa.first));
        if (itr == kCdpParamTable.end())
            return false;

        if (!cw.sysParamCache.SetCdpParam(coin_pair,CdpParamType(pa.first), pa.second))
            return false;

        if (pa.first == CdpParamType ::CDP_INTEREST_PARAM_A || pa.first == CdpParamType::CDP_INTEREST_PARAM_B) {
            if (!cw.sysParamCache.SetCdpInterestParam(coin_pair, CdpParamType(pa.first), context.height, pa.second))
                return false;
        }
    }

    return true;
}

bool CGovDexOpProposal::CheckProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE

    if (dexid == 0)
        return state.DoS(100,ERRORMSG("the No.0 dex operator can't be disable"),
                REJECT_INVALID, "operator0-can't-disable");

    if (operate_type != ProposalOperateType::ENABLE && operate_type != ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, operate type error!"), REJECT_INVALID,
                         "operate-type-error");

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is not a governor!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if ((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
        (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");

    return true;
}

bool CGovDexOpProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is not a governor!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if ((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
       (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");

    DexOperatorDetail newOperator = dexOperator;
    newOperator.activated = (operate_type == ProposalOperateType::ENABLE);

    if (!cw.dexCache.UpdateDexOperator(dexid, dexOperator, newOperator))
        return state.DoS(100, ERRORMSG("%s, save updated dex operator error! dex_id=%u", __func__, dexid),
                         UPDATE_ACCOUNT_FAIL, "save-updated-operator-error");

    return true;
}

bool CGovFeedCoinPairProposal::CheckProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (op_type == ProposalOperateType::NULL_PROPOSAL_OP)
        return state.DoS(100, ERRORMSG("%s(), op_type is null", __func__),
                         REJECT_INVALID, "bad-op-type");
    if (base_symbol == quote_symbol)
        return state.DoS(100, ERRORMSG("%s(): base_symbol==quote_symbol", __func__),
                         REJECT_INVALID, "same-base-quote-symbol");

    if (!cw.assetCache.CheckPriceFeedQuoteSymbol(quote_symbol))
        return state.DoS(100, ERRORMSG("%s(), unsupported quote_symbol=%s", __func__, quote_symbol),
                         REJECT_INVALID, "unsupported-quote-symbol");

    if (!cw.assetCache.CheckPriceFeedBaseSymbol(base_symbol))
        return state.DoS(100, ERRORMSG("%s(), unsupported base_symbol=%s", __func__, base_symbol),
                         REJECT_INVALID, "unsupported-base-symbol");

    PriceCoinPair coinPair(base_symbol, quote_symbol);
    if (kPriceFeedCoinPairSet.count(coinPair) > 0) {
        return state.DoS(100, ERRORMSG("%s(), the hard code price_coin_pair={%s:%s} can not be governed",
                __func__, quote_symbol), REJECT_INVALID, "hard-code-coin-pair");
    }

    bool hasCoin = cw.priceFeedCache.HasFeedCoinPair(coinPair);
    if (hasCoin && op_type == ProposalOperateType ::ENABLE) {
        return state.DoS(100, ERRORMSG("CGovFeedCoinPairProposal:: checkProposal:base_symbol(%s),quote_symbol(%s)"
                                       "is dex quote coin symbol already",base_symbol, quote_symbol),
                         REJECT_INVALID, "symbol-exist");
    }

    if (!hasCoin && op_type == ProposalOperateType ::DISABLE) {
        return state.DoS(100, ERRORMSG("CGovFeedCoinPairProposal:: checkProposal:base_symbol(%s),quote_symbol(%s) "
                                       "is not a dex quote coin symbol ",base_symbol, quote_symbol),
                         REJECT_INVALID, "symbol-not-exist");
    }

    return true;
}

bool CGovFeedCoinPairProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper& cw = *context.pCw;
    PriceCoinPair coinPair(base_symbol, quote_symbol);
    if (ProposalOperateType::ENABLE == op_type)
        return cw.priceFeedCache.AddFeedCoinPair(coinPair);
    else
        return cw.priceFeedCache.EraseFeedCoinPair(coinPair);

}
bool CGovAxcInProposal::CheckProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE;

    AxcSwapCoinPair coinPair;
    if(!cw.axcCache.GetAxcCoinPairByPeerSymbol(peer_chain_token_symbol, coinPair)){
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: swap pair is not exist"),
                REJECT_INVALID, "find-swapcoinpair-error");
    }
    TokenSymbol self_chain_token_symbol = coinPair.self_token_symbol;
    ChainType  peer_chain_type = coinPair.peer_chain_type;

    if ((peer_chain_type == ChainType::BITCOIN && (peer_chain_addr.size() < 26 || peer_chain_addr.size() > 35)) ||
        (peer_chain_type == ChainType::ETHEREUM && (peer_chain_addr.size() > 42)))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: peer_chain_addr=%s invalid",
                                        peer_chain_addr), REJECT_INVALID, "peer_chain_addr-invalid");

    if ( (peer_chain_type == ChainType::BITCOIN && (peer_chain_txid.size() != 65)) ||
         (peer_chain_type == ChainType::ETHEREUM && (peer_chain_txid.size() != 65)) )
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: peer_chain_txid=%s invalid",
                                        peer_chain_txid), REJECT_INVALID, "peer_chain_txid-invalid");
    if (self_chain_uid.IsEmpty())
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: self_chain_uid empty"), 
                                        REJECT_INVALID, "self_chain_uid-empty");
    CAccount acct;
    if (!cw.accountCache.GetAccount(self_chain_uid, acct))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: read account failed"), REJECT_INVALID,
                        "bad-getaccount");

    if (swap_amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: swap_amount=%llu too small",
                                        swap_amount), REJECT_INVALID, "swap_amount-dust");

    uint64_t mintAmount = 0;
    if (cw.axcCache.GetSwapInMintRecord(peer_chain_type, peer_chain_txid, mintAmount))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: GetSwapInMintRecord existing err  %s", peer_chain_txid),
                        REJECT_INVALID, "get_swapin_mint-record-err");
    return true;
}

bool CGovAxcInProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    AxcSwapCoinPair coinPair;
    if(!cw.axcCache.GetAxcCoinPairByPeerSymbol(peer_chain_token_symbol, coinPair)){
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: swap pair is not exist"),
                         REJECT_INVALID, "find-swapcoinpair-error");
    }
    TokenSymbol self_chain_token_symbol = coinPair.self_token_symbol;
    ChainType  peer_chain_type = coinPair.peer_chain_type;


    uint64_t swap_fee_ratio;
    if (!cw.sysParamCache.GetParam(AXC_SWAP_FEE_RATIO, swap_fee_ratio) || swap_fee_ratio * 1.0 / RATIO_BOOST > 1)
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, get sysparam: axc_swap_fee_ratio failed"),
                        REJECT_INVALID, "bad-get-swap_fee_ratio");

    uint64_t swap_fees = swap_fee_ratio * (swap_amount * 1.0 / RATIO_BOOST);
    uint64_t swap_amount_after_fees = swap_amount - swap_fees;

    set<CRegID> govBpRegIds;
    if (!cw.sysGovernCache.GetGovernors(govBpRegIds) || govBpRegIds.size() == 0)
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, failed to get BP Governors"),
                        REJECT_INVALID, "bad-get-bp-governors");

    uint64_t swapFeesPerBp = swap_fees / govBpRegIds.size();
    for (const auto &bpRegID : govBpRegIds) {
        CAccount bpAcct;
        if (!cw.accountCache.GetAccount(bpRegID, bpAcct))
            return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, failed to get BP account (%s)", bpRegID.ToString()),
                        REJECT_INVALID, "bad-get-bp-account");

        if (!bpAcct.OperateBalance(self_chain_token_symbol, BalanceOpType::ADD_FREE, swapFeesPerBp, 
                                    ReceiptCode::AXC_REWARD_FEE_TO_BP, tx.receipts))
            return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, opreate balance failed, swapFeesPerBp=%llu",
                        swapFeesPerBp), REJECT_INVALID, "bad-operate-balance");
    }

    //reward axc GW
    //TODO

    CAccount acct;
    if (!cw.accountCache.GetAccount(self_chain_uid, acct))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, read account failed"), REJECT_INVALID,
                        "bad-getaccount");

    // mint the new mirro-coin (self_chain_token_symbol) out of thin air
    if (!acct.OperateBalance(self_chain_token_symbol, BalanceOpType::ADD_FREE, swap_amount_after_fees, 
                            ReceiptCode::AXC_MINT_COINS, tx.receipts))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, opreate balance failed, swap_amount_after_fees=%llu",
                        swap_amount_after_fees), REJECT_INVALID, "bad-operate-balance");

    if (!cw.accountCache.SetAccount(self_chain_uid, acct))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, write account failed"), REJECT_INVALID,
                         "bad-write-account");

    uint64_t mintAmount = 0;
    if (cw.axcCache.GetSwapInMintRecord(peer_chain_type, peer_chain_txid, mintAmount))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: GetSwapInMintRecord existing err %s",
                        REJECT_INVALID, "get_swapin_mint_record-err"));

    if (!cw.axcCache.SetSwapInMintRecord(peer_chain_type, peer_chain_txid, swap_amount_after_fees))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::CheckProposal: SetSwapInMintRecord existing err %s",
                        REJECT_INVALID, "get_swapin_mint_record-err"));

    return true;
}

bool CGovAxcOutProposal::CheckProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE;

    AxcSwapCoinPair coinPair;
    if(!cw.axcCache.GetAxcCoinPairBySelfSymbol(self_chain_token_symbol, coinPair)){
        return state.DoS(100, ERRORMSG("CGovAxcOutProposal::CheckProposal: self_chain_token_symbol=%s is invalid",
                                       self_chain_token_symbol), REJECT_INVALID, "self_chain_token_symbol-not-valid");
    }

    ChainType  peer_chain_type = coinPair.peer_chain_type;
    if ((peer_chain_type == ChainType::BITCOIN && (peer_chain_addr.size() < 26 || peer_chain_addr.size() > 35)) ||
        (peer_chain_type == ChainType::ETHEREUM && (peer_chain_addr.size() > 42)))
        return state.DoS(100, ERRORMSG("CGovAxcOutProposal::CheckProposal: peer_chain_addr=%s invalid",
                                        peer_chain_addr), REJECT_INVALID, "peer_chain_addr-invalid");

    CAccount acct;
    if (!cw.accountCache.GetAccount(self_chain_uid, acct))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, read account failed"), REJECT_INVALID,
                         "bad-getaccount");
    ReceiptList receipts;
    if (!acct.OperateBalance(self_chain_token_symbol, SUB_FREE, swap_amount, ReceiptCode::NULL_CODE, receipts))
        return state.DoS(100, ERRORMSG("CGovAxcOutProposal::CheckProposal:Account does not have enough %s",
                                   self_chain_token_symbol), REJECT_INVALID, "balance-not-enough");

    if (swap_amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CGovAxcOutProposal::CheckProposal: swap_amount=%llu too small",
                                        swap_amount), REJECT_INVALID, "swap_amount-dust");

    return true;
}

bool CGovAxcOutProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    uint64_t swap_fee_ratio;
    if (!cw.sysParamCache.GetParam(AXC_SWAP_FEE_RATIO, swap_fee_ratio))
        return state.DoS(100, ERRORMSG("CGovAxcOutProposal::ExecuteProposal, get sysparam: axc_swap_fee_ratio failed"),
                        REJECT_INVALID, "bad-get-swap_fee_ratio");

    CAccount acct;
    if (!cw.accountCache.GetAccount(self_chain_uid, acct))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, read account failed"), REJECT_INVALID,
                        "bad-getaccount");

    // burn the mirroed tokens from self-chain
    if (!acct.OperateBalance(self_chain_token_symbol, BalanceOpType::SUB_FREE, swap_amount, ReceiptCode::AXC_BURN_COINS, tx.receipts))
        return state.DoS(100, ERRORMSG("CGovAxcOutProposal::ExecuteProposal, opreate balance failed, swap_amount=%llu",
                        swap_amount), REJECT_INVALID, "bad-operate-balance");

    if (!cw.accountCache.SetAccount(self_chain_uid, acct))
        return state.DoS(100, ERRORMSG("CGovAxcInProposal::ExecuteProposal, write account failed"), REJECT_INVALID,
                         "bad-writeaccount");

    return true;
}


bool CGovAxcCoinProposal::CheckProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (op_type != ProposalOperateType::ENABLE && op_type != ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::CheckProposal, op_type is not 0 or 1"), REJECT_INVALID,
                         "op_type-error");

    bool hasCoinPair = cw.axcCache.HasAxcCoinPairByPeerSymbol(peer_chain_coin_symbol);
    if(op_type == ProposalOperateType::ENABLE && hasCoinPair)
        return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::CheckProposal, the peer coin symbol is exist"), REJECT_INVALID,
                         "coin-exist");

    if(op_type == ProposalOperateType::DISABLE && !hasCoinPair)
        return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::CheckProposal, the peer coin symbol is not exist"), REJECT_INVALID,
                         "coin-not-exist");

    switch (peer_chain_type) {
        case ChainType ::BITCOIN:
        case ChainType ::EOS:
        case ChainType ::ETHEREUM:
            break;
        default:
            return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::CheckProposal, chain type is not eos, btc, ethereum"), REJECT_INVALID,
                             "chain_type-error");
    }

    if (kXChainSwapInTokenMap.find(peer_chain_coin_symbol) != kXChainSwapInTokenMap.end())
        return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::CheckProposal,default pair can't be governed"), REJECT_INVALID,
                         "default-coin-error");

    if (peer_chain_coin_symbol.size() >= 6)
        return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::CheckProposal, peer_chain_coin_symbol size is too long"), REJECT_INVALID,
                         "peer_coin_symbol-error");

    return true;
}

bool  CGovAxcCoinProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE

    if(op_type == ProposalOperateType::DISABLE) {
        if(!cw.axcCache.EraseAxcSwapPair(peer_chain_coin_symbol))
            return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::ExecuteProposal, write db error"), REJECT_INVALID,
                             "db-error");

    } else if (op_type == ProposalOperateType::ENABLE) {
        if (!cw.axcCache.AddAxcSwapPair(peer_chain_coin_symbol, TokenSymbol(strprintf("%s%s", "m", peer_chain_coin_symbol)), peer_chain_type))
            return state.DoS(100, ERRORMSG("CGovAxcCoinProposal::ExecuteProposal, write db error"), REJECT_INVALID,
                             "db-error");

    } else
        return false;

    return true;
}
