// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DELEGATEDB_H
#define PERSIST_DELEGATEDB_H

#include "entities/account.h"
#include "entities/id.h"
#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

typedef vector<CRegID> DelegateVector;

struct VoteDelegate {
    CRegID regid;       // the voted delegate regid
    uint64_t votes = 0;     // the received votes

    IMPLEMENT_SERIALIZE(
        READWRITE(regid);
        READWRITE(VARINT(votes));
    )
};

enum class VoteDelegateState: uint8_t {
    NONE,           // none, init state
    PENDING,        // pending, wait for activating vote delegates
    ACTIVATED,      // activated, vote delegates is activated
};

typedef vector<VoteDelegate> VoteDelegateVector;
struct PendingDelegates {
    VoteDelegateState state = VoteDelegateState::NONE;  // state
    uint32_t counted_vote_height = 0;                   // counting vote height
    VoteDelegateVector top_vote_delegates;              // top vote delegates

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)state);
        READWRITE(VARINT(counted_vote_height));
        READWRITE(top_vote_delegates);
    )

    bool IsSameDelegates(const DelegateVector &delegates) const {
        if (top_vote_delegates.size() != delegates.size())
            return false;

        for (size_t i = 0; i < top_vote_delegates.size(); i++)
            if (top_vote_delegates[i].regid != delegates[i])
                return false;

        return true;
    }

    DelegateVector GetDelegates() const {
        DelegateVector delegates;
        delegates.resize(top_vote_delegates.size());
        for (size_t i = 0; i < top_vote_delegates.size(); i++) {
            delegates[i] = top_vote_delegates[i].regid;
        }
        return std::move(delegates);
    }

    bool IsEmpty() const {
        return state == VoteDelegateState::NONE && counted_vote_height == 0 && top_vote_delegates.empty();
    }

    void SetEmpty() {
        state = VoteDelegateState::NONE;
        counted_vote_height = 0;
        top_vote_delegates.clear();
    }
};

class CDelegateDBCache {
public:
    CDelegateDBCache() {}
    CDelegateDBCache(CDBAccess *pDbAccess)
        : voteRegIdCache(pDbAccess),
          regId2VoteCache(pDbAccess),
          last_vote_height_cache(pDbAccess),
          pending_delegates_cache(pDbAccess),
          active_delegates_cache(pDbAccess) {}

    CDelegateDBCache(CDelegateDBCache *pBaseIn)
        : voteRegIdCache(pBaseIn->voteRegIdCache),
        regId2VoteCache(pBaseIn->regId2VoteCache),
        last_vote_height_cache(pBaseIn->last_vote_height_cache),
        pending_delegates_cache(pBaseIn->pending_delegates_cache),
        active_delegates_cache(pBaseIn->active_delegates_cache) {}

    bool ExistDelegate(const CRegID &regId);
    bool GetTopVoteDelegates(VoteDelegateVector &topVotedDelegates);

    bool SetDelegateVotes(const CRegID &regId, const uint64_t votes);
    bool EraseDelegateVotes(const CRegID &regId, const uint64_t votes);

    int32_t GetLastVoteHeight();
    bool SetLastVoteHeight(int32_t height);

    bool GetPendingDelegates(PendingDelegates &delegates);
    bool SetPendingDelegates(const PendingDelegates &delegates);

    bool GetActiveDelegates(DelegateVector &delegates);
    bool SetActiveDelegates(const DelegateVector &delegates);

    bool SetCandidateVotes(const CRegID &regId, const vector<CCandidateReceivedVote> &candidateVotes);
    bool GetCandidateVotes(const CRegID &regId, vector<CCandidateReceivedVote> &candidateVotes);

    // There’s no reason to worry about performance issues as it will used only in stable coin genesis height.
    bool GetVoterList(map<string/* CRegID */, vector<CCandidateReceivedVote>> &regId2Vote);

    bool Flush();
    uint32_t GetCacheSize() const;
    void Clear();

    void SetBaseViewPtr(CDelegateDBCache *pBaseIn) {
        voteRegIdCache.SetBase(&pBaseIn->voteRegIdCache);
        regId2VoteCache.SetBase(&pBaseIn->regId2VoteCache);
        last_vote_height_cache.SetBase(&pBaseIn->last_vote_height_cache);
        pending_delegates_cache.SetBase(&pBaseIn->pending_delegates_cache);
        active_delegates_cache.SetBase(&pBaseIn->active_delegates_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        voteRegIdCache.SetDbOpLogMap(pDbOpLogMapIn);
        regId2VoteCache.SetDbOpLogMap(pDbOpLogMapIn);
        last_vote_height_cache.SetDbOpLogMap(pDbOpLogMapIn);
        pending_delegates_cache.SetDbOpLogMap(pDbOpLogMapIn);
        active_delegates_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoData() {
        return  voteRegIdCache.UndoData() &&
                regId2VoteCache.UndoData() &&
                last_vote_height_cache.UndoData() &&
                pending_delegates_cache.UndoData() &&
                active_delegates_cache.UndoData();
    }

private:
/*  CCompositeKVCache  prefixType     key                              value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // vote{(uint64t)MAX - $votedBcoins}{$RegId} -> 1
    CCompositeKVCache<dbk::VOTE,       std::pair<string, string>,  uint8_t>                voteRegIdCache;
    CCompositeKVCache<dbk::REGID_VOTE, string/* CRegID */,         vector<CCandidateReceivedVote>> regId2VoteCache;

    CSimpleKVCache<dbk::LAST_VOTE_HEIGHT, uint32_t> last_vote_height_cache;
    CSimpleKVCache<dbk::PENDING_DELEGATES, PendingDelegates> pending_delegates_cache;
    CSimpleKVCache<dbk::ACTIVE_DELEGATES, DelegateVector> active_delegates_cache;

    vector<CRegID> delegateRegIds;
};

#endif // PERSIST_DELEGATEDB_H