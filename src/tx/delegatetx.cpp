// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "delegatetx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

string CDelegateVoteTx::ToString(CAccountCache &view) {
    string str;
    CKeyID keyId;
    view.GetKeyId(txUid, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", GetTxType(nTxType),
                     GetHash().ToString().c_str(), nVersion, keyId.ToAddress(), keyId.ToString());
    str += "vote:\n";
    for (const auto &vote : candidateVotes) {
        str += strprintf("%s", vote.ToString());
    }
    return str;
}

Object CDelegateVoteTx::ToJson(const CAccountCache &accountView) const {
    Object result;
    CAccountCache view(accountView);
    CKeyID keyId;
    pCdMan->pAccountCache->GetKeyId(txUid, keyId);

    result.push_back(Pair("tx_hash", GetHash().GetHex()));
    result.push_back(Pair("tx_type", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid", txUid.ToString()));
    result.push_back(Pair("addr", keyId.ToAddress()));
    result.push_back(Pair("fees", llFees));
    Array candidateVoteArray;
    for (const auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }
    result.push_back(Pair("candidate_votes", candidateVoteArray));
    return result;
}

// FIXME: not useful
bool CDelegateVoteTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    // CKeyID keyId;
    // if (!cw.accountCache.GetKeyId(userId, keyId))
    //     return false;

    // vAddr.insert(keyId);
    // for (auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
    //     vAddr.insert(iter->fund.GetCandidateUid());
    // }
    return true;
}


bool CDelegateVoteTx::CheckTx(CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (0 == candidateVotes.size()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, the deletegate oper fund empty"),
            REJECT_INVALID, "oper-fund-empty-error");
    }
    if (candidateVotes.size() > IniCfg().GetTotalDelegateNum()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, the deletegates number a transaction can't exceeds maximum"),
            REJECT_INVALID, "deletegates-number-error");
    }

    CKeyID sendTxKeyID;
    if (!cw.accountCache.GetKeyId(txUid, sendTxKeyID)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get keyId error by CUserID =%s",
                        txUid.ToString()), REJECT_INVALID, "");
    }

    CAccount sendAcct;
    if (!cw.accountCache.GetAccount(txUid, sendAcct)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, userid=%s",
                        txUid.ToString()), REJECT_INVALID, "bad-read-accountdb");
    }
    if (!sendAcct.IsRegistered()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, account unregistered"),
                        REJECT_INVALID, "bad-account-unregistered");
    }

    if (GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2) {
        IMPLEMENT_CHECK_TX_SIGNATURE(sendAcct.pubKey);
    }

    // check delegate duplication
    set<string> voteKeyIds;
    for (const auto &vote : candidateVotes) {
        if (0 >= vote.GetVotedBcoins() || (uint64_t)GetBaseCoinMaxMoney() < vote.GetVotedBcoins())
            return ERRORMSG("CDelegateVoteTx::CheckTx, votes: %lld not within (0 .. MaxVote)", vote.GetVotedBcoins());

        voteKeyIds.insert(vote.GetCandidateUid().ToString());
        CAccount account;
        if (!cw.accountCache.GetAccount(vote.GetCandidateUid(), account))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, address=%s",
                             vote.GetCandidateUid().ToString()), REJECT_INVALID, "bad-read-accountdb");

        if (GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2) {
            if (!account.IsRegistered()) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, account is unregistered, address=%s",
                                 vote.GetCandidateUid().ToString()), REJECT_INVALID, "bad-read-accountdb");
            }
        }
    }

    if (voteKeyIds.size() != candidateVotes.size()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, duplication vote fund"),
                         REJECT_INVALID, "deletegates-duplication fund-error");
    }

    return true;
}

bool CDelegateVoteTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read regist addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog acctLog(account); //save account state before modification
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate account failed ,regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }
    if (!account.ProcessDelegateVote(candidateVotes, nHeight)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate vote failed ,regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error",
                        account.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    cw.txUndo.accountLogs.push_back(acctLog); //keep the old state after the above operation completed properly.
    cw.txUndo.txHash = GetHash();

    for (const auto &vote : candidateVotes) {
        CAccount delegate;
        const CUserID &delegateUId = vote.GetCandidateUid();
        if (!cw.accountCache.GetAccount(delegateUId, delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read KeyId(%s) account info error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccountLog delegateAcctLog(delegate);
        if (!delegate.StakeVoteBcoins(VoteType(vote.GetCandidateVoteType()), vote.GetVotedBcoins())) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate address %s vote fund error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }
        cw.txUndo.accountLogs.push_back(delegateAcctLog); // keep delegate state before modification

        // set the new value and erase the old value
        CDbOpLogs& opLogs = cw.txUndo.mapDbOpLogs[COMMON_OP];
        CDbOpLog operDbLog;
        if (!cw.contractCache.SetDelegateData(delegate, operDbLog)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, save account id %s vote info error",
                            delegate.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
        opLogs.push_back(operDbLog);

        CDbOpLog eraseDbLog;
        if (delegateAcctLog.receivedVotes > 0) {
            if(!cw.contractCache.EraseDelegateData(delegateAcctLog, eraseDbLog)) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, erase account id %s vote info error",
                                delegateAcctLog.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }
        }

        if (!cw.accountCache.SaveAccount(delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error",
                            account.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }

        opLogs.push_back(eraseDbLog);
    }

    IMPLEMENT_PERSIST_TX_KEYID(txUid, CUserID());

    return true;
}

bool CDelegateVoteTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100,
                             ERRORMSG("CDelegateVoteTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    CDbOpLogs& opLogs = cw.txUndo.mapDbOpLogs[COMMON_OP];
    auto rIterScriptDBLog = opLogs.rbegin();
    for (; rIterScriptDBLog != opLogs.rend(); ++rIterScriptDBLog) {
        // Recover the old value and erase the new value.
        if (!cw.contractCache.SetDelegateData(rIterScriptDBLog->key))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, set delegate data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

        ++rIterScriptDBLog;
        if (!cw.contractCache.EraseDelegateData(rIterScriptDBLog->key))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, erase delegate data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    return true;
}