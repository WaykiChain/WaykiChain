// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "bcointx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "persistence/contractdb.h"
#include "commons/util.h"
#include "main.h"
#include "miner/miner.h"
#include "config/version.h"

bool CBaseCoinTransferTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid.type());

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CBaseCoinTransferTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    CAccount desAcct;
    bool generateRegID = false;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        if (txUid.type() == typeid(CPubKey)) {
            srcAcct.owner_pubkey = txUid.get<CPubKey>();

            CRegID regId;
            // If the source account does NOT have CRegID, need to generate a new CRegID.
            if (!cw.accountCache.GetRegId(txUid, regId)) {
                srcAcct.regid = CRegID(nHeight, nIndex);
                generateRegID = true;
            }
        }
    }

    uint64_t minusValue = llFees + bcoins;
    if (!srcAcct.OperateBalance("WICC", BalanceOpType::ADD_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (generateRegID) {
        if (!cw.accountCache.SaveAccount(srcAcct))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    if (!cw.accountCache.GetAccount(toUid, desAcct)) {
        if (toUid.type() == typeid(CKeyID)) {  // Target account does NOT have CRegID
            desAcct.keyid    = toUid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAcct.OperateBalance("WICC", BalanceOpType::ADD_FREE, bcoins)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(toUid, desAcct))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account error, kyeId=%s",
                         desAcct.keyid.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-save-account");

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid, toUid}))
        return false;

    return true;
}

string CBaseCoinTransferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, bcoins=%ld, llFees=%ld, memo=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), bcoins, llFees,
        HexStr(memo), nValidHeight);
}

Object CBaseCoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID  desKeyId;
    accountCache.GetKeyId(toUid, desKeyId);

    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)

    result.push_back(Pair("to_uid",         toUid.ToString()));
    result.push_back(Pair("to_addr",        desKeyId.ToAddress()));
    result.push_back(Pair("money",          bcoins));
    result.push_back(Pair("memo",           HexStr(memo)));

    return result;
}

bool CBaseCoinTransferTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);

    CKeyID desKeyId;
    if (!cw.accountCache.GetKeyId(toUid, desKeyId))
        return false;

    keyIds.insert(desKeyId);

    return true;
}
