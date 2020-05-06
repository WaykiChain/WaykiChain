// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegatedb.h"

#include "config/configuration.h"


static const int BP_DELEGATE_VOTE_MIN = 21000;

inline static CFixedUInt64 DelegateVoteToKey(uint64_t votes) {
    static_assert(ULONG_MAX == 0xFFFFFFFFFFFFFFFF, "ULONG_MAX == 0xFFFFFFFFFFFFFFFF");
    return CFixedUInt64(ULONG_MAX - votes);
}

inline static uint64_t DelegateVoteFromKey(const CFixedUInt64 &voteKey) {
    static_assert(ULONG_MAX == 0xFFFFFFFFFFFFFFFF, "ULONG_MAX == 0xFFFFFFFFFFFFFFFF");
    return ULONG_MAX - voteKey.value;
}

uint64_t CTopDelegatesIterator::GetVote() const {
    return DelegateVoteFromKey(GetKey().first);
}

CRegID CTopDelegatesIterator::GetRegId() const {
    return CRegID(GetKey().second);
}

bool CDelegateDBCache::GetTopVoteDelegates(uint32_t delegateNum, uint64_t BpMinVote,
                                           VoteDelegateVector &topVoteDelegates, bool isR3Fork) {

    topVoteDelegates.clear();
    topVoteDelegates.reserve(delegateNum);
    auto spIt = CreateTopDelegateIterator();
    for (spIt->First(); spIt->IsValid() && topVoteDelegates.size() < delegateNum; spIt->Next()) {
        uint64_t vote = spIt->GetVote();
        if (isR3Fork && vote < BpMinVote) {
            LogPrint(BCLog::INFO, "[WARN] the %lluTH delegate vote=%llu less than %llu!"
                     " dest_delegate_num=%d\n",
                     topVoteDelegates.size(), spIt->GetVote(), BpMinVote, delegateNum);
            break;
        }
        topVoteDelegates.emplace_back(spIt->GetRegId(), spIt->GetVote());
    }

    if (topVoteDelegates.empty())
        return ERRORMSG("[WARN] topVoteDelegates is empty! expected size=%d\n", delegateNum);

    if (topVoteDelegates.size() != delegateNum) {
        LogPrint(BCLog::INFO, "[WARN] the top delegates size=%d is less than %d"
                    " expected_delegates_size=%d\n",
                    topVoteDelegates.size(), BpMinVote, delegateNum);
    }

    return true;
}

bool CDelegateDBCache::SetDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty()) {
        return true;
    }
    auto key = std::make_pair(DelegateVoteToKey(votes), regId.GetRegIdRaw());
    return voteRegIdCache.SetData(key, 1);
}

bool CDelegateDBCache::EraseDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty()) {
        return true;
    }
    auto key = std::make_pair(DelegateVoteToKey(votes), regId.GetRegIdRaw());

    return voteRegIdCache.EraseData(key);
}


int32_t CDelegateDBCache::GetLastVoteHeight() {
    CVarIntValue<uint32_t> height;
    if (last_vote_height_cache.GetData(height))
        return (int32_t)(height.get());
    else
        return 0;
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
    VoteDelegateVector dv;
    if (GetActiveDelegates(dv)){
        return dv.size();
    }
    throw runtime_error("get actived delegate num error");
}

bool CDelegateDBCache::IsActiveDelegate(const CRegID &regid) {
    VoteDelegate delegate;

    return GetActiveDelegate(regid, delegate);
}

bool CDelegateDBCache::GetActiveDelegate(const CRegID &regid, VoteDelegate &voteDelegate) {
    VoteDelegateVector delegates;
    if (!GetActiveDelegates(delegates))
        return false;

    auto it = std::find_if (delegates.begin(), delegates.end(), [&regid](const VoteDelegate &item){
        return item.regid == regid;
    });
    if (it == delegates.end()) {
        return false;
    }
    voteDelegate = *it;
    return true;
}

bool CDelegateDBCache::GetActiveDelegates(VoteDelegateVector &voteDelegates) {
     bool res =  active_delegates_cache.GetData(voteDelegates);
     if (res){
         assert(voteDelegates.size() != 0 );
     }
     return res;
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

bool CDelegateDBCache::GetVoterList(map<CRegIDKey, vector<CCandidateReceivedVote>> &voters) {
    auto dbIt = MakeDbIterator(regId2VoteCache);
    for (dbIt->First(); dbIt->IsValid(); dbIt->Next()) {
        voters[dbIt->GetKey()] = dbIt->GetValue();
    }
    return true;
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

shared_ptr<CTopDelegatesIterator> CDelegateDBCache::CreateTopDelegateIterator() {
    return make_shared<CTopDelegatesIterator>(voteRegIdCache);
}