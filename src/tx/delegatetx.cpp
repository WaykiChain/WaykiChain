// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "delegatetx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "commons/util/util.h"
#include "main.h"
#include "vm/luavm/luavmrunenv.h"
#include "miner/miner.h"
#include "config/version.h"

bool CDelegateVoteTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    if (candidateVotes.empty() || candidateVotes.size() > IniCfg().GetMaxVoteCandidateNum()) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, candidate votes out of range"), REJECT_INVALID,
                         "candidate-votes-out-of-range");
    }

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, public key is invalid"), REJECT_INVALID,
                        "bad-publickey");

    for (const auto &vote : candidateVotes) {
        // candidate uid should be CPubKey or CRegID
        IMPLEMENT_CHECK_TX_CANDIDATE_REGID_OR_PUBKEY(vote.GetCandidateUid());

        if (0 >= vote.GetVotedBcoins() || (uint64_t)GetBaseCoinMaxMoney() < vote.GetVotedBcoins())
            return ERRORMSG("CDelegateVoteTx::CheckTx, votes: %lld not within (0 .. MaxVote)", vote.GetVotedBcoins());

        auto spCandidateAcct = GetAccount(context, vote.GetCandidateUid(), "candidate_uid");
        if (!spCandidateAcct) return false;

        if (GetFeatureForkVersion(context.height) >= MAJOR_VER_R2) {
            if (!spCandidateAcct->HasOwnerPubKey()) {
                return state.DoS(100, ERRORMSG("CDelegateVoteTx::CheckTx, account is unregistered, address=%s",
                                 vote.GetCandidateUid().ToString()), REJECT_INVALID, "bad-read-accountdb");
            }
        }
    }

    return true;
}

bool CDelegateVoteTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    vector<CCandidateReceivedVote> candidateVotesInOut;
    CRegID &regId = sp_tx_account->regid;
    cw.delegateCache.GetCandidateVotes(regId, candidateVotesInOut);

    if (!sp_tx_account->ProcessCandidateVotes(candidateVotes, candidateVotesInOut, context.height, context.block_time,
                                          cw.accountCache, receipts)) {
        return state.DoS(
            100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate candidate votes failed, txUid=%s", txUid.ToString()),
            OPERATE_CANDIDATE_VOTES_FAIL, "operate-candidate-votes-failed");
    }
    if (!cw.delegateCache.SetCandidateVotes(regId, candidateVotesInOut)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, write candidate votes failed, txUid=%s", txUid.ToString()),
                        WRITE_CANDIDATE_VOTES_FAIL, "write-candidate-votes-failed");
    }

    for (const auto &vote : candidateVotes) {
        const CUserID &delegateUId = vote.GetCandidateUid();

        auto spCandidateAcct = GetAccount(context, vote.GetCandidateUid(), "candidate_uid");
        if (!spCandidateAcct) return false;

        shared_ptr<CAccount> spDelegateAccount = nullptr;
        uint64_t oldVotes = spDelegateAccount->received_votes;
        if (!spDelegateAccount->StakeVoteBcoins(VoteType(vote.GetCandidateVoteType()), vote.GetVotedBcoins())) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, operate account id %s vote fund error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }

        // Votes: set the new value and erase the old value
        if (!cw.delegateCache.SetDelegateVotes(spDelegateAccount->regid, spDelegateAccount->received_votes)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, save account id %s vote info error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-delegatedb");
        }

        if (!cw.delegateCache.EraseDelegateVotes(spDelegateAccount->regid, oldVotes)) {
            return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, erase account id %s vote info error",
                            delegateUId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-delegatedb");
        }
    }

    if (!cw.delegateCache.SetLastVoteHeight(context.height)) {
        return state.DoS(100, ERRORMSG("CDelegateVoteTx::ExecuteTx, save last vote height error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-last-vote-height");
    }

    return true;
}

string CDelegateVoteTx::ToString(CAccountDBCache &accountCache) {
    string str;

    str += strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%llu, valid_height=%d", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), llFees, valid_height);
    str += "vote: ";
    for (const auto &vote : candidateVotes) {
        str += strprintf("%s", vote.ToString());
    }

    return str;
}

Object CDelegateVoteTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    Array candidateVoteArray;
    for (const auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    result.push_back(Pair("candidate_votes",    candidateVoteArray));
    return result;
}
