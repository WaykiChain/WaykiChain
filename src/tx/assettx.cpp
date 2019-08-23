// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assettx.h"

#include "config/const.h"
#include "main.h"
#include "entities/receipt.h"
#include "persistence/assetdb.h"

bool CAssetIssueTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;

    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    auto symbolErr = CAsset::CheckSymbol(asset.symbol);
    if (symbolErr) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, invlid asset symbol! %s", symbolErr),
            REJECT_INVALID, "invalid-asset-symobl");
    }

    if (asset.name.empty() || asset.name.size() > MAX_ASSET_NAME_LEN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_name is empty or len=%d greater than %d",
            asset.name.size(), MAX_ASSET_NAME_LEN), REJECT_INVALID, "invalid-asset-name");
    }

    if (asset.total_supply == 0 || asset.total_supply > MAX_ASSET_TOTAL_SUPPLY) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset total_supply=%llu can not == 0 or > %llu",
            asset.total_supply, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-total-supply");
    }

    if (asset.owner_uid.type() == typeid(CRegID) && !cw.accountCache.RegIDIsMature(asset.owner_uid.get<CRegID>())) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, owner regid=%s not mature yet",
            asset.owner_uid.get<CRegID>().ToString()), REJECT_INVALID, "asset-owner-regid-not-mature");
    }

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !account.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : account.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CAssetIssueTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, read source txUid %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, insufficient funds in account to sub fees, fees=%llu, txUid=%s",
                        llFees, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    if (cw.assetCache.HaveAsset(asset.symbol))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, the asset has been issued! symbol=%s",
            asset.symbol), REJECT_INVALID, "asset-existed-error");

    uint64_t assetIssueFee; //550 WICC
    if (!cw.sysParamCache.GetParam(ASSET_ISSUE_FEE, assetIssueFee)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, read param ASSET_ISSUE_FEE error"),
                         REJECT_INVALID, "read-sysparam-error");
    }
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, assetIssueFee)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, insufficient funds in account to sub issued_fee=%llu, txUid=%s",
                        assetIssueFee, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }
    vector<CRegID> delegateList;
    if (!cw.delegateCache.GetTopDelegateList(delegateList)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, get top delegate list failed",
            txUid.ToString()), REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegateList.size() != 0 && delegateList.size() == IniCfg().GetTotalDelegateNum());

    for (size_t i =0; i < delegateList.size(); i++) {
        const CRegID &delegateRegid = delegateList[i];
        CAccount delegateAccount;
        if (!cw.accountCache.GetAccount(CUserID(delegateRegid), delegateAccount)) {
            return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, get delegate account info failed! delegate regid=%s",
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        uint64_t minerIssuedFee = assetIssueFee / delegateList.size();
        if (i == 0) minerIssuedFee += assetIssueFee % delegateList.size(); // give the dust amount to first delegate account

        if (!delegateAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, minerIssuedFee)) {
            return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, add issued fee to delegate account failed, delegate regid=%s",
                            delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(delegateRegid, delegateAccount))
            return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write delegate account info error, delegate regid=%s",
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-set-accountdb");
    }

    if (!account.OperateBalance(asset.symbol, BalanceOpType::ADD_FREE, asset.total_supply)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, fail to add total_supply to issued account! total_supply=%llu, txUid=%s",
                        asset.total_supply, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    if (!cw.accountCache.SetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, set tx account to db failed! txUid=%s",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-set-accountdb");

    if (!cw.assetCache.SaveAsset(asset))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, save asset failed! txUid=%s",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "save-asset-failed");

    return true;
}

string CAssetIssueTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, valid_height=%d\n"
        "owner_uid=%s, asset_symbol=%s, asset_name=%s, total_supply=%llu, mintable=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), llFees, valid_height,
        asset.owner_uid.ToString(), asset.symbol, asset.name, asset.total_supply, asset.mintable);
}

Object CAssetIssueTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("owner_uid",    asset.owner_uid.ToString()));
    result.push_back(Pair("asset_symbol",   asset.symbol));
    result.push_back(Pair("asset_name",     asset.name));
    result.push_back(Pair("total_supply",   asset.total_supply));
    result.push_back(Pair("mintable",       asset.mintable));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CAssetUpdateData

const EnumTypeMap<CAssetUpdateData::UpdateType, string> CAssetUpdateData::ASSET_UPDATE_TYPE_NAMES = {
    {OWNER_UID, "owner_uid"},
    {NAME, "name"},
    {MINT_AMOUNT, "mint_amount"}
};

shared_ptr<CAssetUpdateData::UpdateType> CAssetUpdateData::ParseUpdateType(const string& str) {
    if (str.empty()) return nullptr;
    for (auto item : ASSET_UPDATE_TYPE_NAMES) {
        if (item.second == str)
            return make_shared<UpdateType>(item.first);
    }
    return nullptr;
}

const string& CAssetUpdateData::GetUpdateTypeName(UpdateType type) {
    auto it = ASSET_UPDATE_TYPE_NAMES.find(type);
    if (it != ASSET_UPDATE_TYPE_NAMES.end()) return it->second;
    return EMPTY_STRING;
}
void CAssetUpdateData::Set(const CUserID &ownerUid) {
    type = OWNER_UID;
    value = ownerUid;
}

void CAssetUpdateData::Set(const string &name) {
    type = NAME;
    value = name;

}
void CAssetUpdateData::Set(const uint64_t &mintAmount) {
    type = MINT_AMOUNT;
    value = mintAmount;

}

string CAssetUpdateData::ValueToString() const {
    string s;
    switch (type) {
        case OWNER_UID:     s += get<CUserID>().ToString(); break;
        case NAME:          s += get<string>(); break;
        case MINT_AMOUNT:   s += std::to_string(get<uint64_t>()); break;
        default: break;
    }
    return s;
}

string CAssetUpdateData::ToString() const {
    string s = "update_type=" + GetUpdateTypeName(type);
    s += ", update_value=" + ValueToString();
    return s;
}

Object CAssetUpdateData::ToJson() const {
    Object result;
    result.push_back(Pair("update_type",   GetUpdateTypeName(type)));
    result.push_back(Pair("update_value",  ValueToString()));
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CAssetUpdateTx

string CAssetUpdateTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%ld, nValidHeight=%d,\n"
        "asset_symbol=%s, %s",
        GetTxType(nTxType), GetHash().ToString(), nVersion, fee_symbol, txUid.ToString(), llFees, nValidHeight,
        asset_symbol, update_data.ToString());
}

Object CAssetUpdateTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("asset_symbol",   asset_symbol));
    Object dataObj = update_data.ToJson();
    result.insert(result.end(), dataObj.begin(), dataObj.end());

    return result;
}

bool CAssetUpdateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    if (update_data.GetType() == CAssetUpdateData::OWNER_UID) {
        if (!AddInvolvedKeyIds({txUid, update_data.get<CUserID>()}, cw, keyIds))
            return false;
    }
    return AddInvolvedKeyIds({txUid}, cw, keyIds);
}

bool CAssetUpdateTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {

    IMPLEMENT_CHECK_TX_FEE;

    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (asset_symbol.empty() || asset_symbol.size() > MAX_TOKEN_SYMBOL_LEN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_symbol is empty or len=%d greater than %d",
            asset_symbol.size(), MAX_TOKEN_SYMBOL_LEN), REJECT_INVALID, "invalid-asset-symobl");
    }

    switch (update_data.GetType()) {
        case CAssetUpdateData::OWNER_UID: {
            const CUserID &newOwnerUid = update_data.get<CUserID>();
            if (newOwnerUid.IsEmpty())
                return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, invalid updated owner user id=%s",
                    newOwnerUid.ToString()), REJECT_INVALID, "invalid-owner-uid");
            break;
        }
        case CAssetUpdateData::NAME: {
            const string &name = update_data.get<string>();
            if (name.empty() || name.size() > MAX_ASSET_NAME_LEN)
                return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, asset name is empty or len=%d greater than %d",
                    name.size(), MAX_ASSET_NAME_LEN), REJECT_INVALID, "invalid-asset-name");
            break;
        }
        case CAssetUpdateData::MINT_AMOUNT: {
            uint64_t mintAmount = update_data.get<uint64_t>();
            if (mintAmount == 0 || mintAmount > MAX_ASSET_TOTAL_SUPPLY) {
                return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, asset mint_amount=%llu is 0 or greater than %llu",
                    mintAmount, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-mint-amount");
            }
            break;
        }
        default: {
            return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, unsupported updated_type=%d",
                update_data.GetType()), REJECT_INVALID, "invalid-update-type");
        }
    }

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !account.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : account.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CAssetUpdateTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, read source txUid %s account info error",
            txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    CAsset asset;
    if (!cw.assetCache.GetAsset(asset_symbol, asset))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, get asset by symbol=%s failed",
            asset_symbol), REJECT_INVALID, "get-asset-failed");

    if (!account.IsMyUid(asset.owner_uid))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, no privilege to update asset, uid dismatch,"
            " txUid=%s, old_asset_uid=%s",
            txUid.ToString(), asset.owner_uid.ToString()), REJECT_INVALID, "asset-uid-dismatch");

    if (!asset.mintable)
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, the asset is not mintable"),
                    REJECT_INVALID, "asset-not-mintable");


    switch (update_data.GetType()) {
        case CAssetUpdateData::OWNER_UID: {
            const CUserID &newOwnerUid = update_data.get<CUserID>();
            if (!account.IsMyUid(newOwnerUid)) {
                CAccount newAccount;
                if (!cw.accountCache.GetAccount(newOwnerUid, newAccount))
                    return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, the new owner uid=%s does not exist.",
                        newAccount.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
                if (!newAccount.RegIDIsMature())
                    return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, the new owner account is not registerd or not matured! new uid=%s",
                        newAccount.ToString()), REJECT_INVALID, "account-not-mature");

                asset.owner_uid = newAccount.regid;
            } else
                LogPrint("INFO", "CAssetUpdateTx::ExecuteTx, the new owner uid=%s equal the old one=%s",
                    newOwnerUid.ToString(), txUid.ToString());
            break;
        }
        case CAssetUpdateData::NAME: {
            asset.name = update_data.get<string>();
            break;
        }
        case CAssetUpdateData::MINT_AMOUNT: {
            uint64_t mintAmount = update_data.get<uint64_t>();
            uint64_t newTotalSupply = asset.total_supply + mintAmount;
            if (newTotalSupply > MAX_ASSET_TOTAL_SUPPLY || newTotalSupply < asset.total_supply) {
                return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, the new mintAmount=%llu + total_supply=%s greater than %llu,",
                            mintAmount, asset.total_supply, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-mint-amount");
            }

            if (!account.OperateBalance(asset_symbol, BalanceOpType::ADD_FREE, mintAmount)) {
                return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, add mintAmount to asset owner account failed, txUid=%s, mintAmount=%llu",
                                txUid.ToString(), mintAmount), UPDATE_ACCOUNT_FAIL, "account-add-free-failed");
            }
            asset.total_supply = newTotalSupply;
            break;
        }
        default: assert(false);
    }

    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, insufficient funds in account, txUid=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    uint64_t assetUpdateFee; //550 WICC
    if (!cw.sysParamCache.GetParam(ASSET_UPDATE_FEE, assetUpdateFee)) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, read param ASSET_UPDATE_FEE error"),
                         REJECT_INVALID, "read-sysparam-error");
    }
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, assetUpdateFee)) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, insufficient funds in account for sub issued fee=%d, txUid=%s",
                        assetUpdateFee, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }
    vector<CRegID> delegateList;
    if (!cw.delegateCache.GetTopDelegateList(delegateList)) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, get top delegate list failed",
            txUid.ToString()), REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegateList.size() != 0 && delegateList.size() == IniCfg().GetTotalDelegateNum());

    for (size_t i =0; i < delegateList.size(); i++) {
        const CRegID &delegateRegid = delegateList[i];
        CAccount delegateAccount;
        if (!cw.accountCache.GetAccount(CUserID(delegateRegid), delegateAccount)) {
            return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, get delegate account info failed! delegate regid=%s",
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        uint64_t minerFee = assetUpdateFee / delegateList.size();
        if (i == 0) minerFee += assetUpdateFee % delegateList.size(); // give the dust amount to first delegate account

        if (!delegateAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, minerFee)) {
            return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, add asset fee to miner failed, miner regid=%s",
                            delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(delegateRegid, delegateAccount))
            return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, write delegate account info error, delegate regid=%s",
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!cw.assetCache.SaveAsset(asset))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, save asset failed",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "save-asset-failed");

    if (!cw.accountCache.SetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, write txUid %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}