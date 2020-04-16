// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assettx.h"

#include "config/const.h"
#include "main.h"
#include "entities/receipt.h"
#include "persistence/assetdb.h"

static const string ASSET_ACTION_ISSUE = "issue";
static const string ASSET_ACTION_UPDATE = "update";

Object AssetToJson(const CAccountDBCache &accountCache, const CUserIssuedAsset &asset) {
    Object result;
    CKeyID ownerKeyid;
    accountCache.GetKeyId(asset.owner_uid, ownerKeyid);
    result.push_back(Pair("asset_symbol",   asset.asset_symbol));
    result.push_back(Pair("asset_name",     asset.asset_name));
    result.push_back(Pair("owner_uid",      asset.owner_uid.ToString()));
    result.push_back(Pair("owner_addr",     ownerKeyid.ToAddress()));
    result.push_back(Pair("total_supply",   asset.total_supply));
    result.push_back(Pair("mintable",       asset.mintable));
    return result;
}

Object AssetToJson(const CAccountDBCache &accountCache, const CAsset &asset){
    return asset.ToJsonObj();
}
static bool ProcessAssetFee(CCacheWrapper &cw, CValidationState &state, const string &action,
    CAccount &txAccount, uint32_t currHeight, ReceiptList &receipts) {

    uint64_t assetFee = 0;
    if (action == ASSET_ACTION_ISSUE) {
        if (!cw.sysParamCache.GetParam(ASSET_ISSUE_FEE, assetFee))
            return state.DoS(100, ERRORMSG("ProcessAssetFee, read param ASSET_ACTION_ISSUE error"),
                            REJECT_INVALID, "read-sysparam-error");
    } else {
        assert(action == ASSET_ACTION_UPDATE);
        if (!cw.sysParamCache.GetParam(ASSET_UPDATE_FEE, assetFee))
            return state.DoS(100, ERRORMSG("ProcessAssetFee, read param ASSET_UPDATE_FEE error"),
                            REJECT_INVALID, "read-sysparam-error");
    }

    if (!txAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, assetFee, ReceiptType::TRANSFER_ACTUAL_COINS, receipts))
        return state.DoS(100, ERRORMSG("ProcessAssetFee, insufficient funds in account for %s asset fee=%llu, tx_regid=%s",
                        action, assetFee, txAccount.regid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");

    uint64_t assetRiskFeeRatio;
    if(!cw.sysParamCache.GetParam(SysParamType::ASSET_RISK_FEE_RATIO, assetRiskFeeRatio)) {
        return state.DoS(100, ERRORMSG("ProcessAssetFee, get assetRiskFeeRatio error",
                                       action, assetFee, txAccount.regid.ToString()), READ_SYS_PARAM_FAIL, "read-db-error");
    }

    uint64_t riskFee       = assetFee * assetRiskFeeRatio / RATIO_BOOST;
    uint64_t minerTotalFee = assetFee - riskFee;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("ProcessAssetFee, get risk reserve account failed"),
                        READ_ACCOUNT_FAIL, "get-account-failed");

    ReceiptType code = (action == ASSET_ACTION_ISSUE) ? ReceiptType::ASSET_ISSUED_FEE_TO_RESERVE :
                                                        ReceiptType::ASSET_UPDATED_FEE_TO_RESERVE;

    if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, riskFee, code, receipts)) {
        return state.DoS(100, ERRORMSG("ProcessAssetFee, operate balance failed! add %s asset fee=%llu to risk reserve account error",
            action, riskFee), UPDATE_ACCOUNT_FAIL, "update-account-failed");
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("ProcessAssetFee, write fcoin genesis account info error, regid=%s",
            fcoinGenesisAccount.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    VoteDelegateVector delegates;
    if (!cw.delegateCache.GetActiveDelegates(delegates)) {
        return state.DoS(100, ERRORMSG("ProcessAssetFee, GetActiveDelegates failed"),
            REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegates.size() != 0 );

    for (size_t i = 0; i < delegates.size(); i++) {
        const CRegID &delegateRegid = delegates[i].regid;
        CAccount delegateAccount;
        if (!cw.accountCache.GetAccount(CUserID(delegateRegid), delegateAccount)) {
            return state.DoS(100, ERRORMSG("ProcessAssetFee, get delegate account info failed! delegate regid=%s",
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        uint64_t minerUpdatedFee = minerTotalFee / delegates.size();
        if (i == 0) minerUpdatedFee += minerTotalFee % delegates.size(); // give the dust amount to topmost miner

        ReceiptType code = (action == ASSET_ACTION_ISSUE) ? ReceiptType::ASSET_ISSUED_FEE_TO_MINER :
                                                            ReceiptType::ASSET_UPDATED_FEE_TO_MINER;
        if (!delegateAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, minerUpdatedFee, code, receipts)) {
            return state.DoS(100, ERRORMSG("ProcessAssetFee, add %s asset fee to miner failed, miner regid=%s",
                action, delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(delegateRegid, delegateAccount))
            return state.DoS(100, ERRORMSG("ProcessAssetFee, write delegate account info error, delegate regid=%s",
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CUserIssueAssetTx

bool CUserIssueAssetTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    string errMsg = "";
    if (!CAsset::CheckSymbol(AssetType::UIA, asset.asset_symbol, errMsg))
        return state.DoS(100, ERRORMSG("invalid asset symbol! %s", errMsg),
                        REJECT_INVALID, "invalid-asset-symbol");

    if (asset.asset_name.empty() || asset.asset_name.size() > MAX_ASSET_NAME_LEN)
        return state.DoS(100, ERRORMSG("asset_name is empty or len=%d greater than %d",
                        asset.asset_name.size(), MAX_ASSET_NAME_LEN), REJECT_INVALID, "invalid-asset-name");

    if (asset.total_supply == 0 || asset.total_supply > MAX_ASSET_TOTAL_SUPPLY)
        return state.DoS(100, ERRORMSG("asset total_supply=%llu can not == 0 or > %llu",
            asset.total_supply, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-total-supply");

    if (!asset.owner_uid.is<CRegID>())
        return state.DoS(100, ERRORMSG("asset owner_uid must be regid"), REJECT_INVALID, "owner-uid-type-error");

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("public key is invalid"), REJECT_INVALID, "bad-publickey");

    if (!txAccount.IsRegistered() || !txUid.get<CRegID>().IsMature(context.height))
        return state.DoS(100, ERRORMSG(" account unregistered or immature"),
                         REJECT_INVALID, "account-unregistered-or-immature");

    return true;
}

bool CUserIssueAssetTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (cw.assetCache.HasAsset(asset.asset_symbol))
        return state.DoS(100, ERRORMSG("CUserIssueAssetTx::ExecuteTx, the asset has been issued! symbol=%s",
            asset.asset_symbol), REJECT_INVALID, "asset-existed-error");

    shared_ptr<CAccount> spAssetAccount = nullptr;
    CAccount *pOwnerAccount = nullptr;
    {

        if (txAccount.IsSelfUid(asset.owner_uid)) {
            pOwnerAccount = &txAccount;
        } else {
            spAssetAccount = make_shared<CAccount>();

            if (!cw.accountCache.GetAccount(asset.owner_uid, *spAssetAccount))
                return state.DoS(100, ERRORMSG(" read account failed! asset owner "
                    "account not exist, owner_uid=%s", asset.owner_uid.ToDebugString()), REJECT_INVALID, "bad-getaccount");
            pOwnerAccount = spAssetAccount.get();
        }

        if (pOwnerAccount->regid.IsEmpty() || !pOwnerAccount->regid.IsMature(context.height)) {
            return state.DoS(100, ERRORMSG(" owner regid=%s account is unregistered or immature",
                asset.owner_uid.get<CRegID>().ToString()), REJECT_INVALID, "owner-account-unregistered-or-immature");
        }

        if (!ProcessAssetFee(cw, state, ASSET_ACTION_ISSUE, txAccount, context.height, receipts))
            return false;

        if (!pOwnerAccount->OperateBalance(asset.asset_symbol, BalanceOpType::ADD_FREE, asset.total_supply,
                                            ReceiptType::ASSET_MINT_NEW_AMOUNT, receipts)) {
            return state.DoS(100, ERRORMSG("CUserIssueAssetTx::ExecuteTx, fail to add total_supply to issued account! total_supply=%llu, txUid=%s",
                            asset.total_supply, txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
        }
    }

    if ( spAssetAccount && !cw.accountCache.SetAccount(spAssetAccount->keyid, *spAssetAccount) )
        return state.DoS(100, ERRORMSG("CUserIssueAssetTx::ExecuteTx, set asset owner account to db failed! owner_uid=%s",
                        asset.owner_uid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-set-accountdb");

    //Persist with Owner's RegID to save space than other User ID types
    CAsset savedAsset(asset.asset_symbol, asset.asset_name, AssetType::UIA, AssetPermType::PERM_DEX_BASE,
                    CUserID(pOwnerAccount->regid), asset.total_supply, asset.mintable);

    if (!cw.assetCache.SetAsset(savedAsset))
        return state.DoS(100, ERRORMSG("CUserIssueAssetTx::ExecuteTx, save asset failed! txUid=%s",
            txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "save-asset-failed");

    return true;
}

string CUserIssueAssetTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, valid_height=%d, "
        "owner_uid=%s, asset_symbol=%s, asset_name=%s, total_supply=%llu, mintable=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToDebugString(), llFees, valid_height,
        asset.owner_uid.ToDebugString(), asset.asset_symbol, asset.asset_name, asset.total_supply, asset.mintable);
}

Object CUserIssueAssetTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    container::Append(result, AssetToJson(accountCache, asset));
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CUserUpdateAsset

static const EnumTypeMap<CUserUpdateAsset::UpdateType, string> ASSET_UPDATE_TYPE_NAMES = {
    {CUserUpdateAsset::OWNER_UID,   "owner_uid"},
    {CUserUpdateAsset::NAME,        "name"},
    {CUserUpdateAsset::MINT_AMOUNT, "mint_amount"}
};

static const unordered_map<string, CUserUpdateAsset::UpdateType> ASSET_UPDATE_PARSE_MAP = {
    {"owner_addr",  CUserUpdateAsset::OWNER_UID},
    {"name",        CUserUpdateAsset::NAME},
    {"mint_amount", CUserUpdateAsset::MINT_AMOUNT}
};

shared_ptr<CUserUpdateAsset::UpdateType> CUserUpdateAsset::ParseUpdateType(const string& str) {
    if (!str.empty()) {
        auto it = ASSET_UPDATE_PARSE_MAP.find(str);
        if (it != ASSET_UPDATE_PARSE_MAP.end()) {
            return make_shared<UpdateType>(it->second);
        }
    }
    return nullptr;
}

const string& CUserUpdateAsset::GetUpdateTypeName(UpdateType type) {
    auto it = ASSET_UPDATE_TYPE_NAMES.find(type);
    if (it != ASSET_UPDATE_TYPE_NAMES.end()) return it->second;
    return EMPTY_STRING;
}
void CUserUpdateAsset::Set(const CUserID &ownerUid) {
    type = OWNER_UID;
    value = ownerUid;
}

void CUserUpdateAsset::Set(const string &name) {
    type = NAME;
    value = name;

}
void CUserUpdateAsset::Set(const uint64_t &mintAmount) {
    type = MINT_AMOUNT;
    value = mintAmount;

}

string CUserUpdateAsset::ValueToString() const {
    string s;
    switch (type) {
        case OWNER_UID:     s += get<CUserID>().ToString(); break;
        case NAME:          s += get<string>(); break;
        case MINT_AMOUNT:   s += std::to_string(get<uint64_t>()); break;
        default: break;
    }
    return s;
}

string CUserUpdateAsset::ToString(const CAccountDBCache &accountCache) const {
    string s = "update_type=" + GetUpdateTypeName(type);
    s += ", update_value=" + ValueToString();
    return s;
}

Object CUserUpdateAsset::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    result.push_back(Pair("update_type",   GetUpdateTypeName(type)));
    result.push_back(Pair("update_value",  ValueToString()));
    if (type == OWNER_UID) {
        CKeyID ownerKeyid;
        accountCache.GetKeyId(get<CUserID>(), ownerKeyid);
        result.push_back(Pair("owner_addr",   ownerKeyid.ToAddress()));
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CUserUpdateAssetTx

string CUserUpdateAssetTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%ld, valid_height=%d, asset_symbol=%s, "
        "update_data=%s",
        GetTxType(nTxType), GetHash().ToString(), nVersion, fee_symbol, txUid.ToDebugString(), llFees, valid_height,
        asset_symbol, update_data.ToString(accountCache));
}

Object CUserUpdateAssetTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("asset_symbol",   asset_symbol));
    container::Append(result, update_data.ToJson(accountCache));

    return result;
}

bool CUserUpdateAssetTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    string errMsg = "";
    if (!CAsset::CheckSymbol(AssetType::UIA, asset_symbol, errMsg))
        return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::CheckTx, asset_symbol error: %s", errMsg),
                        REJECT_INVALID, "invalid-asset-symbol");

    switch (update_data.GetType()) {
        case CUserUpdateAsset::OWNER_UID: {
            const CUserID &newOwnerUid = update_data.get<CUserID>();
            if (!newOwnerUid.is<CRegID>()) {
                return state.DoS(100, ERRORMSG("%s, the new asset owner_uid must be regid", __FUNCTION__), REJECT_INVALID,
                    "owner-uid-type-error");
            }
            break;
        }
        case CUserUpdateAsset::NAME: {
            const string &name = update_data.get<string>();
            if (name.empty() || name.size() > MAX_ASSET_NAME_LEN)
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::CheckTx, asset name is empty or len=%d greater than %d",
                    name.size(), MAX_ASSET_NAME_LEN), REJECT_INVALID, "invalid-asset-name");
            break;
        }
        case CUserUpdateAsset::MINT_AMOUNT: {
            uint64_t mintAmount = update_data.get<uint64_t>();
            if (mintAmount == 0 || mintAmount > MAX_ASSET_TOTAL_SUPPLY) {
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::CheckTx, asset mint_amount=%llu is 0 or greater than %llu",
                    mintAmount, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-mint-amount");
            }
            break;
        }
        default: {
            return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::CheckTx, unsupported updated_type=%d",
                update_data.GetType()), REJECT_INVALID, "invalid-update-type");
        }
    }

    if (!txAccount.IsRegistered() || !txUid.get<CRegID>().IsMature(context.height))
        return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::CheckTx, account unregistered or immature"),
                         REJECT_INVALID, "account-unregistered-or-immature");

    return true;
}


bool CUserUpdateAssetTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    CAsset asset;
    if (!cw.assetCache.GetAsset(asset_symbol, asset))
        return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, get asset by symbol=%s failed",
            asset_symbol), REJECT_INVALID, "get-asset-failed");

    if (!txAccount.IsSelfUid(asset.owner_uid))
        return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, no privilege to update asset, uid dismatch,"
            " txUid=%s, old_asset_uid=%s",
            txUid.ToDebugString(), asset.owner_uid.ToString()), REJECT_INVALID, "asset-uid-dismatch");


    switch (update_data.GetType()) {
        case CUserUpdateAsset::OWNER_UID: {
            const CUserID &newOwnerUid = update_data.get<CUserID>();
            if (txAccount.IsSelfUid(newOwnerUid))
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, the new owner uid=%s is belong to old owner account",
                    newOwnerUid.ToDebugString()), REJECT_INVALID, "invalid-new-asset-owner-uid");

            CAccount newAccount;
            if (!cw.accountCache.GetAccount(newOwnerUid, newAccount))
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, the new owner uid=%s does not exist.",
                    newOwnerUid.ToDebugString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
            if (!newAccount.IsRegistered())
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, the new owner account is not registered! new uid=%s",
                    newOwnerUid.ToDebugString()), REJECT_INVALID, "account-not-registered");
            if (!newAccount.regid.IsMature(context.height))
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, the new owner regid is not matured! new uid=%s",
                    newOwnerUid.ToDebugString()), REJECT_INVALID, "account-not-matured");

            asset.owner_uid = newAccount.regid;
            break;
        }
        case CUserUpdateAsset::NAME: {
            asset.asset_name = update_data.get<string>();
            break;
        }
        case CUserUpdateAsset::MINT_AMOUNT: {

            if (!asset.mintable)
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, the asset is not mintable"),
                                 REJECT_INVALID, "asset-not-mintable");

            uint64_t mintAmount = update_data.get<uint64_t>();
            uint64_t newTotalSupply = asset.total_supply + mintAmount;
            if (newTotalSupply > MAX_ASSET_TOTAL_SUPPLY || newTotalSupply < asset.total_supply) {
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, the new mintAmount=%llu + total_supply=%s greater than %llu,",
                            mintAmount, asset.total_supply, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-mint-amount");
            }

            if (!txAccount.OperateBalance(asset_symbol, BalanceOpType::ADD_FREE, mintAmount,
                                        ReceiptType::ASSET_MINT_NEW_AMOUNT, receipts)) {
                return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, add mintAmount to asset owner account failed, txUid=%s, mintAmount=%llu",
                                txUid.ToDebugString(), mintAmount), UPDATE_ACCOUNT_FAIL, "account-add-free-failed");
            }

            asset.total_supply = newTotalSupply;
            break;
        }
        default: assert(false);
    }

    if (!ProcessAssetFee(cw, state, ASSET_ACTION_UPDATE, txAccount, context.height, receipts))
        return false;

    if (!cw.assetCache.SetAsset(asset))
        return state.DoS(100, ERRORMSG("CUserUpdateAssetTx::ExecuteTx, save asset failed",
            txUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "save-asset-failed");

    return true;
}
