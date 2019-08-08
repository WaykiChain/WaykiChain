// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assettx.h"

#include "config/const.h"
#include "main.h"
#include "entities/receipt.h"
#include "persistence/assetdb.h"


bool CAssetIssueTx::CheckTx(int height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE(fee_symbol);

    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (asset.symbol.empty() || asset.symbol.size() > MAX_TOKEN_SYMBOL_LEN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_symbol is empty or len=%d greater than %d",
            asset.symbol.size(), MAX_TOKEN_SYMBOL_LEN), REJECT_INVALID, "invalid-asset-symobl");
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

bool CAssetIssueTx::ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, read source txUid %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, insufficient funds in account, txUid=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    uint64_t assetIssueFee; //550 WICC
    if (!cw.sysParamCache.GetParam(ASSET_ISSUE_FEE, assetIssueFee)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, read param ASSET_ISSUE_FEE error"),
                         REJECT_INVALID, "read-sysparam-error");
    }
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, assetIssueFee)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, insufficient funds in account for sub issued fee=%d, txUid=%s",
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
                delegateRegid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");      
    }

    if (!cw.accountCache.SetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write txUid %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!cw.assetCache.SaveAsset(asset))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, save asset failed",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "save-asset-failed");


    vector<CUserID> relatedUids = {txUid};
    if (!account.IsMyUid(asset.owner_uid)) relatedUids.push_back(asset.owner_uid);

    if (!SaveTxAddresses(height, index, cw, state, relatedUids)) return false;

    return true;
}

bool CAssetIssueTx::GetInvolvedKeyIds(CCacheWrapper & cw, set<CKeyID> &keyIds) {
    return AddInvolvedKeyIds({txUid, asset.owner_uid}, cw, keyIds);
}

string CAssetIssueTx::ToString(CAccountDBCache &view) {
    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, nValidHeight=%d\n"
        "owner_uid=%s, asset_symbol=%s, asset_name=%s, total_supply=%llu, mintable=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), llFees, nValidHeight,
        asset.owner_uid.ToString(), asset.symbol, asset.name, asset.total_supply, asset.mintable);
}

Object CAssetIssueTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)

    result.push_back(Pair("owner_uid",    asset.owner_uid.ToString()));
    result.push_back(Pair("asset_symbol",   asset.symbol));
    result.push_back(Pair("asset_name",     asset.name));
    result.push_back(Pair("total_supply",   asset.total_supply));
    result.push_back(Pair("mintable",       asset.mintable));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CAssetUpdateTx

string CAssetUpdateTx::ToString(CAccountDBCache &view) {
    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n"
        "owner_uid=%s, asset_name=%s, mint_amount=%llu",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), llFees, nValidHeight,
        owner_uid.ToString(), asset_name, mint_amount);
}

Object CAssetUpdateTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)

    result.push_back(Pair("owner_uid",   owner_uid.ToString()));
    result.push_back(Pair("asset_name",     asset_name));
    result.push_back(Pair("mint_amount",    mint_amount));

    return result;
}

bool CAssetUpdateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return AddInvolvedKeyIds({txUid, owner_uid}, cw, keyIds);
}

bool CAssetUpdateTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {

    IMPLEMENT_CHECK_TX_FEE(fee_symbol);

    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    
    if (asset_symbol.empty() || asset_symbol.size() > MAX_TOKEN_SYMBOL_LEN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_symbol is empty or len=%d greater than %d",
            asset_symbol.size(), MAX_TOKEN_SYMBOL_LEN), REJECT_INVALID, "invalid-asset-symobl");
    }

    if (asset_name.empty() || asset_name.size() > MAX_ASSET_NAME_LEN) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, asset_name is empty or len=%d greater than %d",
            asset_name.size(), MAX_ASSET_NAME_LEN), REJECT_INVALID, "invalid-asset-name");
    }

    if (mint_amount > MAX_ASSET_TOTAL_SUPPLY) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, asset total_supply=%llu greater than %llu",
            mint_amount, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-asset-name");
    }

    if (owner_uid.type() == typeid(CRegID) && !cw.accountCache.RegIDIsMature(owner_uid.get<CRegID>())) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::CheckTx, owner regid=%s not mature yet",
            owner_uid.get<CRegID>().ToString()), REJECT_INVALID, "asset-owner-regid-not-mature");
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

    uint64_t newTotalSupply = asset.total_supply + mint_amount;
    assert(newTotalSupply >= asset.total_supply);
    if (newTotalSupply > MAX_ASSET_TOTAL_SUPPLY) {
        return state.DoS(100, ERRORMSG("CAssetUpdateTx::ExecuteTx, new_total_supply=%llu greater than %llu,"
                     " old_total_supply=%llu, mint_amount=%llu", newTotalSupply, MAX_ASSET_TOTAL_SUPPLY, 
                     asset.total_supply, mint_amount), REJECT_INVALID, "invalid-mint-amount");
    }

    asset.owner_uid = owner_uid;
    asset.name = asset_name;
    asset.total_supply += mint_amount;

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

    vector<CUserID> relatedUids = {txUid};
    if (!account.IsMyUid(owner_uid)) relatedUids.push_back(owner_uid);

    if (!SaveTxAddresses(height, index, cw, state, relatedUids)) return false;

    return true;
}