// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockpricemediantx.h"
#include "main.h"


bool CBlockPriceMedianTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    return true;
}

bool CBlockPriceMedianTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount acctInfo;
    if (!cw.accountCache.GetAccount(txUid, acctInfo)) {
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctInfoLog(acctInfo);
    // TODO: want to check something

    CUserID userId = acctInfo.keyId;
    if (!cw.accountCache.SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.txUndo.accountLogs.push_back(acctInfoLog);
    cw.txUndo.txHash = GetHash();

   if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CBlockPriceMedianTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &view) {
    //TODO
    return "";
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &AccountView) const {
    //TODO
    return Object();
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

uint64_t CBlockPriceMedianTx::GetMedianPriceByType(const CoinType coinType, const PriceType priceType) {
    return mapMedianPricePoints.count(CCoinPriceType(coinType, priceType))
               ? mapMedianPricePoints[CCoinPriceType(coinType, priceType)]
               : 0;
}