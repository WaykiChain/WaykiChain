// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegatedb.h"

#include "configuration.h"

bool CDelegateCache::LoadTopDelegates() {
    delegateRegIds.clear();

    // vote{(uint64t)MAX - $votedBcoins}{$RegId} --> 1
    set<std::pair<string, string> > regIds;
    voteRegIdCache.GetTopNElements(IniCfg().GetTotalDelegateNum(), regIds);

    assert(regIds.size() == IniCfg().GetTotalDelegateNum());

    for (const auto &regId : regIds) {
        delegateRegIds.push_back(CRegID(std::get<1>(regId)));
    }

    assert(delegateRegIds.size() == IniCfg().GetTotalDelegateNum());

    return true;
}

bool CDelegateCache::ExistDelegate(const CRegID &delegateRegId) {
    if (delegateRegIds.empty()) {
        LoadTopDelegates();
    }

    return std::find(delegateRegIds.begin(), delegateRegIds.end(), delegateRegId) != delegateRegIds.end();
}

bool CDelegateCache::GetTopDelegates(vector<CRegID> &delegatesList) {
    if (delegateRegIds.size() != IniCfg().GetTotalDelegateNum()) {
        return false;
    }

    delegatesList = delegateRegIds;

    return true;
}

bool CDelegateCache::SetDelegateVotes(const CRegID &regId, const uint64_t votes) {
    if (votes == 0) {
        return true;
    }

    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - votes);
    auto key                  = std::make_pair(strVotes, regId.ToRawString());
    static uint8_t value      = 1;

    return voteRegIdCache.SetData(key, value);
}

bool CDelegateCache::EraseDelegateVotes(const CRegID &regId, const uint64_t votes) {
    if (votes == 0) {
        return true;
    }

    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - votes);
    auto oldKey               = std::make_pair(strVotes, regId.ToRawString());

    return voteRegIdCache.EraseData(oldKey);
}

bool CDelegateCache::SetCandidateVotes(const CRegID &regId, const vector<CCandidateVote> &candidateVotes) {
    return regId2VoteCache.SetData(regId.ToRawString(), candidateVotes);
}

bool CDelegateCache::GetCandidateVotes(const CRegID &regId, vector<CCandidateVote> &candidateVotes) {
    return regId2VoteCache.GetData(regId.ToRawString(), candidateVotes);
}

bool CDelegateCache::UndoData(dbk::PrefixType prefixType, const CDbOpLogs &dbOpLogs) {
    for (auto it = dbOpLogs.rbegin(); it != dbOpLogs.rend(); ++it) {
        auto &dbOpLog = *it;
        switch (dbOpLog.GetPrefixType()) {
            case dbk::REGID_VOTE:
                return regId2VoteCache.UndoData(dbOpLog);
            default:
                LogPrint("ERROR", "CDelegateCache::UndoData can not handle the dbOpLog=", dbOpLog.ToString());
                return false;
        }
    }

    return true;
}
