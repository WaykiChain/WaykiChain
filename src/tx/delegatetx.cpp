// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "delegatetx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "commons/util.h"
#include "main.h"
#include "vm/luavm/vmrunenv.h"
#include "miner/miner.h"
#include "config/version.h"

string CDelegateVoteTx::ToString(CAccountDBCache &accountCache) {
    string str;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, keyId.ToAddress(), keyId.ToString());
    str += "vote:\n";
    for (const auto &vote : candidateVotes) {
        str += strprintf("%s", vote.ToString());
    }
    return str;
}

Object CDelegateVoteTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    Array candidateVoteArray;
    for (const auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    result.push_back(Pair("tx_hash",            GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("tx_addr",            keyId.ToAddress()));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("candidate_votes",    candidateVoteArray));
    result.push_back(Pair("valid_height",       nValidHeight));

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


bool CDelegateVoteTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
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
    if (!sendAcct.HaveOwnerPubKey()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, account unregistered"),
                        REJECT_INVALID, "bad-account-unregistered");
    }

    if (GetFeatureForkVersion(nHeight) == MAJOR_VER_R2) {
        IMPLEMENT_CHECK_TX_SIGNATURE(sendAcct.owner_pubkey);
    }

    // check candidate duplication
    set<CKeyID> voteKeyIds;
    for (const auto &vote : candidateVotes) {
        // candidate uid should be CPubKey or CRegID
        IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(vote.GetCandidateUid().type());

        if (0 >= vote.GetVotedBcoins() || (uint64_t)GetBaseCoinMaxMoney() < vote.GetVotedBcoins())
            return ERRORMSG("CDelegateVoteTx::CheckTx, votes: %lld not within (0 .. MaxVote)", vote.GetVotedBcoins());

        CAccount account;
        if (!cw.accountCache.GetAccount(vote.GetCandidateUid(), account))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, address=%s",
                             vote.GetCandidateUid().ToString()), REJECT_INVALID, "bad-read-accountdb");
        if (vote.GetCandidateUid().type() == typeid(CPubKey)) {
            voteKeyIds.insert(vote.GetCandidateUid().get<CPubKey>().GetKeyId());
        } else {  // vote.GetCandidateUid().type() == typeid(CRegID)
            voteKeyIds.insert(account.keyid);
        }

        if (GetFeatureForkVersion(nHeight) == MAJOR_VER_R2) {
            if (!account.HaveOwnerPubKey()) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, account is unregistered, address=%s",
                                 vote.GetCandidateUid().ToString()), REJECT_INVALID, "bad-read-accountdb");
            }
        }
    }

    if (voteKeyIds.size() != candidateVotes.size()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, duplication candidate"), REJECT_INVALID,
                         "duplication-candidate-error");
    }

    return true;
}

bool CDelegateVoteTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read regist addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccount acctLog(account);  // Keep account state before modification.
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate account failed, regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }

    vector<CCandidateVote> candidateVotesInOut;
    CRegID regId = txUid.get<CRegID>();
    cw.delegateCache.GetCandidateVotes(regId, candidateVotesInOut);

    if (!account.ProcessDelegateVotes(candidateVotes, candidateVotesInOut, nHeight, &cw.accountCache)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate vote failed, regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
    }
    if (!cw.delegateCache.SetCandidateVotes(regId, candidateVotesInOut, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, write candidate votes failed, regId=%s", txUid.ToString()),
                        WRITE_CANDIDATE_VOTES_FAIL, "write-candidate-votes-failed");
    }

    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error",
                        account.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    // Keep the old state after the above operation completed properly.
    cw.txUndo.accountLogs.push_back(acctLog);
    cw.txUndo.txid = GetHash();

    for (const auto &vote : candidateVotes) {
        CAccount delegate;
        const CUserID &delegateUId = vote.GetCandidateUid();
        if (!cw.accountCache.GetAccount(delegateUId, delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read KeyId(%s) account info error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccount delegateAcctLog(delegate);
        if (!delegate.StakeVoteBcoins(VoteType(vote.GetCandidateVoteType()), vote.GetVotedBcoins())) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate address %s vote fund error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }
        cw.txUndo.accountLogs.push_back(delegateAcctLog); // Keep delegate state before modification.

        // Votes: set the new value and erase the old value
        if (!cw.delegateCache.SetDelegateVotes(delegate.regid, delegate.received_votes)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, save account id %s vote info error",
                            delegate.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }

        if (!cw.delegateCache.EraseDelegateVotes(delegateAcctLog.regid, delegateAcctLog.received_votes)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, erase account id %s vote info error",
                            delegateAcctLog.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }

        if (!cw.accountCache.SaveAccount(delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error",
                            account.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
    }

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid}))
        return false;

    return true;
}

bool CDelegateVoteTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    if (!UndoTxAddresses(cw, state))
        return false;

    vector<CAccount>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        if (!cw.accountCache.GetAccount(CUserID(rIterAccountLog->keyid), account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        // Votes: recover the old value and erase the new value.
        if (account.received_votes != rIterAccountLog->received_votes) {
            if (!cw.delegateCache.EraseDelegateVotes(account.regid, account.received_votes)) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, save account id %s vote info error",
                                account.regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }

            if (!cw.delegateCache.SetDelegateVotes(rIterAccountLog->regid, rIterAccountLog->received_votes)) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, erase account id %s vote info error",
                                rIterAccountLog->regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100,ERRORMSG("CDelegateVoteTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(CUserID(rIterAccountLog->keyid), account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    if (!cw.delegateCache.UndoCandidateVotes(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, UndoCandidateVotes error"),
                         UPDATE_ACCOUNT_FAIL, "undo-candidate-votes-failed");
    }

    return true;
}