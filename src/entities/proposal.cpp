// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include <algorithm>
#include "main.h"
#include <set>
#include "config/txbase.h"

extern bool CheckIsGovernor(CRegID account, ProposalType proposalType,CCacheWrapper&cw );
extern uint8_t GetGovernorApprovalMinCount(ProposalType proposalType, CCacheWrapper& cw );
bool CParamsGovernProposal::ExecuteProposal(CTxExecuteContext& context){

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

 bool CParamsGovernProposal::CheckProposal(CTxExecuteContext& context ) {

     CValidationState &state = *context.pState;

     if(param_values.size() == 0)
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, params list is empty"), REJECT_INVALID,
                        "params-empty");
       for(auto pa: param_values){
           if(SysParamTable.count(SysParamType(pa.first)) == 0){
               return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, parameter name (%s) is not in sys params list ", pa.first),
                       REJECT_INVALID, "params-error");
           }
       }

     return true ;
}

bool CCdpParamGovernProposal::ExecuteProposal(CTxExecuteContext& context){

    CCacheWrapper &cw       = *context.pCw;
    for( auto pa: param_values){
        auto itr = CdpParamTable.find(CdpParamType(pa.first));
        if(itr == CdpParamTable.end())
            return false ;

        if(!cw.sysParamCache.SetCdpParam(coin_pair,CdpParamType(pa.first), pa.second)){
            return false ;
        }
        if(pa.first == CdpParamType ::CDP_INTEREST_PARAM_A
           || pa.first == CdpParamType::CDP_INTEREST_PARAM_B){

            if(!cw.sysParamCache.SetCdpInterestParam(coin_pair, CdpParamType(pa.first), context.height, pa.second)){
                return false ;
            }

        }
    }

    return true ;
}

bool CCdpParamGovernProposal::CheckProposal(CTxExecuteContext& context ) {

    CValidationState &state = *context.pState;

    if (param_values.size() == 0 || param_values.size() > 50)
        return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, params list is empty or size >50"), REJECT_INVALID,
                         "params-empty");
    for (auto pa: param_values) {
        if (SysParamTable.count(SysParamType(pa.first)) == 0) {
            return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, parameter name (%s) is not in sys params list ",
                                           pa.first),
                             REJECT_INVALID, "params-error");
        }
    }

    return true;
}

bool CGovernorUpdateProposal::ExecuteProposal(CTxExecuteContext& context){

    CCacheWrapper &cw       = *context.pCw;
    if(operate_type == ProposalOperateType::DISABLE){
        vector<CRegID> governors ;
        if(cw.sysGovernCache.GetGovernors(governors)){

            for(auto itr = governors.begin();itr !=governors.end();){
                if(*itr == governor_regid){
                    governors.erase(itr);
                    break ;
                }else
                    itr++ ;
            }
            return cw.sysGovernCache.SetGovernors(governors) ;
        }
        return false ;

    }else if(operate_type == ProposalOperateType::ENABLE){

        vector<CRegID> governors ;
        cw.sysGovernCache.GetGovernors(governors);

        if(find( governors.begin(),governors.end(),governor_regid) != governors.end()){
            return false ;
        }

        governors.push_back(governor_regid) ;
        return cw.sysGovernCache.SetGovernors(governors) ;
    }

    return false  ;

}

 bool CGovernorUpdateProposal::CheckProposal(CTxExecuteContext& context ){

    IMPLEMENT_DEFINE_CW_STATE

     if(operate_type != ProposalOperateType::ENABLE && operate_type != ProposalOperateType::DISABLE){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, operate type is illegal!"), REJECT_INVALID,
                          "operate_type-illegal");
     }

     CAccount governor_account ;
     if(!cw.accountCache.GetAccount(governor_regid,governor_account)){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, governor regid(%s) is not exist!", governor_regid.ToString()), REJECT_INVALID,
                          "governor-not-exist");
     }
     vector<CRegID> governers ;

     if(operate_type == ProposalOperateType ::DISABLE&&!cw.sysGovernCache.CheckIsGovernor(governor_regid)){
         return state.DoS(100, ERRORMSG("CProposalRequestTx::CheckTx, regid(%s) is not a governor!", governor_regid.ToString()), REJECT_INVALID,
                          "regid-not-governor");
     }
    return true ;
}

bool CDexSwitchProposal::ExecuteProposal(CTxExecuteContext& context) {

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
bool CDexSwitchProposal::CheckProposal(CTxExecuteContext& context ) {

    IMPLEMENT_DEFINE_CW_STATE

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


bool CMinerFeeProposal:: CheckProposal(CTxExecuteContext& context ) {

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

bool CMinerFeeProposal:: ExecuteProposal(CTxExecuteContext& context) {
    CCacheWrapper &cw       = *context.pCw;
    return cw.sysParamCache.SetMinerFee(tx_type,fee_symbol,fee_sawi_amount);
}

///////////////////////////////////////////////////////////////////////////////
// class CCdpCoinPairProposal

 Object CCdpCoinPairProposal::ToJson() {
    Object o = CProposal::ToJson();
    o.push_back(Pair("cdp_coin_pair", cdp_coin_pair.ToString()));

    o.push_back(Pair("status", GetCdpCoinPairStatusName(status))) ;
    return o ;
}

string CCdpCoinPairProposal::ToString() {
    return  strprintf("cdp_coin_pair=%s", cdp_coin_pair.ToString()) + ", " +
            strprintf("status=%s", GetCdpCoinPairStatusName(status));
}


shared_ptr<string> CheckCdpAssetSymbol(CCacheWrapper &cw, const TokenSymbol &symbol) {
    size_t coinSymbolSize = symbol.size();
    if (coinSymbolSize == 0 || coinSymbolSize > MAX_TOKEN_SYMBOL_LEN) {
        return make_shared<string>("empty or too long");
    }

    if ((coinSymbolSize < MIN_ASSET_SYMBOL_LEN && !kCoinTypeSet.count(symbol)) ||
        (coinSymbolSize >= MIN_ASSET_SYMBOL_LEN && !cw.assetCache.HaveAsset(symbol)))
        return make_shared<string>("unsupported symbol");

    return nullptr;
}


bool CCdpCoinPairProposal::CheckProposal(CTxExecuteContext& context ) {

    IMPLEMENT_DEFINE_CW_STATE

    if (kScoinSymbolSet.count(cdp_coin_pair.bcoin_symbol) == 0) {
        return state.DoS(100, ERRORMSG("%s, the scoin_symbol=%s of cdp coin pair does not support!",
                __func__, cdp_coin_pair.bcoin_symbol), REJECT_INVALID, "unsupported_scoin_symbol");
    }

    auto symbolErr = CheckCdpAssetSymbol(cw, cdp_coin_pair.bcoin_symbol);
    if (symbolErr) {
        return state.DoS(100, ERRORMSG("%s(), unsupport cdp asset symbol=%s! %s", cdp_coin_pair.bcoin_symbol, *symbolErr),
            REJECT_INVALID, "unsupported-asset-symbol");
    }

    if (status == CdpCoinPairStatus::NONE || kCdpCoinPairStatusNames.count(status) == 0 ) {
        return state.DoS(100, ERRORMSG("%s(), unsupport status=%d", (uint8_t)status), REJECT_INVALID, "unsupported-status");
    }
  return true ;
}

bool CCdpCoinPairProposal::ExecuteProposal(CTxExecuteContext& context) {

    if (!context.pCw->cdpCache.SetCdpCoinPairStatus(cdp_coin_pair, status)) {
        return context.pState->DoS(100, ERRORMSG("%s(), save cdp coin pair failed! coin_pair=%s, status=%s",
                cdp_coin_pair.ToString(), GetCdpCoinPairStatusName(status)),
            REJECT_INVALID, "unsupported-asset-symbol");
    }
    return true;
}


bool CCoinTransferProposal:: ExecuteProposal(CTxExecuteContext& context) {


    IMPLEMENT_DEFINE_CW_STATE;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::ExecuteProposal, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t minusValue = amount;
    if (!srcAccount.OperateBalance(token, BalanceOpType::SUB_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::ExecuteProposal, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::ExecuteProposal, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(to_uid, desAccount)) {
        if (to_uid.is<CKeyID>()) {  // first involved in transaction
            desAccount.keyid = to_uid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CCoinTransferProposal::ExecuteProposal, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAccount.OperateBalance(token, BalanceOpType::ADD_FREE, amount)) {
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::ExecuteProposal, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(to_uid, desAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::ExecuteProposal, save account error, kyeId=%s",
                                       desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");


    return true ;
}
bool CCoinTransferProposal:: CheckProposal(CTxExecuteContext& context ) {

    IMPLEMENT_DEFINE_CW_STATE

    if (amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::CheckProposal, dust amount, %llu < %llu", amount,
                                       DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(from_uid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferProposal::CheckProposal, read account failed"), REJECT_INVALID,
                         "bad-getaccount");



    return true ;
}



bool CBPCountUpdateProposal:: ExecuteProposal(CTxExecuteContext& context) {

    IMPLEMENT_DEFINE_CW_STATE;

    auto currentBpCount = cw.delegateCache.GetActivedDelegateNum() ;
    if(!cw.sysParamCache.SetCurrentBpCount(currentBpCount)) {
        return state.DoS(100, ERRORMSG("CBPCountUpdateProposal::ExecuteProposal, save current bp count failed!"),
                REJECT_INVALID, "save-currbpcount-failed");
    }

    if(!cw.sysParamCache.SetNewBpCount(bp_count,launch_height)){
        return state.DoS(100, ERRORMSG("CBPCountUpdateProposal::ExecuteProposal, save new bp count failed!"),
                REJECT_INVALID, "save-newbpcount-failed");
    }

    return true ;

}
bool CBPCountUpdateProposal:: CheckProposal(CTxExecuteContext& context ) {

    CValidationState& state = *context.pState ;

    if( bp_count == 0 || bp_count>=255)
        return state.DoS(100, ERRORMSG("CBPCountUpdateProposal::CheckProposal,bp_count must be between 0 and 255"),
                REJECT_INVALID,"bad-bp-count") ;

    if(launch_height < (uint32_t)context.height +3600){
        return state.DoS(100, ERRORMSG("CBPCountUpdateProposal::CheckProposal,launch_height must more than current height + 3600"),
                         REJECT_INVALID,"bad-bp-count") ;

    }

    return true  ;
}

