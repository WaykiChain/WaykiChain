// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegatedb.h"

#include "config/configuration.h"

bool CDelegateDBCache::ExistDelegate(const CRegID &regid) {
    DelegateVector delegates;
    if (!GetActiveDelegates(delegates))
        return false;

    return std::find(delegates.begin(), delegates.end(), regid) != delegates.end();
}

bool CDelegateDBCache::GetTopVoteDelegates(VoteDelegateVector &topVotedDelegates) {

    // votes{(uint64t)MAX - $votedBcoins}{$RegId} --> 1
    set<decltype(voteRegIdCache)::KeyType> topKeys;
    voteRegIdCache.GetTopNElements(IniCfg().GetTotalDelegateNum(), topKeys);

    // assert(regIds.size() == IniCfg().GetTotalDelegateNum());

    for (const auto &key : topKeys) {
        const string &votesStr = std::get<0>(key);
        const string &regIdStr = std::get<1>(key);
        VoteDelegate votedDelegate;
        votedDelegate.regid.SetRegID(UnsignedCharArray(regIdStr.begin(), regIdStr.end()));
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
    auto key                  = std::make_pair(strVotes, regId.ToRawString());
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
    auto oldKey               = std::make_pair(strVotes, regId.ToRawString());

    return voteRegIdCache.EraseData(oldKey);
}


int32_t CDelegateDBCache::GetLastVoteHeight() {
    uint32_t height;
    if (!last_vote_height_cache.GetData(height))
        height = 0;
    return (int32_t)height;
}

bool CDelegateDBCache::SetLastVoteHeight(int32_t height) {
    return last_vote_height_cache.SetData((uint32_t)height);
}


bool CDelegateDBCache::GetPendingDelegates(PendingDelegates &delegates) {
    return pending_delegates_cache.GetData(delegates);
}

bool CDelegateDBCache::SetPendingDelegates(const PendingDelegates &delegates) {
    return pending_delegates_cache.SetData(delegates);

}

bool CDelegateDBCache::GetActiveDelegates(DelegateVector &delegates) {
    return active_delegates_cache.GetData(delegates);
}

bool CDelegateDBCache::SetActiveDelegates(const DelegateVector &delegates) {
    return active_delegates_cache.SetData(delegates);
}

bool CDelegateDBCache::SetCandidateVotes(const CRegID &regId,
                                       const vector<CCandidateReceivedVote> &candidateVotes) {
    return regId2VoteCache.SetData(regId.ToRawString(), candidateVotes);
}

bool CDelegateDBCache::GetCandidateVotes(const CRegID &regId, vector<CCandidateReceivedVote> &candidateVotes) {
    return regId2VoteCache.GetData(regId.ToRawString(), candidateVotes);
}

bool CDelegateDBCache::GetVoterList(map<string/* CRegID */, vector<CCandidateReceivedVote>> &regId2Vote) {
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