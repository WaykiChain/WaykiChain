// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockrewardtx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "persistence/contractdb.h"
#include "crypto/hash.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

bool CBlockRewardTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    CAccount acctInfo;
    if (!cw.pAccountCache->GetAccount(txUid, acctInfo)) {
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctInfoLog(acctInfo);
    if (0 == nIndex) {
        // nothing to do here
    } else if (-1 == nIndex) {  // maturity reward tx, only update values
        acctInfo.bcoins += rewardValue;
    } else {  // never go into this step
        return ERRORMSG("CBlockRewardTx::ExecuteTx, invalid index");
    }

    CUserID userId = acctInfo.keyID;
    if (!cw.pAccountCache->SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.pTxUndo->Clear();
    cw.pTxUndo->vAccountLog.push_back(acctInfoLog);
    cw.pTxUndo->txHash = GetHash();

    IMPLEMENT_PERSIST_TX_KEYID(txUid, CUserID());

    return true;
}

bool CBlockRewardTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.pTxUndo->vAccountLog.rbegin();
    for (; rIterAccountLog != cw.pTxUndo->vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.pAccountCache->GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.pAccountCache->SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    IMPLEMENT_UNPERSIST_TX_STATE;
    return true;
}

string CBlockRewardTx::ToString(CAccountCache &view) {
    string str;
    CKeyID keyId;
    view.GetKeyId(txUid, keyId);
    CRegID regId;
    view.GetRegId(txUid, regId);
    str += strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyid=%s, rewardValue=%ld\n",
                    GetTxType(nTxType), GetHash().ToString().c_str(), nVersion,
                    regId.ToString(), keyId.GetHex(), rewardValue);

    return str;
}

Object CBlockRewardTx::ToJson(const CAccountCache &AccountView) const{
    Object result;
    CAccountCache view(AccountView);
    CKeyID keyid;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("tx_type", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    if (txUid.type() == typeid(CRegID)) {
        result.push_back(Pair("regid", txUid.get<CRegID>().ToString()));
    }
    if (txUid.type() == typeid(CPubKey)) {
        result.push_back(Pair("pubkey", txUid.get<CPubKey>().ToString()));
    }
    view.GetKeyId(txUid, keyid);
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("money", rewardValue));
    result.push_back(Pair("valid_height", nHeight));
    return result;
}

bool CBlockRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (txUid.type() == typeid(CRegID)) {
        if (!cw.pAccountCache->GetKeyId(txUid, keyId))
            return false;

        keyIds.insert(keyId);
    } else if (txUid.type() == typeid(CPubKey)) {
        CPubKey pubKey = txUid.get<CPubKey>();
        if (!pubKey.IsFullyValid())
            return false;

        keyIds.insert(pubKey.GetKeyId());
    }

    return true;
}