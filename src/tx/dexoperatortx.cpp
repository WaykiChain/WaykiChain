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
    CAccount &txAccount, vector<CReceipt> &receipts,uint32_t currHeight) {

    uint64_t exchangeFee = 0;
    if (action == OPERATOR_ACTION_REGISTER) {
        if (!cw.sysParamCache.GetParam(DEX_OPERATOR_REGISTER_FEE, exchangeFee))
            return state.DoS(100, ERRORMSG("read param DEX_OPERATOR_REGISTER_FEE error"),
                            REJECT_INVALID, "read-sysparam-error");
    } else {
        assert(action == OPERATOR_ACTION_UPDATE);
        if (!cw.sysParamCache.GetParam(DEX_OPERATOR_UPDATE_FEE, exchangeFee))
            return state.DoS(100, ERRORMSG("read param DEX_OPERATOR_UPDATE_FEE error"),
                            REJECT_INVALID, "read-sysparam-error");
    }

    if (!txAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, exchangeFee,
                                ReceiptCode::DEX_ASSET_FEE_TO_SETTLER, receipts))
        return state.DoS(100, ERRORMSG("tx account insufficient funds for operator %s fee! fee=%llu, tx_addr=%s",
                        action, exchangeFee, txAccount.keyid.ToAddress()),
                        UPDATE_ACCOUNT_FAIL, "insufficent-funds");

    uint64_t dexOperatorRiskFeeRatio;
    if(!cw.sysParamCache.GetParam(SysParamType::DEX_OPERATOR_RISK_FEE_RATIO, dexOperatorRiskFeeRatio)) {
        return state.DoS(100, ERRORMSG("ProcessDexOperatorFee, get dexOperatorRiskFeeRatio error",
                                       action, exchangeFee, txAccount.regid.ToString()), READ_SYS_PARAM_FAIL, "read-db-error");
    }
    uint64_t riskFee       = exchangeFee * dexOperatorRiskFeeRatio / RATIO_BOOST;
    uint64_t minerTotalFee = exchangeFee - riskFee;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("get risk reserve account failed"),
                        READ_ACCOUNT_FAIL, "get-account-failed");

    ReceiptCode code = (action == OPERATOR_ACTION_REGISTER) ? ReceiptCode::DEX_OPERATOR_REG_FEE_TO_RESERVE :
                                                              ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_RESERVE;

    if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, riskFee, code, receipts)) {
        return state.DoS(100, ERRORMSG("operate balance failed! add %s asset fee=%llu to risk reserve account error",
                        action, riskFee), UPDATE_ACCOUNT_FAIL, "update-account-failed");
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("write risk reserve account error, regid=%s",
                        fcoinGenesisAccount.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    VoteDelegateVector delegates;
    if (!cw.delegateCache.GetActiveDelegates(delegates)) {
        return state.DoS(100, ERRORMSG("GetActiveDelegates failed"), REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegates.size() != 0 );

    for (size_t i = 0; i < delegates.size(); i++) {
        const CRegID &delegateRegid = delegates[i].regid;
        CAccount delegateAccount;
        if (!cw.accountCache.GetAccount(CUserID(delegateRegid), delegateAccount)) {
            return state.DoS(100, ERRORMSG("get delegate account info failed! delegate regid=%s",
                            delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        uint64_t minerFee = minerTotalFee / delegates.size();
        if (i == 0) minerFee += minerTotalFee % delegates.size(); // give the dust amount to topmost miner

        ReceiptCode code = (action == OPERATOR_ACTION_REGISTER) ? ReceiptCode::DEX_OPERATOR_REG_FEE_TO_MINER :
                            ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_MINER;

        if (!delegateAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, minerFee, code, receipts)) {
            return state.DoS(100, ERRORMSG("add %s asset fee to miner failed, miner regid=%s",
                            action, delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(delegateRegid, delegateAccount))
            return state.DoS(100, ERRORMSG("write delegate account info error, delegate regid=%s",
                            delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

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
    result.push_back(Pair("fee_receiver_uid", data.fee_receiver_uid.ToString()));
    result.push_back(Pair("dex_name", data.name));
    result.push_back(Pair("portal_url", data.portal_url));
    result.push_back(Pair("maker_fee_ratio",data.maker_fee_ratio));
    result.push_back(Pair("taker_fee_ratio",data.taker_fee_ratio));
    result.push_back(Pair("memo",data.memo));
    return result;
}

bool CDEXOperatorRegisterTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    if (!data.owner_uid.is<CRegID>())
        return state.DoS(100, ERRORMSG("owner_uid must be regid"), REJECT_INVALID,
            "owner-uid-type-error");

    if (!data.fee_receiver_uid.is<CRegID>())
        return state.DoS(100, ERRORMSG("fee_receiver_uid must be regid"), REJECT_INVALID,
            "match-uid-type-error");

    if (data.name.size() > MAX_NAME_LEN)
        return state.DoS(100, ERRORMSG("name len=%d greater than %d",
            data.name.size(), MAX_NAME_LEN), REJECT_INVALID, "invalid-name");

    if(data.memo.size() > MAX_COMMON_TX_MEMO_SIZE)
        return state.DoS(100, ERRORMSG("memo len=%d greater than %d",
                                       data.memo.size(), MAX_COMMON_TX_MEMO_SIZE), REJECT_INVALID, "invalid-memo");

    if (data.maker_fee_ratio > MAX_MATCH_FEE_RATIO_VALUE)
        return state.DoS(100, ERRORMSG("maker_fee_ratio=%d is greater than %d",
            data.maker_fee_ratio, MAX_MATCH_FEE_RATIO_VALUE), REJECT_INVALID, "invalid-match-fee-ratio-type");

    if (data.taker_fee_ratio > MAX_MATCH_FEE_RATIO_VALUE)
        return state.DoS(100, ERRORMSG("taker_fee_ratio=%d is greater than %d",
            data.taker_fee_ratio, MAX_MATCH_FEE_RATIO_VALUE), REJECT_INVALID, "invalid-match-fee-ratio-type");

    return true;
}

bool CDEXOperatorRegisterTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    CAccount ownerAccount;
    if (txAccount.IsSelfUid(data.owner_uid)) {
        ownerAccount = txAccount;
    } else {
        if (!cw.accountCache.GetAccount(data.owner_uid, ownerAccount))
            return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::ExecuteTx, read owner account failed! owner_uid=%s",
                data.owner_uid.ToDebugString()), REJECT_INVALID, "owner-account-not-exist");
    }
    shared_ptr<CAccount> pMatchAccount;
    if (!txAccount.IsSelfUid(data.fee_receiver_uid) && !ownerAccount.IsSelfUid(data.fee_receiver_uid)) {
        if (!cw.accountCache.HasAccount(data.fee_receiver_uid))
            return state.DoS(100, ERRORMSG("CDEXOperatorRegisterTx::ExecuteTx, get match account failed! fee_receiver_uid=%s",
                data.fee_receiver_uid.ToDebugString()), REJECT_INVALID, "match-account-not-exist");
    }

    if (cw.dexCache.HasDexOperatorByOwner(ownerAccount.regid))
        return state.DoS(100, ERRORMSG("the owner already has a dex operator! owner_regid=%s",
                        ownerAccount.regid.ToString()), REJECT_INVALID, "owner-had-dexoperator-already");

    if (!ProcessDexOperatorFee(cw, state, OPERATOR_ACTION_REGISTER, txAccount, receipts,context.height))
        return false;

    uint32_t new_id;
    if (!cw.dexCache.IncDexID(new_id))
        return state.DoS(100, ERRORMSG("increase dex id error! txUid=%s", GetHash().ToString() ),
            UPDATE_ACCOUNT_FAIL, "inc_dex_id_error");

    DexOperatorDetail detail = {
        data.owner_uid.get<CRegID>(),
        data.fee_receiver_uid.get<CRegID>(),
        data.name,
        data.portal_url,
        data.public_mode,
        data.maker_fee_ratio,
        data.taker_fee_ratio,
        data.memo
    };

    if (!cw.dexCache.CreateDexOperator(new_id, detail))
        return state.DoS(100, ERRORMSG("save new dex operator error! new_id=%u", new_id),
                         UPDATE_ACCOUNT_FAIL, "save-operator-error");

    return true;
}

bool CDEXOperatorUpdateData::Check(string& errmsg, string& errcode,const uint32_t currentHeight ){

    if(field == UPDATE_NONE || field > MEMO ){
        errmsg = "CDEXOperatorUpdateData::check(): update field is error";
        errcode= "empty-update-data";
        return false;
    }

    if (field == FEE_RECEIVER_UID || field == OWNER_UID){
        string placeholder = (field == FEE_RECEIVER_UID)? "fee_receiver": "owner";

        auto uid = std::make_shared<CUserID>(get<CUserID>());
        if (!uid) {
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) is a invalid account",placeholder, ValueToString());
            errcode = strprintf("%s-uid-invalid", placeholder);
            return false;
        }
        CAccount account;
        if (!pCdMan->pAccountCache->GetAccount(*uid,account)){
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) is not exist! ",placeholder, ValueToString());
            errcode = strprintf("%s-uid-invalid", placeholder);
            return false;
        }
        if (account.regid.IsEmpty() ||!account.IsRegistered() || !account.regid.IsMature(currentHeight)) {
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) don't have regid or regid is immature ! ",placeholder, ValueToString() );
            errcode = strprintf("%s-uid-invalid", placeholder);
            return false;
        }
    }


    if (field == NAME && get<string>().size() > MAX_NAME_LEN) {
           errmsg = strprintf("%s, name len=%d greater than %d", __func__,get<string>().size(), MAX_NAME_LEN);
           errcode = "invalid-name";
           return false;
    }

    if (field == MEMO && get<string>().size() > MAX_COMMON_TX_MEMO_SIZE){
        errmsg = strprintf("%s, memo len=%d greater than %d", __func__,get<string>().size(), MAX_COMMON_TX_MEMO_SIZE);
        errcode = "invalid-memo";
        return false;
    }


    if(field == TAKER_FEE_RATIO || field == MAKER_FEE_RATIO ){

        uint64_t v = get<uint64_t>();
        if( v > MAX_MATCH_FEE_RATIO_VALUE){
            errmsg = strprintf("%s, fee_ratio=%d is greater than %d", __func__,
                               v, MAX_MATCH_FEE_RATIO_VALUE);
            errcode = "invalid-match-fee-ratio-type";
            return false;
        }
    }

    return true;

}

bool CDEXOperatorUpdateData::UpdateToDexOperator(DexOperatorDetail& detail,CCacheWrapper& cw) {

    switch (field) {
        case FEE_RECEIVER_UID:{
            if(get<CUserID>().is<CRegID>()) {
                detail.fee_receiver_regid = get<CUserID>().get<CRegID>();
                return true;
            } else{
                return false;
            }
        }
        case OWNER_UID:{
            if(get<CUserID>().is<CRegID>()){
                detail.owner_regid = get<CUserID>().get<CRegID>();
                return true;
            } else{
                return false;
            }
        }
        case NAME:
            detail.name = get<string>();
            break;
        case PORTAL_URL:
            detail.portal_url = get<string>();
            break;
        case TAKER_FEE_RATIO:
            detail.taker_fee_ratio = get<uint64_t>();
            break;
        case MAKER_FEE_RATIO:
            detail.maker_fee_ratio = get<uint64_t>();
            break;
        case MEMO:
            detail.memo =get<string>();
            break;

        default:
            return false;

    }

    return true;

}

string CDEXOperatorUpdateTx::ToString(CAccountDBCache &accountCache) {

    return "";
}

Object CDEXOperatorUpdateTx::ToJson(const CAccountDBCache &accountCache) const {

    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("update_field", update_data.field));
    result.push_back(Pair("update_value", update_data.ValueToString()));
    result.push_back(Pair("dex_id", update_data.dexId));
    return result;
}
bool CDEXOperatorUpdateTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    string errmsg ;
    string errcode ;
    if(!update_data.Check(errmsg ,errcode, context.height )){
        return state.DoS(100, ERRORMSG(errmsg), REJECT_INVALID, errcode);
    }

    if(update_data.field == CDEXOperatorUpdateData::OWNER_UID){
        if(cw.dexCache.HasDexOperatorByOwner(CRegID(update_data.ValueToString())))
            return state.DoS(100, ERRORMSG("the owner already has a dex operator! owner_regid=%s",
                                           update_data.ValueToString()), REJECT_INVALID, "owner-had-dexoperator");
    }

    return true ;
}

bool CDEXOperatorUpdateTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    DexOperatorDetail oldDetail  ;
    if (!cw.dexCache.GetDexOperator((DexID)update_data.dexId, oldDetail))
        return state.DoS(100, ERRORMSG("the dexoperator( id= %u) is not exist!",
                                       update_data.dexId), UPDATE_ACCOUNT_FAIL, "dexoperator-not-exist");

    if (!txAccount.IsSelfUid(oldDetail.owner_regid))
        return state.DoS(100, ERRORMSG("only owner can update dexoperator! owner_regid=%s, txUid=%s, dexId=%u",
                                       oldDetail.owner_regid.ToString(),txUid.ToString(), update_data.dexId),
                                               UPDATE_ACCOUNT_FAIL, "dexoperator-update-permession-deny");

    if (!ProcessDexOperatorFee(cw, state, OPERATOR_ACTION_UPDATE, txAccount, receipts, context.height))
         return false;

    DexOperatorDetail detail = oldDetail;
    if (!update_data.UpdateToDexOperator(detail, cw))
        return state.DoS(100, ERRORMSG("copy updated dex operator error! dex_id=%u", update_data.dexId),
                         UPDATE_ACCOUNT_FAIL, "copy-updated-operator-error");

    if (!cw.dexCache.UpdateDexOperator(update_data.dexId, oldDetail, detail))
        return state.DoS(100, ERRORMSG("save updated dex operator error! dex_id=%u", update_data.dexId),
                         UPDATE_ACCOUNT_FAIL, "save-updated-operator-error");

    return true;
}
