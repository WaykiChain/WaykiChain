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
    // if (!cw.pAccountCache->GetKeyId(userId, keyId))
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
    if (!cw.pAccountCache->GetKeyId(txUid, sendTxKeyID)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get keyId error by CUserID =%s",
                        txUid.ToString()), REJECT_INVALID, "");
    }

    CAccount sendAcct;
    if (!cw.pAccountCache->GetAccount(txUid, sendAcct)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, userid=%s",
                        txUid.ToString()), REJECT_INVALID, "bad-read-accountdb");
    }
    if (!sendAcct.IsRegistered()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, pubkey not registered"),
                        REJECT_INVALID, "bad-no-pubkey");
    }

    if ( GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2 ) {
        IMPLEMENT_CHECK_TX_SIGNATURE(sendAcct.pubKey);
    }

    // check delegate duplication
    set<string> voteKeyIds;
    for (const auto &vote : candidateVotes) {
        if (0 >= vote.GetVotedBcoins() || (uint64_t)GetBaseCoinMaxMoney() < vote.GetVotedBcoins())
            return ERRORMSG("CDelegateVoteTx::CheckTx, votes: %lld not within (0 .. MaxVote)", vote.GetVotedBcoins());

        voteKeyIds.insert(vote.GetCandidateUid().ToString());
        CAccount acctInfo;
        if (!cw.pAccountCache->GetAccount(vote.GetCandidateUid(), acctInfo))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, get account info error, address=%s",
                             vote.GetCandidateUid().ToString()), REJECT_INVALID, "bad-read-accountdb");

        if (GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2) {
            if (!acctInfo.IsRegistered()) {
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
    CAccount acctInfo;
    if (!cw.pAccountCache->GetAccount(txUid, acctInfo)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read regist addr %s account info error", txUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog acctInfoLog(acctInfo); //save account state before modification
    if (!acctInfo.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate account failed ,regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }
    if (!acctInfo.ProcessDelegateVote(candidateVotes, nHeight)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate vote failed ,regId=%s", txUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
    }
    if (!cw.pAccountCache->SaveAccount(acctInfo)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error", acctInfo.regID.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    cw.pTxUndo->vAccountLog.push_back(acctInfoLog); //keep the old state after the above operation completed properly.
    cw.pTxUndo->txHash = GetHash();

    for (const auto &vote : candidateVotes) {
        CAccount delegate;
        const CUserID &delegateUId = vote.GetCandidateUid();
        if (!cw.pAccountCache->GetAccount(delegateUId, delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, read KeyId(%s) account info error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccountLog delegateAcctLog(delegate);
        if (!delegate.OperateVote(VoteType(vote.GetCandidateVoteType()), vote.GetVotedBcoins())) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate delegate address %s vote fund error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }
        cw.pTxUndo->vAccountLog.push_back(delegateAcctLog); // keep delegate state before modification

        // set the new value and erase the old value
        CContractDBOperLog operDbLog;
        if (!cw.pContractCache->SetDelegateData(delegate, operDbLog)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, save account id %s vote info error",
                            delegate.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
        cw.pTxUndo->vContractOperLog.push_back(operDbLog);

        CContractDBOperLog eraseDbLog;
        if (delegateAcctLog.receivedVotes > 0) {
            if(!cw.pContractCache->EraseDelegateData(delegateAcctLog, eraseDbLog)) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, erase account id %s vote info error",
                                delegateAcctLog.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }
        }

        if (!cw.pAccountCache->SaveAccount(delegate)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, create new account script id %s script info error",
                            acctInfo.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }

        cw.pTxUndo->vContractOperLog.push_back(eraseDbLog);
    }

    if (SysCfg().GetAddressToTxFlag()) {
        CContractDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if (!cw.pAccountCache->GetKeyId(txUid, sendKeyId)) {
            return ERRORMSG("CDelegateVoteTx::ExecuteTx, get regAcctId by account error!");
        }

        if (!cw.pContractCache->SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, cw.pTxUndo->txHash.GetHex(), operAddressToTxLog))
            return false;

        cw.pTxUndo->vContractOperLog.push_back(operAddressToTxLog);
    }
    return true;
}

bool CDelegateVoteTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.pTxUndo->vAccountLog.rbegin();
    for (; rIterAccountLog != cw.pTxUndo->vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.pAccountCache->GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100,
                             ERRORMSG("CDelegateVoteTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.pAccountCache->SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    vector<CContractDBOperLog>::reverse_iterator rIterScriptDBLog = cw.pTxUndo->vContractOperLog.rbegin();
    if (SysCfg().GetAddressToTxFlag() && cw.pTxUndo->vContractOperLog.size() > 0) {
        if (!cw.pContractCache->UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, undo scriptdb data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        ++rIterScriptDBLog;
    }

    for (; rIterScriptDBLog != cw.pTxUndo->vContractOperLog.rend(); ++rIterScriptDBLog) {
        // Recover the old value and erase the new value.
        if (!cw.pContractCache->SetDelegateData(rIterScriptDBLog->vKey))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, set delegate data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

        ++rIterScriptDBLog;
        if (!cw.pContractCache->EraseDelegateData(rIterScriptDBLog->vKey))
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::UndoExecuteTx, erase delegate data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    return true;
}