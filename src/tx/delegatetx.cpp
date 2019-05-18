// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "delegatetx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

string CDelegateVoteTx::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(txUid, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", GetTxType(nTxType),
        GetHash().ToString().c_str(), nVersion, keyId.ToAddress(), keyId.ToString());
    str += "vote:\n";
    for (auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        str += strprintf("%s", item->ToString());
    }
    return str;
}

Object CDelegateVoteTx::ToJson(const CAccountViewCache &accountView) const {
    Object result;
    CAccountViewCache view(accountView);
    CKeyID keyId;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid", txUid.ToString()));
    view.GetKeyId(txUid, keyId);
    result.push_back(Pair("addr", keyId.ToAddress()));
    result.push_back(Pair("fees", llFees));
    Array operVoteFundArray;
    for (auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        operVoteFundArray.push_back(item->ToJson());
    }
    result.push_back(Pair("operVoteFundList", operVoteFundArray));
    return result;
}

// FIXME: not useuful
bool CDelegateVoteTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    // CKeyID keyId;
    // if (!view.GetKeyId(userId, keyId))
    //     return false;

    // vAddr.insert(keyId);
    // for (auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
    //     vAddr.insert(iter->fund.GetVoteId());
    // }
    return true;
}

bool CDelegateVoteTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                                CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                                CScriptDBViewCache &scriptDB) {
    CAccount acctInfo;
    if (!view.GetAccount(txUid, acctInfo)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read regist addr %s account info error", txUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog acctInfoLog(acctInfo); //save account state before modification
    uint64_t minusValue = llFees;
    if (minusValue > 0) {
        if(!acctInfo.OperateAccount(MINUS_FREE, minusValue, nHeight))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate account failed ,regId=%s", txUid.ToString()),
                UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }
    if (!acctInfo.ProcessDelegateVote(operVoteFunds, nHeight)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate vote failed ,regId=%s", txUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
    }
    if (!view.SaveAccountInfo(acctInfo)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error", acctInfo.regID.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    txundo.vAccountLog.push_back(acctInfoLog); //keep the old state after the above operation completed properly.
    txundo.txHash = GetHash();

    for (auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
        CAccount delegate;
        const CUserID &delegateUId = iter->fund.GetVoteId();
        if (!view.GetAccount(delegateUId, delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read KeyId(%s) account info error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccountLog delegateAcctLog(delegate);
        if (!delegate.OperateVote(VoteOperType(iter->operType), iter->fund.GetVoteCount())) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate address %s vote fund error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }
        txundo.vAccountLog.push_back(delegateAcctLog); // keep delegate state before modification

        // set the new value and erase the old value
        CScriptDBOperLog operDbLog;
        if (!scriptDB.SetDelegateData(delegate, operDbLog)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, save account id %s vote info error",
                            delegate.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
        txundo.vScriptOperLog.push_back(operDbLog);

        CScriptDBOperLog eraseDbLog;
        if (delegateAcctLog.inVoteBcoins > 0) {
            if(!scriptDB.EraseDelegateData(delegateAcctLog, eraseDbLog)) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, erase account id %s vote info error",
                                delegateAcctLog.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }
        }

        if (!view.SaveAccountInfo(delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error",
                            acctInfo.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }

        txundo.vScriptOperLog.push_back(eraseDbLog);
    }

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if (!view.GetKeyId(txUid, sendKeyId)) {
            return ERRORMSG("CDelegateVoteTx::ExecuteTx, get regAcctId by account error!");
        }

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    return true;
}

bool CDelegateVoteTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                                CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                                CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100,
                             ERRORMSG("CDelegateVoteTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!view.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    if (SysCfg().GetAddressToTxFlag() && txundo.vScriptOperLog.size() > 0) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, undo scriptdb data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        ++rIterScriptDBLog;
    }

    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        // Recover the old value and erase the new value.
        if (!scriptDB.SetDelegateData(rIterScriptDBLog->vKey))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, set delegate data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

        ++rIterScriptDBLog;
        if (!scriptDB.EraseDelegateData(rIterScriptDBLog->vKey))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, erase delegate data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    return true;
}

bool CDelegateVoteTx::CheckTx(CValidationState &state, CAccountViewCache &view,
                          CScriptDBViewCache &scriptDB) {
    if (txUid.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, send account is not CRegID type"),
            REJECT_INVALID, "deletegate-tx-error");
    }
    if (0 == operVoteFunds.size()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, the deletegate oper fund empty"),
            REJECT_INVALID, "oper-fund-empty-error");
    }
    if (operVoteFunds.size() > IniCfg().GetTotalDelegateNum()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, the deletegates number a transaction can't exceeds maximum"),
            REJECT_INVALID, "deletegates-number-error");
    }
    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, delegate tx fee out of range"),
            REJECT_INVALID, "bad-tx-fee-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    CKeyID sendTxKeyID;
    if(!view.GetKeyId(txUid, sendTxKeyID)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get keyId error by CUserID =%s",
                        txUid.ToString()), REJECT_INVALID, "");
    }

    CAccount sendAcct;
    if (!view.GetAccount(txUid, sendAcct)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, userid=%s",
                        txUid.ToString()), REJECT_INVALID, "bad-read-accountdb");
    }
    if (!sendAcct.IsRegistered()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, pubkey not registered"),
                        REJECT_INVALID, "bad-no-pubkey");
    }

    if ( GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2 ) {
        if (!CheckSignatureSize(signature)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, signature size invalid"),
                REJECT_INVALID, "bad-tx-sig-size");
        }

        uint256 signhash = ComputeSignatureHash();
        if (!CheckSignScript(signhash, signature, sendAcct.pubKey)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, CheckSignScript failed"),
                REJECT_INVALID, "bad-signscript-check");
        }
    }

    // check delegate duplication
    set<string> setOperVoteKeyID;
    for (auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        if (0 >= item->fund.GetVoteCount() || (uint64_t)GetMaxMoney() < item->fund.GetVoteCount())
            return ERRORMSG("CDelegateVoteTx::CheckTx, votes: %lld not within (0 .. MaxVote)",
                            item->fund.GetVoteCount());

        setOperVoteKeyID.insert(item->fund.ToString());
        CAccount acctInfo;
        if (!view.GetAccount(item->fund.GetVoteId(), acctInfo))
            return state.DoS(100,
                             ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, address=%s",
                                      item->fund.ToString()),
                             REJECT_INVALID, "bad-read-accountdb");

        if (GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2) {
            if (!acctInfo.IsRegistered()) {
                return state.DoS(
                    100,
                    ERRORMSG("CDelegateVoteTx::CheckTx, account is unregistered, address=%s",
                             item->fund.ToString()),
                    REJECT_INVALID, "bad-read-accountdb");
            }
        }
    }

    if (setOperVoteKeyID.size() != operVoteFunds.size()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, duplication vote fund"),
                         REJECT_INVALID, "deletegates-duplication fund-error");
    }

    return true;
}