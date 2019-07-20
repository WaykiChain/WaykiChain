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

    void SetBaseViewPtr(CDelegateDBCache *pBaseIn) {
        voteRegIdCache.SetBase(&pBaseIn->voteRegIdCache);
        regId2VoteCache.SetBase(&pBaseIn->regId2VoteCache);
    }

    bool LoadTopDelegates();
    bool ExistDelegate(const CRegID &regId);
    bool GetTopDelegates(vector<CRegID> &delegatesList);

    bool SetDelegateVotes(const CRegID &regId, const uint64_t votes);
    bool EraseDelegateVotes(const CRegID &regId, const uint64_t votes);

    bool SetCandidateVotes(const CRegID &regId, const vector<CCandidateVote> &candidateVotes,
                           CDBOpLogMap &dbOpLogMap);
    bool GetCandidateVotes(const CRegID &regId, vector<CCandidateVote> &candidateVotes);
    bool UndoCandidateVotes(CDBOpLogMap &dbOpLogMap);


    bool Flush();
    uint32_t GetCacheSize() const;
    void Clear();

private:
/*  CDBScalarValueCache  prefixType     key                         value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // vote{(uint64t)MAX - $votedBcoins}{$RegId} -> 1
    CDBMultiValueCache<dbk::VOTE,       std::pair<string, string>,  uint8_t>                voteRegIdCache;
    CDBMultiValueCache<dbk::REGID_VOTE, string/* CRegID */,         vector<CCandidateVote>> regId2VoteCache;

    vector<CRegID> delegateRegIds;
};

#endif // PERSIST_DELEGATEDB_H