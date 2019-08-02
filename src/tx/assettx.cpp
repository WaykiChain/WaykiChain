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

    uint64_t assetIssueFeeMin; //550 WICC
    if (!cw.sysParamCache.GetParam(ASSET_ISSUE_FEE_MIN, assetIssueFeeMin)) {
         return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, read param ASSET_ISSUE_FEE_MIN error",
            asset_symbol.size()), REJECT_INVALID, "read-sysparam-error");
    }

    if (llFees < assetIssueFeeMin * COIN) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, llFees=%d < required=%d",
            llFees, assetIssueFeeMin * COIN), REJECT_INVALID, "asset-fee-insufficient");
    }

    if (asset_symbol.size() > 12) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_symbol size=%d greater than 12",
            asset_symbol.size()), REJECT_INVALID, "asset-symobl-is-too-long");
    }
    if (asset_name.size() > 32) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, asset_name size=%d greater than 32",
            asset_name.size()), REJECT_INVALID, "asset-name-is-too-long");
    }

    if (!cw.accountCache.RegIDIsMature(owner_regid)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::CheckTx, owner_regid=%s not mature yet",
            owner_regid.ToRawString()), REJECT_INVALID, "asset-owner-regid-not-mature");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());

    return true;
}

bool CAssetIssueTx::ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, read source keyId %s account info error",
            keyId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    //TODO: get 11 delegate accounts and divide evenly the llFees to them
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees/11)) {
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, insufficient funds in account, keyid=%s",
                        keyId.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write source addr %s account info error",
            regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    //TODO: save asset into assetCache

    if (!SaveTxAddresses(height, index, cw, state, {txUid}))
        return false;

    return true;
}

bool CAssetIssueTx::GetInvolvedKeyIds(CCacheWrapper & cw, set<CKeyID> &keyIds) {
    if (!txUid.get<CPubKey>().IsFullyValid())
        return false;

    keyIds.insert(txUid.get<CPubKey>().GetKeyId());
    return true;
}

string CAssetIssueTx::ToString(CAccountDBCache &view) {
    return strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n"
                     "owner_regid=%s, asset_symbol=%s, asset_name=%s, total_supply=%d, mintable=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.get<CPubKey>().ToString(), llFees,
                     txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight,
                     owner_regid, asset_symbol, asset_name, total_supply, mintable);
}

Object CAssetIssueTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)

    result.push_back(Pair("owner_regid",    owner_regid));
    result.push_back(Pair("asset_symbol",   asset_symbol));
    result.push_back(Pair("asset_name",     asset_name));
    result.push_back(Pair("total_supply",   total_supply));
    result.push_back(Pair("mintable",       mintable));

    return result;
}
