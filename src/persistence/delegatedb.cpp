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

const CRegID &CTopDelegatesIterator::GetRegid() const {
    return GetKey().second.regid;
}

bool CDelegateDBCache::GetTopVoteDelegates(uint32_t delegateNum, uint64_t delegateVoteMin,
                                           VoteDelegateVector &topVoteDelegates, bool isR3Fork) {

    topVoteDelegates.clear();
    topVoteDelegates.reserve(delegateNum);
    auto spIt = CreateTopDelegateIterator();
    for (spIt->First(); spIt->IsValid() && topVoteDelegates.size() < delegateNum; spIt->Next()) {
        uint64_t vote = spIt->GetVote();
        if (isR3Fork && vote < BP_DELEGATE_VOTE_MIN) {
            LogPrint(BCLog::ERROR, "[WARNING] %s, the %lluTH delegate vote=%llu less than %llu!"
                     " dest_delegate_num=%d\n",
                     __func__, topVoteDelegates.size(), BP_DELEGATE_VOTE_MIN, delegateNum);
            break;
        }
        topVoteDelegates.emplace_back(spIt->GetRegid(), spIt->GetVote());
    }
    if (topVoteDelegates.empty())
        return ERRORMSG("[WARNING] %s, topVoteDelegates is empty! dest_delegate_num=%d\n",
                    __func__, delegateNum);
    if (topVoteDelegates.size() != delegateNum) {
        LogPrint(BCLog::INFO, "[WARNING] %s, the top delegates size=%d is less than %d"
                    " specified_delegate_num=%d\n",
                    __func__, topVoteDelegates.size(), BP_DELEGATE_VOTE_MIN, delegateNum);
    }
    return true;
}

bool CDelegateDBCache::SetDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty()) {
        return true;
    }
    auto key = std::make_pair(DelegateVoteToKey(votes), CRegIDKey(regId));
    return voteRegIdCache.SetData(key, 1);
}

bool CDelegateDBCache::EraseDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty()) {
        return true;
    }
    auto key = std::make_pair(DelegateVoteToKey(votes), CRegIDKey(regId));

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

shared_ptr<CTopDelegatesIterator> CDelegateDBCache::CreateTopDelegateIterator() {
    return make_shared<CTopDelegatesIterator>(voteRegIdCache);
}