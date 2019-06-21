// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DELEGATEDB_H
#define PERSIST_DELEGATEDB_H

#include "accounts/account.h"
#include "accounts/id.h"
#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

class CDelegateCache {
public:
    CDelegateCache(){};
    CDelegateCache(CDBAccess *pDbAccess) : voteRegIdCache(pDbAccess){};
    CDelegateCache(CDelegateCache *pBaseIn) : voteRegIdCache(pBaseIn->voteRegIdCache){};

    bool LoadTopDelegates();
    bool ExistDelegate(const CRegID &regId);
    bool GetTopDelegates(vector<CRegID> &delegatesList);

    bool SetDelegateVotes(const CRegID &regId, const uint64_t votes);
    bool EraseDelegateVotes(const CRegID &regId, const uint64_t votes);

    bool SetCandidateVotes(const CRegID &regId, const vector<CCandidateVote> &candidateVotes);
    bool GetCandidateVotes(const CRegID &regId, vector<CCandidateVote> &candidateVotes);

    bool UndoData(dbk::PrefixType prefixType, const CDbOpLogs &dbOpLogs);

    void SetBaseView(CDelegateCache *pBaseIn) { voteRegIdCache = pBaseIn->voteRegIdCache; }
    // TODO:
    void Flush() {}

private:
/*  CDBScalarValueCache  prefixType     key                         value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // vote{(uint64t)MAX - $votedBcoins}{$RegId} -> 1
    CDBMultiValueCache<dbk::VOTE,       std::pair<string, string>,  uint8_t>                voteRegIdCache;
    CDBMultiValueCache<dbk::REGID_VOTE, string/* CRegID */,         vector<CCandidateVote>> regId2VoteCache;

    vector<CRegID> delegateRegIds;
};

#endif // PERSIST_DELEGATEDB_H