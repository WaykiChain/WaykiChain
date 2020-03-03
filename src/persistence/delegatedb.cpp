// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegatedb.h"

#include "config/configuration.h"

bool CDelegateDBCache::GetTopVoteDelegates(uint32_t delegateNum ,VoteDelegateVector &topVotedDelegates) {

    // votes{(uint64t)MAX - $votedBcoins}{$RegId} --> 1
    set<decltype(voteRegIdCache)::KeyType> topKeys;
    voteRegIdCache.GetTopNElements(delegateNum, topKeys);

    for (const auto &key : topKeys) {
        const string &votesStr = std::get<0>(key);
        const CRegIDKey &regIdKey = std::get<1>(key);
        VoteDelegate votedDelegate;
        votedDelegate.regid = regIdKey.regid;
        votedDelegate.votes = std::strtoull(votesStr.c_str(), nullptr, 10);
        topVotedDelegates.push_back(votedDelegate);
    }

    return true;
}

bool CDelegateDBCache::SetDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty()) {
        return true;
    }

    delegateRegIds.clear();

    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - votes);
    auto key                  = std::make_pair(strVotes, CRegIDKey(regId));
    static uint8_t value      = 1;

    return voteRegIdCache.SetData(key, value);
}

bool CDelegateDBCache::EraseDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty()) {
        return true;
    }

    delegateRegIds.clear();

    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - votes);
    auto oldKey               = std::make_pair(strVotes, CRegIDKey(regId));

    return voteRegIdCache.EraseData(oldKey);
}


int32_t CDelegateDBCache::GetLastVoteHeight() {
    CVarIntValue<uint32_t> height;
    if (last_vote_height_cache.GetData(height))
            return (int32_t)(height.get());
    return 0 ;
}

bool CDelegateDBCache::SetLastVoteHeight(int32_t height) {
    return last_vote_height_cache.SetData(CVarIntValue<uint32_t>(height));
}


bool CDelegateDBCache::GetPendingDelegates(PendingDelegates &delegates) {
    return pending_delegates_cache.GetData(delegates);
}

bool CDelegateDBCache::SetPendingDelegates(const PendingDelegates &delegates) {
    return pending_delegates_cache.SetData(delegates);

}
uint32_t CDelegateDBCache::GetActivedDelegateNum() {
    VoteDelegateVector dv ;
    if(GetActiveDelegates(dv)){
        return dv.size() ;
    }
    return 11 ;
}

bool CDelegateDBCache::IsActiveDelegate(const CRegID &regid) {
    VoteDelegate delegate;

    return GetActiveDelegate(regid, delegate);
}

bool CDelegateDBCache::GetActiveDelegate(const CRegID &regid, VoteDelegate &voteDelegate) {
    VoteDelegateVector delegates;
    if (!GetActiveDelegates(delegates))
        return false;

    auto it = std::find_if(delegates.begin(), delegates.end(), [&regid](const VoteDelegate &item){
        return item.regid == regid;
    });
    if (it == delegates.end()) {
        return false;
    }
    voteDelegate = *it;
    return true;
}

bool CDelegateDBCache::GetActiveDelegates(VoteDelegateVector &voteDelegates) {
    return active_delegates_cache.GetData(voteDelegates);
}

bool CDelegateDBCache::SetActiveDelegates(const VoteDelegateVector &voteDelegates) {
    return active_delegates_cache.SetData(voteDelegates);
}

bool CDelegateDBCache::SetCandidateVotes(const CRegID &regId,
                                       const vector<CCandidateReceivedVote> &candidateVotes) {
    return regId2VoteCache.SetData(regId, candidateVotes);
}

bool CDelegateDBCache::GetCandidateVotes(const CRegID &regId, vector<CCandidateReceivedVote> &candidateVotes) {
    return regId2VoteCache.GetData(regId, candidateVotes);
}

bool CDelegateDBCache::GetVoterList(map<CRegIDKey, vector<CCandidateReceivedVote>> &regId2Vote) {
    return regId2VoteCache.GetAllElements(regId2Vote);
}

bool CDelegateDBCache::Flush() {
    voteRegIdCache.Flush();
    regId2VoteCache.Flush();
    last_vote_height_cache.Flush();
    pending_delegates_cache.Flush();
    active_delegates_cache.Flush();

    return true;
}

uint32_t CDelegateDBCache::GetCacheSize() const {
    return  voteRegIdCache.GetCacheSize() +
            regId2VoteCache.GetCacheSize() +
            last_vote_height_cache.GetCacheSize() +
            pending_delegates_cache.GetCacheSize() +
            active_delegates_cache.GetCacheSize();
}

void CDelegateDBCache::Clear() {
    voteRegIdCache.Clear();
    regId2VoteCache.Clear();
    last_vote_height_cache.Clear();
    pending_delegates_cache.Clear();
    active_delegates_cache.Clear();
}