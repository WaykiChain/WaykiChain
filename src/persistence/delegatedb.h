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

class CDelegateDBCache {
public:
    CDelegateDBCache(){};
    CDelegateDBCache(CDBAccess *pDbAccess) : voteRegIdCache(pDbAccess), regId2VoteCache(pDbAccess){};
    CDelegateDBCache(CDelegateDBCache *pBaseIn)
        : voteRegIdCache(pBaseIn->voteRegIdCache), regId2VoteCache(pBaseIn->regId2VoteCache){};

    bool LoadTopDelegateList();
    bool ExistDelegate(const CRegID &regId);
    bool GetTopDelegateList(vector<CRegID> &delegatesList);

    bool SetDelegateVotes(const CRegID &regId, const uint64_t votes);
    bool EraseDelegateVotes(const CRegID &regId, const uint64_t votes);

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
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        voteRegIdCache.SetDbOpLogMap(pDbOpLogMapIn);
        regId2VoteCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return voteRegIdCache.UndoDatas() &&
               regId2VoteCache.UndoDatas();
    }
private:
/*  CSimpleKVCache  prefixType     key                         value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // vote{(uint64t)MAX - $votedBcoins}{$RegId} -> 1
    CCompositeKVCache<dbk::VOTE,       std::pair<string, string>,  uint8_t>                voteRegIdCache;
    CCompositeKVCache<dbk::REGID_VOTE, string/* CRegID */,         vector<CCandidateReceivedVote>> regId2VoteCache;

    vector<CRegID> delegateRegIds;
};

#endif // PERSIST_DELEGATEDB_H