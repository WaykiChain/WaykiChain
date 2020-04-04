// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DELEGATEDB_H
#define PERSIST_DELEGATEDB_H

#include "entities/account.h"
#include "entities/id.h"
#include "entities/vote.h"
#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbiterator.h"
#include "dbconf.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

/*  CCompositeKVCache  prefixType     key                              value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // {vote(MAX - $votedBcoins)}{$RegId} -> 1
    // vote(MAX - $votedBcoins) save as CFixedUInt64 to ensure that the keys are sorted by vote value from big to small
typedef CCompositeKVCache<dbk::VOTE,  std::pair<CFixedUInt64, CRegIDKey>,  uint8_t>         CVoteRegIdCache;

class CTopDelegatesIterator: public CDbIterator<CVoteRegIdCache> {
public:
    typedef CDbIterator<CVoteRegIdCache> Base;
    using Base::Base;

    uint64_t GetVote() const;

    const CRegID &GetRegid() const;
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

    bool GetTopVoteDelegates(uint32_t delegateNum, uint64_t delegateVoteMin,
                             VoteDelegateVector &topVoteDelegates, bool isR3Fork);

    bool SetDelegateVotes(const CRegID &regid, const uint64_t votes);
    bool EraseDelegateVotes(const CRegID &regid, const uint64_t votes);

    int32_t GetLastVoteHeight();
    bool SetLastVoteHeight(int32_t height);
    uint32_t GetActivedDelegateNum();

    bool GetPendingDelegates(PendingDelegates &delegates);
    bool SetPendingDelegates(const PendingDelegates &delegates);

    bool IsActiveDelegate(const CRegID &regid);
    bool GetActiveDelegate(const CRegID &regid, VoteDelegate &voteDelegate);
    bool GetActiveDelegates(VoteDelegateVector &voteDelegates);
    bool SetActiveDelegates(const VoteDelegateVector &voteDelegates);

    bool SetCandidateVotes(const CRegID &regid, const vector<CCandidateReceivedVote> &candidateVotes);
    bool GetCandidateVotes(const CRegID &regid, vector<CCandidateReceivedVote> &candidateVotes);

    // There’s no reason to worry about performance issues as it will used only in stable coin genesis height.
    bool GetVoterList(map<CRegIDKey, vector<CCandidateReceivedVote>> &regId2Vote);

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

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        voteRegIdCache.RegisterUndoFunc(undoDataFuncMap);
        regId2VoteCache.RegisterUndoFunc(undoDataFuncMap);
        last_vote_height_cache.RegisterUndoFunc(undoDataFuncMap);
        pending_delegates_cache.RegisterUndoFunc(undoDataFuncMap);
        active_delegates_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CTopDelegatesIterator> CreateTopDelegateIterator();
public:
/*  CCompositeKVCache  prefixType     key                              value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // {vote(MAX - $votedBcoins)}{$RegId} -> 1
    // vote(MAX - $votedBcoins) save as CFixedUInt64 to ensure that the keys are sorted by vote value from big to small
    CVoteRegIdCache voteRegIdCache;

    CCompositeKVCache<dbk::REGID_VOTE, CRegIDKey,         vector<CCandidateReceivedVote>> regId2VoteCache;

    CSimpleKVCache<dbk::LAST_VOTE_HEIGHT, CVarIntValue<uint32_t>> last_vote_height_cache;
    CSimpleKVCache<dbk::PENDING_DELEGATES, PendingDelegates> pending_delegates_cache;
    CSimpleKVCache<dbk::ACTIVE_DELEGATES, VoteDelegateVector> active_delegates_cache;

    vector<CRegID> delegateRegIds;
};

#endif // PERSIST_DELEGATEDB_H