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
    IMPLEMENT_CHECK_TX_FEE(SYMB::WICC);

    if (asset.owner_regid.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, owner_regid is empty"),
                         REJECT_INVALID, "invalid-owner-regid");
    }

    if (asset.symbol.empty() || asset.symbol.size() > MAX_TOKEN_SYMBOL_LEN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_symbol is empty or len=%d greater than %d",
            asset.symbol.size(), MAX_TOKEN_SYMBOL_LEN), REJECT_INVALID, "invalid-asset-symobl");
    }

    if (asset.name.empty() || asset.name.size() > MAX_ASSET_NAME_LEN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_name is empty or len=%d greater than %d",
            asset.name.size(), MAX_ASSET_NAME_LEN), REJECT_INVALID, "invalid-asset-name");
    }

    if (!cw.accountCache.RegIDIsMature(asset.owner_regid)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, owner_regid=%s not mature yet",
            asset.owner_regid.ToRawString()), REJECT_INVALID, "asset-owner-regid-not-mature");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());

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
    if (!cw.delegateCache.GetTopDelegates(delegateList)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, get top delegates failed",
            txUid.ToString()), REJECT_INVALID, "get-delegates-failed");
    }
    assert(delegateList.size() != 0 && delegateList.size() == IniCfg().GetTotalDelegateNum());

    for (int i =0; i < delegateList.size(); i++) {
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


    if (!SaveTxAddresses(height, index, cw, state, {txUid})) return false;

    return true;
}

bool CAssetIssueTx::GetInvolvedKeyIds(CCacheWrapper & cw, set<CKeyID> &keyIds) {
    return AddInvolvedKeyIds({txUid}, cw, keyIds);
}

string CAssetIssueTx::ToString(CAccountDBCache &view) {
    return strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n"
                     "owner_regid=%s, asset_symbol=%s, asset_name=%s, total_supply=%d, mintable=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.get<CPubKey>().ToString(), llFees,
                     txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight,
                     asset.owner_regid, asset.symbol, asset.name, asset.total_supply, asset.mintable);
}

Object CAssetIssueTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)

    result.push_back(Pair("owner_regid",    asset.owner_regid.ToString));
    result.push_back(Pair("asset_symbol",   asset.symbol));
    result.push_back(Pair("asset_name",     asset.name));
    result.push_back(Pair("total_supply",   asset.total_supply));
    result.push_back(Pair("mintable",       asset.mintable));

    return result;
}
