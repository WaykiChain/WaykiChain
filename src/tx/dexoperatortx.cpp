// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexoperatortx.h"

#include "config/configuration.h"
#include "main.h"
#include <regex>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// ProcessAssetFee

static const string OPERATOR_ACTION_REGISTER = "register";
static const string OPERATOR_ACTION_UPDATE = "update";
static const uint32_t MAX_NAME_LEN = 32;
static const uint64_t MAX_MATCH_FEE_RATIO_VALUE = 50000000; // 50%

static bool ProcessDexOperatorFee(CCacheWrapper &cw, CValidationState &state, const string &action,
    CAccount &txAccount, vector<CReceipt> &receipts) {

    uint64_t exchangeFee = 0;
    if (action == OPERATOR_ACTION_REGISTER) {
        if (!cw.sysParamCache.GetParam(DEX_OPERATOR_REGISTER_FEE, exchangeFee))
            return state.DoS(100, ERRORMSG("%s(), read param DEX_OPERATOR_REGISTER_FEE error", __func__),
                            REJECT_INVALID, "read-sysparam-error");
    } else {
        assert(action == OPERATOR_ACTION_UPDATE);
        if (!cw.sysParamCache.GetParam(DEX_OPERATOR_UPDATE_FEE, exchangeFee))
            return state.DoS(100, ERRORMSG("%s(), read param DEX_OPERATOR_UPDATE_FEE error", __func__),
                            REJECT_INVALID, "read-sysparam-error");
    }

    if (!txAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, exchangeFee))
        return state.DoS(100, ERRORMSG("%s(), tx account insufficient funds for operator %s fee! fee=%llu, tx_addr=%s",
                        __func__, action, exchangeFee, txAccount.keyid.ToAddress()),
                        UPDATE_ACCOUNT_FAIL, "insufficent-funds");

    uint64_t riskFee       = exchangeFee * ASSET_RISK_FEE_RATIO / RATIO_BOOST;
    uint64_t minerTotalFee = exchangeFee - riskFee;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("%s(), get risk riserve account failed", __func__),
                        READ_ACCOUNT_FAIL, "get-account-failed");

    if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, riskFee)) {
        return state.DoS(100, ERRORMSG("%s(), operate balance failed! add %s asset fee=%llu to risk riserve account error",
            __func__, action, riskFee), UPDATE_ACCOUNT_FAIL, "update-account-failed");
    }
    if (action == OPERATOR_ACTION_REGISTER)
        receipts.emplace_back(txAccount.regid, fcoinGenesisAccount.regid, SYMB::WICC, riskFee, ReceiptCode::DEX_OPERATOR_REG_FEE_TO_RISERVE);
    else
        receipts.emplace_back(txAccount.regid, fcoinGenesisAccount.regid, SYMB::WICC, riskFee, ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_RISERVE);

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("%s(), write risk riserve account error, regid=%s",
            __func__, fcoinGenesisAccount.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    VoteDelegateVector delegates;
    if (!cw.delegateCache.GetActiveDelegates(delegates)) {
        return state.DoS(100, ERRORMSG("%s(), GetActiveDelegates failed", __func__),
            REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegates.size() != 0 && delegates.size() == IniCfg().GetTotalDelegateNum());

    for (size_t i = 0; i < delegates.size(); i++) {
        const CRegID &delegateRegid = delegates[i].regid;
        CAccount delegateAccount;
        if (!cw.accountCache.GetAccount(CUserID(delegateRegid), delegateAccount)) {
            return state.DoS(100, ERRORMSG("%s(), get delegate account info failed! delegate regid=%s",
                __func__, delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        uint64_t minerFee = minerTotalFee / delegates.size();
        if (i == 0) minerFee += minerTotalFee % delegates.size(); // give the dust amount to topmost miner

        if (!delegateAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, minerFee)) {
            return state.DoS(100, ERRORMSG("%s(), add %s asset fee to miner failed, miner regid=%s",
                __func__, action, delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(delegateRegid, delegateAccount))
            return state.DoS(100, ERRORMSG("%s(), write delegate account info error, delegate regid=%s",
                __func__, delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

        if (action == OPERATOR_ACTION_REGISTER)
            receipts.emplace_back(txAccount.regid, delegateRegid, SYMB::WICC, minerFee, ReceiptCode::DEX_OPERATOR_REG_FEE_TO_MINER);
        else
            receipts.emplace_back(txAccount.regid, delegateRegid, SYMB::WICC, minerFee, ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_MINER);
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////
// class CDEXOperatorRegisterTx

string CDEXOperatorRegisterTx::ToString(CAccountDBCache &accountCache) {
    // TODO: ...
    return "";
}

Object CDEXOperatorRegisterTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("owner_uid", data.owner_uid.ToString()));
    result.push_back(Pair("match_uid", data.match_uid.ToString()));
    result.push_back(Pair("dex_name", data.name));
    result.push_back(Pair("portal_url", data.portal_url));
    result.push_back(Pair("maker_fee_ratio",data.maker_fee_ratio));
    result.push_back(Pair("taker_fee_ratio",data.taker_fee_ratio));
    result.push_back(Pair("memo",data.memo));
    return result ;
}

bool CDEXOperatorRegisterTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;

    if (!data.owner_uid.is<CRegID>()) {
        return state.DoS(100, ERRORMSG("%s, owner_uid must be regid", __func__), REJECT_INVALID,
            "owner-uid-type-error");
    }

    if (!data.match_uid.is<CRegID>()) {
        return state.DoS(100, ERRORMSG("%s, match_uid must be regid", __func__), REJECT_INVALID,
            "match-uid-type-error");
    }

    if (data.name.size() > MAX_NAME_LEN) {
        return state.DoS(100, ERRORMSG("%s, name len=%d greater than %d", __func__,
            data.name.size(), MAX_NAME_LEN), REJECT_INVALID, "invalid-name");
    }

    if(data.memo.size() > MAX_COMMON_TX_MEMO_SIZE){
        return state.DoS(100, ERRORMSG("%s, memo len=%d greater than %d", __func__,
                                       data.memo.size(), MAX_COMMON_TX_MEMO_SIZE), REJECT_INVALID, "invalid-memo");
    }

    if (data.maker_fee_ratio > MAX_MATCH_FEE_RATIO_VALUE)
        return state.DoS(100, ERRORMSG("%s, maker_fee_ratio=%d is greater than %d", __func__,
            data.maker_fee_ratio, MAX_MATCH_FEE_RATIO_VALUE), REJECT_INVALID, "invalid-match-fee-ratio-type");
    if (data.taker_fee_ratio > MAX_MATCH_FEE_RATIO_VALUE)
        return state.DoS(100, ERRORMSG("%s, taker_fee_ratio=%d is greater than %d", __func__,
            data.taker_fee_ratio, MAX_MATCH_FEE_RATIO_VALUE), REJECT_INVALID, "invalid-match-fee-ratio-type");

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::CheckTx, read account failed! tx account not exist, txUid=%s",
                     txUid.ToDebugString()), REJECT_INVALID, "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}
bool CDEXOperatorRegisterTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    vector<CReceipt> receipts;
    shared_ptr<CAccount> pTxAccount = make_shared<CAccount>();
    if (pTxAccount == nullptr || !cw.accountCache.GetAccount(txUid, *pTxAccount))
        return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::ExecuteTx, read tx account by txUid=%s error",
            txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!pTxAccount->OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::ExecuteTx, insufficient funds in account to sub fees, fees=%llu, txUid=%s",
                        llFees, txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    shared_ptr<CAccount> pOwnerAccount;
    if (pTxAccount->IsMyUid(data.owner_uid)) {
        pOwnerAccount = pTxAccount;
    } else {
        pOwnerAccount = make_shared<CAccount>();
        if (pOwnerAccount == nullptr || !cw.accountCache.GetAccount(data.owner_uid, *pOwnerAccount))
            return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::ExecuteTx, read owner account failed! owner_uid=%s",
                data.owner_uid.ToDebugString()), REJECT_INVALID, "owner-account-not-exist");
    }
    shared_ptr<CAccount> pMatchAccount;
    if (!pTxAccount->IsMyUid(data.match_uid) && !pOwnerAccount->IsMyUid(data.match_uid)) {
        if (!cw.accountCache.HaveAccount(data.match_uid))
            return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::ExecuteTx, get match account failed! match_uid=%s",
                data.match_uid.ToDebugString()), REJECT_INVALID, "match-account-not-exist");
    }

    if(cw.dexCache.HaveDexOperatorByOwner(pOwnerAccount->regid))
        return state.DoS(100, ERRORMSG("%s, the owner already has a dex operator! owner_regid=%s", __func__,
            pOwnerAccount->regid.ToString()), REJECT_INVALID, "owner-had-dexoperator-already");

    if (!ProcessDexOperatorFee(cw, state, OPERATOR_ACTION_REGISTER, *pTxAccount, receipts))
        return false;

    uint32_t new_id;
    if (!cw.dexCache.IncDexID(new_id))
        return state.DoS(100, ERRORMSG("%s, increase dex id error! txUid=%s", __func__),
            UPDATE_ACCOUNT_FAIL, "inc_dex_id_error");

    DexOperatorDetail detail = {
        data.owner_uid.get<CRegID>(),
        data.match_uid.get<CRegID>(),
        data.name,
        data.portal_url,
        data.maker_fee_ratio,
        data.taker_fee_ratio,
        data.memo
    };
    if (!cw.dexCache.CreateDexOperator(new_id, detail))
        return state.DoS(100, ERRORMSG("%s, save new dex operator error! new_id=%u", __func__, new_id),
                         UPDATE_ACCOUNT_FAIL, "save-operator-error");

    if (!cw.accountCache.SetAccount(txUid, *pTxAccount))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, set tx account to db failed! txUid=%s",
            txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-set-accountdb");

    if(!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
    return true;
}

bool CDEXOperatorUpdateData::Check(string& errmsg, string& errcode,const uint32_t currentHeight ){

    if(IsEmpty()){
        errmsg = "CDEXOperatorUpdateData::check(): update data is empty" ;
        errcode= "empty-update-data" ;
        return false ;
    }

    if(field == MATCH_UID || field == OWNER_UID){
        string placeholder = (field == MATCH_UID)? "match": "owner" ;

        auto uid = CUserID::ParseUserId(value);
        if (!uid) {
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) is a invalid account",placeholder, value);
            errcode = strprintf("%s-uid-invalid", placeholder) ;
            return false ;
        }
        CAccount account ;
        if( !pCdMan->pAccountCache->GetAccount(*uid,account)){
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) is not exist! ",placeholder, value );
            errcode = strprintf("%s-uid-invalid", placeholder) ;
            return false ;
        }
        if( account.regid.IsEmpty() ||!account.IsRegistered() || !account.regid.IsMature(currentHeight)){
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) don't have regid or regid is immature ! ",placeholder, value );
            errcode = strprintf("%s-uid-invalid", placeholder) ;
            return false ;
        }
    }


    if(field == NAME && value.size() > MAX_NAME_LEN) {
           errmsg = strprintf("%s, name len=%d greater than %d", __func__,value.size(), MAX_NAME_LEN);
           errcode = "invalid-name" ;
           return false ;
    }

    if(field == MEMO && value.size() > MAX_COMMON_TX_MEMO_SIZE){
        errmsg = strprintf("%s, memo len=%d greater than %d", __func__,value.size(), MAX_COMMON_TX_MEMO_SIZE);
        errcode = "invalid-memo" ;
        return false ;
    }


    if(field == TAKER_FEE_RATIO || field == MAKER_FEE_RATIO ){
        regex  r("[0-9]+");
        if(!regex_match(value ,r )){
            errmsg = strprintf("%s, fee_ratio format is error", __func__);
            errcode = "invalid-match-fee-ratio-type" ;
            return false ;
        }

        uint64_t v = atoui64(value) ;
        if( v > MAX_MATCH_FEE_RATIO_VALUE){
            errmsg = strprintf("%s, fee_ratio=%d is greater than %d", __func__,
                               v, MAX_MATCH_FEE_RATIO_VALUE);
            errcode = "invalid-match-fee-ratio-type" ;
            return false ;
        }
    }

    return true ;

}

bool CDEXOperatorUpdateData::GetRegID(CCacheWrapper &cw,CRegID& regid) {


    auto uid = CUserID::ParseUserId(value ) ;
    if((*uid).is<CRegID>()){
        regid =  (*uid).get<CRegID>() ;
        return true;
    }

    CAccount account ;
    if(cw.accountCache.GetAccount(*uid,account) && !account.regid.IsEmpty()){
        regid = account.regid;
        return true ;
    }
    return false ;
}

bool CDEXOperatorUpdateData::UpdateToDexOperator(DexOperatorDetail& detail,CCacheWrapper& cw) {
    if(field == MATCH_UID ) {
        CRegID regid ;
        bool res = GetRegID(cw ,regid) ;
        if(res){
            detail.match_regid = regid ;
        }
        return  res ;
    } else if(field == NAME ) {
        detail.name = value;
    } else if(field == PORTAL_URL) {
        detail.portal_url = value ;
    } else if(field == TAKER_FEE_RATIO ) {
        detail.taker_fee_ratio = atoui64(value);
    } else if(field == MAKER_FEE_RATIO) {
        detail.maker_fee_ratio = atoui64(value);
    } else if(field == MEMO) {
        detail.memo = value ;
    } else if( field == OWNER_UID) {
        CRegID regid ;
        bool res = GetRegID(cw ,regid) ;
        if(res){
            detail.owner_regid = regid ;
        }
        return  res ;
    }

    return true ;

}

string CDEXOperatorUpdateTx::ToString(CAccountDBCache &accountCache) {

    return "" ;
}

Object CDEXOperatorUpdateTx::ToJson(const CAccountDBCache &accountCache) const {

    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("update_field", update_data.field)) ;
    result.push_back(Pair("update_value", update_data.value)) ;
    result.push_back(Pair("dex_id", update_data.dexId));
    return result;
}
bool CDEXOperatorUpdateTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;

    string errmsg ;
    string errcode ;
    if(!update_data.Check(errmsg ,errcode, context.height )){
        return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::CheckTx, %s",errmsg), REJECT_INVALID, errcode);
    }

    if(update_data.field == CDEXOperatorUpdateData::OWNER_UID){
        if(cw.dexCache.HaveDexOperatorByOwner(CRegID(update_data.value)))
            return state.DoS(100, ERRORMSG("%s, the owner already has a dex operator! owner_regid=%s", __func__,
                                           update_data.value), REJECT_INVALID, "owner-had-dexoperator");
    }

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("CDEXOperatorUpdateTx::CheckTx, read account failed! tx account not exist, txUid=%s",
                                       txUid.ToDebugString()), REJECT_INVALID, "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true ;
}

bool CDEXOperatorUpdateTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    vector<CReceipt> receipts;
    shared_ptr<CAccount> pTxAccount = make_shared<CAccount>();
    if (pTxAccount == nullptr || !cw.accountCache.GetAccount(txUid, *pTxAccount))
        return state.DoS(100, ERRORMSG("CDEXOperatorUpdateTx::ExecuteTx, read tx account by txUid=%s error",
                                       txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!pTxAccount->OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXOperatorUpdateTx::ExecuteTx, insufficient funds in account to sub fees, fees=%llu, txUid=%s",
                                       llFees, txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    DexOperatorDetail oldDetail  ;
    if(!cw.dexCache.GetDexOperator((DexID)update_data.dexId, oldDetail)){
        return state.DoS(100, ERRORMSG("CDEXOperatorUpdateTx::ExecuteTx, the dexoperator( id= %u) is not exist!",
                                       update_data.dexId), UPDATE_ACCOUNT_FAIL, "dexoperator-not-exist");
    }

    if (!pTxAccount->IsMyUid(oldDetail.owner_regid)) {
        return state.DoS(100, ERRORMSG("CDEXOperatorUpdateTx::ExecuteTx, only owner can update dexoperator!， owner_regid=%s, txUid=%s, dexId=%u",
                                       oldDetail.owner_regid.ToString(),txUid.ToString(), update_data.dexId),
                                               UPDATE_ACCOUNT_FAIL, "dexoperator-update-permession-deny");
    }





    /* if (!ProcessDexOperatorFee(cw, state, OPERATOR_ACTION_REGISTER, *pTxAccount, receipts))
         return false;*/

    DexOperatorDetail detail = {
            oldDetail.owner_regid,
            oldDetail.match_regid,
            oldDetail.name,
            oldDetail.portal_url,
            oldDetail.maker_fee_ratio,
            oldDetail.taker_fee_ratio,
            oldDetail.memo
    };
    if(!update_data.UpdateToDexOperator(detail,cw) ){
        return state.DoS(100, ERRORMSG("%s, copy updated dex operator error! dex_id=%u", __func__, update_data.dexId),
                         UPDATE_ACCOUNT_FAIL, "copy-updated-operator-error");
    }

    if (!cw.dexCache.UpdateDexOperator(update_data.dexId, oldDetail, detail))
        return state.DoS(100, ERRORMSG("%s, save updated dex operator error! dex_id=%u", __func__, update_data.dexId),
                         UPDATE_ACCOUNT_FAIL, "save-updated-operator-error");

    if (!cw.accountCache.SetAccount(txUid, *pTxAccount))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, set tx account to db failed! txUid=%s",
                                       txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-set-accountdb");

    if(!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, set tx receipts failed!! txid=%s",
                                       GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
    return true;
}