// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "config/txbase.h"
#include "config/const.h"
#include "entities/asset.h"
#include "entities/proposal.h"
#include "persistence/cachewrapper.h"

#include <algorithm>
#include "main.h"
#include <set>

extern bool CheckIsGovernor(CRegID account, ProposalType proposalType,CCacheWrapper&cw );
extern uint8_t GetGovernorApprovalMinCount(ProposalType proposalType, CCacheWrapper& cw );

bool CGovSysParamProposal::CheckProposal(CTxExecuteContext& context ) {
     CValidationState &state = *context.pState;

     if(param_values.size() == 0)
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, params list is empty"), REJECT_INVALID,
                        "params-empty");
       for(auto pa: param_values){
           if(SysParamTable.count(SysParamType(pa.first)) == 0){
               return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                       REJECT_INVALID, "params-error");
           }
           string errorInfo = CheckSysParamValue(SysParamType(pa.first), pa.second);

           if(errorInfo != EMPTY_STRING)
               return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx failed: %s ", errorInfo),
                                REJECT_INVALID, "params-range-error");
       }

     return true ;
}

bool CGovSysParamProposal::ExecuteProposal(CTxExecuteContext& context){
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


bool CGovCdpParamProposal::ExecuteProposal(CTxExecuteContext& context) {
    CCacheWrapper &cw       = *context.pCw;
    for (auto pa: param_values){
        auto itr = CdpParamTable.find(CdpParamType(pa.first));
        if (itr == CdpParamTable.end())
            return false ;

        if (!cw.sysParamCache.SetCdpParam(coin_pair,CdpParamType(pa.first), pa.second))
            return false ;

        if (pa.first == CdpParamType ::CDP_INTEREST_PARAM_A || pa.first == CdpParamType::CDP_INTEREST_PARAM_B) {
            if (!cw.sysParamCache.SetCdpInterestParam(coin_pair, CdpParamType(pa.first), context.height, pa.second))
                return false ;
        }
    }

    return true ;
}

bool CGovCdpParamProposal::CheckProposal(CTxExecuteContext& context ) {
    CValidationState &state = *context.pState;

    if (param_values.size() == 0 || param_values.size() > 50)
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, params list is empty or size >50"), REJECT_INVALID,
                         "params-empty");

    for (auto pa: param_values) {
        if (CdpParamTable.count(CdpParamType(pa.first)) == 0) {
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                            REJECT_INVALID, "params-error");
        }

        string errorInfo = CheckCdpParamValue(CdpParamType(pa.first), pa.second);
        if(errorInfo != EMPTY_STRING)
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx failed: %s ", errorInfo),
                             REJECT_INVALID, "params-range-error");
    }

    return true;
}

bool CGovBpMcListProposal::ExecuteProposal(CTxExecuteContext& context) {
    CCacheWrapper &cw       = *context.pCw;

    if (op_type == ProposalOperateType::DISABLE) {
        vector<CRegID> governors;
        if (cw.sysGovernCache.GetGovernors(governors)) {
            for (auto itr = governors.begin(); itr != governors.end();) {
                if (*itr == governor_regid) {
                    governors.erase(itr);
                    break ;
                } else
                    itr++ ;
            }
            return cw.sysGovernCache.SetGovernors(governors) ;
        }

        return false ;

    } else if (op_type == ProposalOperateType::ENABLE) {
        vector<CRegID> governors ;
        cw.sysGovernCache.GetGovernors(governors);

        if (find(governors.begin(),governors.end(),governor_regid) != governors.end())
            return false ;

        governors.push_back(governor_regid) ;
        return cw.sysGovernCache.SetGovernors(governors) ;
    }

    return false  ;

}

 bool CGovBpMcListProposal::CheckProposal(CTxExecuteContext& context ){
    IMPLEMENT_DEFINE_CW_STATE

     if(op_type != ProposalOperateType::ENABLE && op_type != ProposalOperateType::DISABLE){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, operate type is illegal!"), REJECT_INVALID,
                          "operate_type-illegal");
     }

     CAccount governor_account ;
     if(!cw.accountCache.GetAccount(governor_regid,governor_account)){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, governor regid(%s) is not exist!", governor_regid.ToString()), REJECT_INVALID,
                          "governor-not-exist");
     }
     vector<CRegID> governers ;

     if(op_type == ProposalOperateType ::DISABLE&&!cw.sysGovernCache.CheckIsGovernor(governor_regid)){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, regid(%s) is not a governor!", governor_regid.ToString()), REJECT_INVALID,
                          "regid-not-governor");
     }
    return true ;
}

bool CGovDexOpProposal::ExecuteProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is not a governor!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
       (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE)){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");
    }

    DexOperatorDetail newOperator = dexOperator;
    newOperator.activated = (operate_type == ProposalOperateType::ENABLE);

    if (!cw.dexCache.UpdateDexOperator(dexid, dexOperator, newOperator))
        return state.DoS(100, ERRORMSG("%s, save updated dex operator error! dex_id=%u", __func__, dexid),
                         UPDATE_ACCOUNT_FAIL, "save-updated-operator-error");

    return true ;
}

bool CGovDexOpProposal::CheckProposal(CTxExecuteContext& context ) {
    IMPLEMENT_DEFINE_CW_STATE

    if(dexid == 0)
        return state.DoS(100,ERRORMSG("the No.0 dex operator can't be disable"),
                REJECT_INVALID, "operator0-can't-disable");
    if(operate_type != ProposalOperateType::ENABLE && operate_type != ProposalOperateType::DISABLE){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, operate type error!"), REJECT_INVALID,
                         "operate-type-error");
    }

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is not a governor!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
        (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE)){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");
    }

    return true ;
}


bool CGovMinerFeeProposal:: CheckProposal(CTxExecuteContext& context ) {
    CValidationState& state = *context.pState ;

    if(!kFeeSymbolSet.count(fee_symbol)) {
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, fee symbol(%s) is invalid!", fee_symbol),
                        REJECT_INVALID,
                        "feesymbol-error");
    }

    auto itr = kTxFeeTable.find(tx_type);
    if(itr == kTxFeeTable.end()){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, the tx type (%d) is invalid!", tx_type),
                        REJECT_INVALID,
                        "txtype-error");
    }

    if(!std::get<5>(itr->second)){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, the tx type (%d) miner fee can't be updated!", tx_type),
                        REJECT_INVALID,
                        "can-not-update");
    }

    if(fee_sawi_amount == 0 ){
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, the tx type (%d) miner fee can't be zero", tx_type),
                        REJECT_INVALID,
                        "can-not-be-zero");
    }
    return true ;
}

bool CGovMinerFeeProposal:: ExecuteProposal(CTxExecuteContext& context) {
    CCacheWrapper &cw       = *context.pCw;
    return cw.sysParamCache.SetMinerFee(tx_type,fee_symbol,fee_sawi_amount);
}

bool CGovCoinPairProposal::CheckProposal(CTxExecuteContext& context ) {
    IMPLEMENT_DEFINE_CW_STATE

    if (kScoinSymbolSet.count(GOV_CDP_COINPAIR.bcoin_symbol) == 0)
        return state.DoS(100, ERRORMSG("%s, the scoin_symbol=%s of cdp coin pair does not support!",
                __func__, GOV_CDP_COINPAIR.bcoin_symbol), REJECT_INVALID, "unsupported_scoin_symbol");

    if (!cw.assetCache.CheckAsset(GOV_CDP_COINPAIR.bcoin_symbol, AssetPermType::PERM_CDP_BCOIN))
        return state.DoS(100, ERRORMSG("%s(), unsupported cdp bcoin symbol=%s!", GOV_CDP_COINPAIR.bcoin_symbol),
            REJECT_INVALID, "unsupported-asset-bcoin-symbol");

    if (status == CdpCoinPairStatus::NONE || kCdpCoinPairStatusNames.count(status) == 0 )
        return state.DoS(100, ERRORMSG("%s(), unsupport status=%d", (uint8_t)status), REJECT_INVALID, "unsupported-status");
  
    return true ;
}

bool CGovCoinPairProposal::ExecuteProposal(CTxExecuteContext& context) {

    if (!context.pCw->cdpCache.SetCdpCoinPairStatus(GOV_CDP_COINPAIR, status)) {
        return context.pState->DoS(100, ERRORMSG("%s(), save cdp coin pair failed! coin_pair=%s, status=%s",
                GOV_CDP_COINPAIR.ToString(), GetCdpCoinPairStatusName(status)),
            REJECT_INVALID, "unsupported-asset-symbol");
    }
    return true;
}


bool CGovCoinTransferProposal:: ExecuteProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t minusValue = amount;
    if (!srcAccount.OperateBalance(token, BalanceOpType::SUB_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(to_uid, desAccount)) {
        if (to_uid.is<CKeyID>()) {  // first involved in transaction
            desAccount.keyid = to_uid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAccount.OperateBalance(token, BalanceOpType::ADD_FREE, amount)) {
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(to_uid, desAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, save account error, kyeId=%s",
                                       desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");


    return true ;
}

bool CGovCoinTransferProposal:: CheckProposal(CTxExecuteContext& context ) {
    IMPLEMENT_DEFINE_CW_STATE

    if (amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::CheckProposal, dust amount, %llu < %llu", amount,
                                       DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::CheckProposal, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    return true ;
}


bool CGovBpSizeProposal:: ExecuteProposal(CTxExecuteContext& context) {
    IMPLEMENT_DEFINE_CW_STATE;

    auto currentBpCount = cw.delegateCache.GetActivedDelegateNum() ;
    if(!cw.sysParamCache.SetCurrentBpCount(currentBpCount)) {
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::ExecuteProposal, save current bp count failed!"),
                REJECT_INVALID, "save-currbpcount-failed");
    }

    if(!cw.sysParamCache.SetNewBpCount(bp_count,effective_height)){
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::ExecuteProposal, save new bp count failed!"),
                REJECT_INVALID, "save-newbpcount-failed");
    }

    return true ;

}
bool CGovBpSizeProposal:: CheckProposal(CTxExecuteContext& context ) {
    CValidationState& state = *context.pState ;

    if (bp_count == 0) //bp_count > BP_MAX_COUNT: always false
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::CheckProposal, bp_count must be between 1 and 255"),
                        REJECT_INVALID,"bad-bp-count") ;

    if (effective_height < (uint32_t) context.height + GOVERN_EFFECTIVE_AFTER_BLOCK_COUNT)
        return state.DoS(100, ERRORMSG("CGovBpSizeProposal::CheckProposal: effective_height must be >= current height + 3600"),
                         REJECT_INVALID,"bad-bp-count") ;

    return true  ;
}

bool CGovDexQuoteProposal::ExecuteProposal(CTxExecuteContext& context) {
    CCacheWrapper& cw = *context.pCw ;

    if(ProposalOperateType::ENABLE == op_type)
        return cw.dexCache.AddDexQuoteCoin(coin_symbol) ;
    else
        return cw.dexCache.EraseDexQuoteCoin(coin_symbol) ;

}

bool CGovDexQuoteProposal::CheckProposal(CTxExecuteContext& context ) {
    IMPLEMENT_DEFINE_CW_STATE

    if(op_type == ProposalOperateType::NULL_PROPOSAL_OP)
        return state.DoS(100, ERRORMSG("CGovDexQuoteProposal:: checkProposal: op_type is null "),
                        REJECT_INVALID, "bad-op-type") ;

    string errMsg = "";
    if (!CAsset::CheckSymbol(AssetType::DIA, coin_symbol, errMsg))
        return state.DoS(100, ERRORMSG("CGovDexQuoteProposal:: checkProposal: CheckSymbol failed: %s", errMsg),
                        REJECT_INVALID, "bad-symbol") ;

    bool hasCoin = cw.dexCache.HaveDexQuoteCoin(coin_symbol);
    if (hasCoin && op_type == ProposalOperateType::ENABLE)
        return state.DoS(100, ERRORMSG("CGovDexQuoteProposal:: checkProposal:coin_symbol(%s) "
                                       "is dex quote coin symbol already",coin_symbol),
                         REJECT_INVALID, "symbol-exist") ;

    if (!hasCoin && op_type == ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("CGovDexQuoteProposal:: checkProposal:coin_symbol(%s) "
                                       "is not a dex quote coin symbol ",coin_symbol),
                         REJECT_INVALID, "symbol-not-exist") ;

    return true ;
}

bool CGovFeedCoinPairProposal::CheckProposal(CTxExecuteContext& context ) {
    IMPLEMENT_DEFINE_CW_STATE

    if(op_type == ProposalOperateType::NULL_PROPOSAL_OP)
        return state.DoS(100, ERRORMSG("CGovDexQuoteProposal:: checkProposal: op_type is null "),
                         REJECT_INVALID, "bad-op-type") ;

    if (!cw.assetCache.CheckAsset(feed_symbol, AssetPermType::PERM_PRICE_FEED))
        return state.DoS(100, ERRORMSG("CGovFeedCoinPairProposal:: checkProposal: feed_symbol invalid"),
                         REJECT_INVALID, "bad-symbol") ;

    if (!kPriceQuoteSymbolSet.count(quote_symbol))
        return state.DoS(100, ERRORMSG("CGovFeedCoinPairProposal:: checkProposal: invalid quote_symbol %s", quote_symbol),
                         REJECT_INVALID, "bad-symbol") ;

    bool hasCoin = cw.priceFeedCache.HasFeedCoinPair(feed_symbol, quote_symbol);
    if( hasCoin && op_type == ProposalOperateType ::ENABLE) {
        return state.DoS(100, ERRORMSG("CGovFeedCoinPairProposal:: checkProposal:feed_symbol(%s),quote_symbol(%s)"
                                       "is dex quote coin symbol already",feed_symbol, quote_symbol),
                         REJECT_INVALID, "symbol-exist") ;
    }

    if( !hasCoin && op_type == ProposalOperateType ::DISABLE) {
        return state.DoS(100, ERRORMSG("CGovFeedCoinPairProposal:: checkProposal:feed_symbol(%s),quote_symbol(%s) "
                                       "is not a dex quote coin symbol ",feed_symbol, quote_symbol),
                         REJECT_INVALID, "symbol-not-exist") ;
    }
    return true ;
}

bool CGovFeedCoinPairProposal::ExecuteProposal(CTxExecuteContext& context) {

    CCacheWrapper& cw = *context.pCw ;

    if (ProposalOperateType::ENABLE == op_type)
        return cw.priceFeedCache.AddFeedCoinPair(feed_symbol, quote_symbol) ;
    else
        return cw.priceFeedCache.EraseFeedCoinPair(feed_symbol, quote_symbol) ;

}


bool CGovAxcInProposal::CheckProposal(CTxExecuteContext& context ) {

    CValidationState& state = *context.pState ;

    return true  ;
}

bool CGovAxcInProposal::ExecuteProposal(CTxExecuteContext& context ) {

    CValidationState& state = *context.pState ;

    return true  ;
}

bool CGovAxcOutProposal::CheckProposal(CTxExecuteContext& context ) {

    CValidationState& state = *context.pState ;

    return true  ;
}

bool CGovAxcOutProposal::ExecuteProposal(CTxExecuteContext& context ) {

    CValidationState& state = *context.pState ;

    return true  ;
}