// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegatedb.h"

#include "config/configuration.h"

bool CDelegateDBCache::LoadTopDelegateList() {
    delegateRegIds.clear();

    // vote{(uint64t)MAX - $votedBcoins}{$RegId} --> 1
    set<std::pair<string, string> > regIds;
    voteRegIdCache.GetTopNElements(IniCfg().GetTotalDelegateNum(), regIds);

    // assert(regIds.size() == IniCfg().GetTotalDelegateNum());

    for (const auto &regId : regIds) {
        string strRegId = std::get<1>(regId);
        delegateRegIds.push_back(CRegID(UnsignedCharArray(strRegId.begin(), strRegId.end())));
    }

    return true;
}

bool CDelegateDBCache::ExistDelegate(const CRegID &delegateRegId) {
    if (delegateRegIds.empty()) {
        LoadTopDelegateList();
    }

    return std::find(delegateRegIds.begin(), delegateRegIds.end(), delegateRegId) != delegateRegIds.end();
}

bool CDelegateDBCache::GetTopDelegateList(vector<CRegID> &delegatesList) {
    if (delegateRegIds.empty()) {
        LoadTopDelegateList();
    }

    if (delegateRegIds.size() != IniCfg().GetTotalDelegateNum()) {
        LogPrint("ERROR", "CDelegateDBCache::GetTopDelegateList, only got %lu delegates(need %u)\n", delegateRegIds.size(),
                 IniCfg().GetTotalDelegateNum());
        return false;
    }

    delegatesList = delegateRegIds;

    return true;
}

bool CDelegateDBCache::SetDelegateVotes(const CRegID &regId, const uint64_t votes) {
    // If CRegID is empty, ignore received votes for forward compatibility.
    if (regId.IsEmpty() || votes == 0) {
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
    if (regId.IsEmpty() || votes == 0) {
        return true;
    }

    delegateRegIds.clear();

    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - votes);
    auto oldKey               = std::make_pair(strVotes, regId.ToRawString());

    return voteRegIdCache.EraseData(oldKey);
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

    return true;
}

uint32_t CDelegateDBCache::GetCacheSize() const {
    return voteRegIdCache.GetCacheSize() + regId2VoteCache.GetCacheSize();
}

void CDelegateDBCache::Clear() {
    voteRegIdCache.Clear();
    regId2VoteCache.Clear();
}