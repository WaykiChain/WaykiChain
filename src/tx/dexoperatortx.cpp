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
static const uint64_t ORDER_OPEN_DEXOP_LIST_SIZE_MAX = 500;

static bool ProcessDexOperatorFee(CBaseTx &tx, CTxExecuteContext& context, const string &action) {

    IMPLEMENT_DEFINE_CW_STATE;
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

    if (!tx.sp_tx_account->OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, exchangeFee,
                                ReceiptType::DEX_ASSET_FEE_TO_OPERATOR, tx.receipts))
        return state.DoS(100, ERRORMSG("tx account insufficient funds for operator %s fee! fee=%llu, tx_addr=%s",
                        action, exchangeFee, tx.sp_tx_account->keyid.ToAddress()),
                        UPDATE_ACCOUNT_FAIL, "insufficent-funds");

    uint64_t dexOperatorRiskFeeRatio;
    if(!cw.sysParamCache.GetParam(SysParamType::DEX_OPERATOR_RISK_FEE_RATIO, dexOperatorRiskFeeRatio)) {
        return state.DoS(100, ERRORMSG("ProcessDexOperatorFee, get dexOperatorRiskFeeRatio error"),
                READ_SYS_PARAM_FAIL, "read-db-error");
    }
    uint64_t riskFee       = exchangeFee * dexOperatorRiskFeeRatio / RATIO_BOOST;
    uint64_t minerTotalFee = exchangeFee - riskFee;

        auto spFcoinAccount = tx.GetAccount(context, SysCfg().GetFcoinGenesisRegId(), "fcoin");
        if (!spFcoinAccount) return false;

    ReceiptType code = (action == OPERATOR_ACTION_REGISTER) ? ReceiptType::DEX_OPERATOR_REG_FEE_TO_RESERVE :
                                                              ReceiptType::DEX_OPERATOR_UPDATED_FEE_TO_RESERVE;

    if (!spFcoinAccount->OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, riskFee, code, tx.receipts)) {
        return state.DoS(100, ERRORMSG("operate balance failed! add %s asset fee=%llu to risk reserve account error",
                        action, riskFee), UPDATE_ACCOUNT_FAIL, "update-account-failed");
    }

    VoteDelegateVector delegates;
    if (!cw.delegateCache.GetActiveDelegates(delegates)) {
        return state.DoS(100, ERRORMSG("GetActiveDelegates failed"), REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegates.size() != 0 );

    for (size_t i = 0; i < delegates.size(); i++) {
        const CRegID &delegateRegid = delegates[i].regid;
        auto spDelegateAccount = tx.GetAccount(context, delegateRegid, "delegate_regid");
        if (!spDelegateAccount) return false;
        uint64_t minerFee = minerTotalFee / delegates.size();
        if (i == 0) minerFee += minerTotalFee % delegates.size(); // give the dust amount to topmost miner

        ReceiptType code = (action == OPERATOR_ACTION_REGISTER) ? ReceiptType::DEX_OPERATOR_REG_FEE_TO_MINER :
                            ReceiptType::DEX_OPERATOR_UPDATED_FEE_TO_MINER;

        if (!spDelegateAccount->OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, minerFee, code, tx.receipts)) {
            return state.DoS(100, ERRORMSG("add %s asset fee to miner failed, miner regid=%s",
                            action, delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        }
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
// class CDEXOperatorRegisterTx



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

    if (data.order_open_dexop_list.size() > ORDER_OPEN_DEXOP_LIST_SIZE_MAX)
        return state.DoS(100, ERRORMSG("size=%u of order_open_dexop_list exceed max=%u",
            data.order_open_dexop_list.size(), ORDER_OPEN_DEXOP_LIST_SIZE_MAX),
            REJECT_INVALID, "invalid-shared-dexop-list-size");
    return true;
}

bool CDEXOperatorRegisterTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    DexOpIdValueSet dexIdSet;
    for (auto &item : data.order_open_dexop_list) {
        auto ret = dexIdSet.insert(item);
        if (!ret.second) {
            return state.DoS(100, ERRORMSG("duplicated item=%s in  order_open_dexop_list",
                db_util::ToString(item)), REJECT_INVALID, "duplicated-item-in-shared-dexop-list");
        }
    }

    shared_ptr<CAccount> spOwnerAccount;
    if (sp_tx_account->IsSelfUid(data.owner_uid)) {
        spOwnerAccount = sp_tx_account;
    } else {
        spOwnerAccount = GetAccount(context, data.owner_uid, "owner_uid");
        if (!spOwnerAccount) return false;
    }
    shared_ptr<CAccount> pMatchAccount;
    if (!sp_tx_account->IsSelfUid(data.fee_receiver_uid) && !sp_tx_account->IsSelfUid(data.fee_receiver_uid)) {
        pMatchAccount = GetAccount(context, data.fee_receiver_uid, "fee_receiver_uid");
        if (!pMatchAccount) return false;
    }

    if (cw.dexCache.HasDexOperatorByOwner(spOwnerAccount->regid))
        return state.DoS(100, ERRORMSG("the owner's operator exist! owner_regid=%s",
                        spOwnerAccount->regid.ToString()), REJECT_INVALID, "owner-dexoperator-exist");

    if (!ProcessDexOperatorFee(*this, context, OPERATOR_ACTION_REGISTER))
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
        data.order_open_mode,
        data.maker_fee_ratio,
        data.taker_fee_ratio,
        dexIdSet,
        data.memo
    };

    if (!cw.dexCache.CreateDexOperator(new_id, detail))
        return state.DoS(100, ERRORMSG("save new dex operator error! new_id=%u", new_id),
                         UPDATE_ACCOUNT_FAIL, "save-operator-error");

    return true;
}

bool CDEXOperatorUpdateData::Check(CBaseTx &tx, CCacheWrapper &cw, string& errmsg, string& errcode,const uint32_t currentHeight){

    DexOperatorDetail dex;

    if(!cw.dexCache.GetDexOperator(dexId,dex)) {
        errmsg = strprintf("dexoperator(id=%d) not found", dexId);
        errcode = "invalid-dex-id";
        return false;
    }

    if(field == UPDATE_NONE || field > MEMO ){
        errmsg = "CDEXOperatorUpdateData::check(): update field is error";
        errcode= "empty-update-data";
        return false;
    }

    if (field == FEE_RECEIVER_UID || field == OWNER_UID){
        string placeholder = (field == FEE_RECEIVER_UID)? "fee_receiver": "owner";

        const auto &uid = get<CUserID>();

        auto spAccount = tx.GetAccount(cw, uid);
        if (!spAccount) {
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) is not exist! ",placeholder, ValueToString());
            errcode = strprintf("%s-uid-invalid", placeholder);
            return false;
        }
        if (!spAccount->IsRegistered() || !spAccount->regid.IsMature(currentHeight)) {
            errmsg = strprintf("CDEXOperatorUpdateData::check(): %s_uid (%s) not register or regid is immature ! ",placeholder, ValueToString() );
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


    if (field == TAKER_FEE_RATIO || field == MAKER_FEE_RATIO ){

        uint64_t v = get<uint64_t>();
        if( v > MAX_MATCH_FEE_RATIO_VALUE){
            errmsg = strprintf("%s, fee_ratio=%d is greater than %d", __func__,
                               v, MAX_MATCH_FEE_RATIO_VALUE);
            errcode = "invalid-match-fee-ratio-type";
            return false;
        }
    }

    if (field == OPEN_MODE) {
        dex::OpenMode  om = get<dex::OpenMode>();
        if (om != dex::OpenMode::PRIVATE && om != dex::OpenMode::PUBLIC) {
            errmsg = strprintf("dex open mode value(%d) is error", (uint8_t)om);
            errcode = "invalid-open-mode";
            return false;
        }

        if (om == dex.order_open_mode) {
            errmsg = strprintf("the new dex open mode value(%d) is as same as old open mode", (uint8_t)om);
            errcode = "same-open-mode";
            return false;
        }
    }

    if (field == ORDER_OPEN_DEXOP_LIST) {
       auto dexlist = get<DexOpIdValueList>();
       for (auto dexOpId: dexlist) {
           if (!cw.dexCache.HaveDexOperator(dexOpId.get())) {
                errmsg = strprintf("dex(id=%d) is not exist!!", dexOpId.get());
                errcode = "invalid-dexid";
                return false;
           }
           if( dexOpId.get() == dexId) {
               errmsg = "the open dexop list can't contains self";
               errcode = "self-dexid-error";
               return false;
           }
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
            detail.memo = get<string>();
            break;
        case OPEN_MODE:
            detail.order_open_mode = get<dex::OpenMode>();
            break;
        case ORDER_OPEN_DEXOP_LIST:{
            auto dexIdList = get<DexOpIdValueList>();
            DexOpIdValueSet dexIdSet;
            for (auto dexId: dexIdList) {
                dexIdSet.insert(dexId);
            }
            detail.order_open_dexop_set = dexIdSet;
            break;
        }
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

    string errmsg;
    string errcode;
    if(!update_data.Check(*this, cw, errmsg ,errcode, context.height )){
        return state.DoS(100, ERRORMSG("%s", errmsg), REJECT_INVALID, errcode);
    }

    if(update_data.field == CDEXOperatorUpdateData::OWNER_UID){
        if(cw.dexCache.HasDexOperatorByOwner(CRegID(update_data.ValueToString())))
            return state.DoS(100, ERRORMSG("the owner already has a dex operator! owner_regid=%s",
                                           update_data.ValueToString()), REJECT_INVALID, "owner-had-dexoperator");
    }

    return true;
}

bool CDEXOperatorUpdateTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    DexOperatorDetail oldDetail;
    if (!cw.dexCache.GetDexOperator((DexID)update_data.dexId, oldDetail))
        return state.DoS(100, ERRORMSG("the dexoperator( id= %u) is not exist!",
                                       update_data.dexId), UPDATE_ACCOUNT_FAIL, "dexoperator-not-exist");

    if (!sp_tx_account->IsSelfUid(oldDetail.owner_regid))
        return state.DoS(100, ERRORMSG("only owner can update dexoperator! owner_regid=%s, txUid=%s, dexId=%u",
                                       oldDetail.owner_regid.ToString(),txUid.ToString(), update_data.dexId),
                                               UPDATE_ACCOUNT_FAIL, "dexoperator-update-permession-deny");

    if (!ProcessDexOperatorFee(*this, context, OPERATOR_ACTION_UPDATE))
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
