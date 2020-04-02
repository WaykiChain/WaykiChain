// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_VOTE_H
#define ENTITIES_VOTE_H

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "key.h"
#include "crypto/hash.h"
#include "entities/id.h"

enum VoteType : uint8_t {
    NULL_VOTE   = 0,  //!< invalid vote operate
    ADD_BCOIN   = 1,  //!< add operate
    MINUS_BCOIN = 2,  //!< minus operate
};

static const unordered_map<uint8_t, string> kVoteTypeMap = {
    { NULL_VOTE,    "NULL_VOTE"     },
    { ADD_BCOIN,    "ADD_BCOIN"     },
    { MINUS_BCOIN,  "MINUS_BCOIN"   },
};

class CCandidateVote {
private:
    uint8_t voteType;        //!< 1:ADD_BCOIN 2:MINUS_BCOIN
    CUserID candidateUid;    //!< candidate RegId or PubKey
    uint64_t votedBcoins;    //!< count of votes to the candidate

    mutable uint256 sigHash;  //!< only in memory

public:
    CCandidateVote() {
        voteType     = NULL_VOTE;
        candidateUid = CUserID();
        votedBcoins  = 0;
    }
    CCandidateVote(VoteType voteTypeIn, CUserID voteUserIdIn, uint64_t votedBcoinsIn) {
        voteType     = voteTypeIn;
        candidateUid = voteUserIdIn;
        votedBcoins  = votedBcoinsIn;
    }
    CCandidateVote(const CCandidateVote &voteIn) {
        voteType     = voteIn.voteType;
        candidateUid = voteIn.candidateUid;
        votedBcoins  = voteIn.votedBcoins;
    }
    CCandidateVote &operator=(const CCandidateVote &voteIn) {
        if (this == &voteIn)
            return *this;

        this->voteType     = voteIn.voteType;
        this->candidateUid = voteIn.candidateUid;
        this->votedBcoins  = voteIn.votedBcoins;
        return *this;
    }
    ~CCandidateVote() {}

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << voteType << candidateUid << VARINT(votedBcoins);
    }

    friend bool operator<(const CCandidateVote &fa, const CCandidateVote &fb) {
        return (fa.votedBcoins <= fb.votedBcoins);
    }
    friend bool operator>(const CCandidateVote &fa, const CCandidateVote &fb) {
        return !operator<(fa, fb);
    }
    friend bool operator==(const CCandidateVote &fa, const CCandidateVote &fb) {
        return (fa.candidateUid == fb.candidateUid && fa.votedBcoins == fb.votedBcoins);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(voteType);
        READWRITE(candidateUid);
        READWRITE(VARINT(votedBcoins));
    );

    const CUserID &GetCandidateUid() const { return candidateUid; }
    unsigned char GetCandidateVoteType() const { return voteType; }
    uint64_t GetVotedBcoins() const { return votedBcoins; }
    void SetVotedBcoins(uint64_t votedBcoinsIn) { votedBcoins = votedBcoinsIn; }
    void SetCandidateUid(const CUserID &votedUserIdIn) { candidateUid = votedUserIdIn; }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(json_spirit::Pair("vote_type", GetVoteType(voteType)));
        obj.push_back(json_spirit::Pair("candidate_uid", candidateUid.ToJson()));
        obj.push_back(json_spirit::Pair("voted_bcoins", ValueFromAmount(votedBcoins)));

        return obj;
    }

    string ToString() const {
        return strprintf("voteType: %s, candidateUid: %s, votes: %f\n", GetVoteType(voteType),
                               candidateUid.ToString(), ValueFromAmount(votedBcoins));
    }

private:
    static string GetVoteType(const unsigned char voteTypeIn) {
        auto it = kVoteTypeMap.find(voteTypeIn);
        if (it != kVoteTypeMap.end())
            return it->second;
        else
            return "";
    }
};


class CCandidateReceivedVote {
public:
    CCandidateReceivedVote() {};

    CCandidateReceivedVote(const CCandidateVote &vote):
        candidate_uid(vote.GetCandidateUid()),
        voted_bcoins(vote.GetVotedBcoins()) { };

public:
    friend bool operator<(const CCandidateReceivedVote &fa, const CCandidateReceivedVote &fb) {
        return (fa.voted_bcoins <= fb.voted_bcoins);
    }
    friend bool operator>(const CCandidateReceivedVote &fa, const CCandidateReceivedVote &fb) {
        return !operator<(fa, fb);
    }
    friend bool operator==(const CCandidateReceivedVote &fa, const CCandidateReceivedVote &fb) {
        return (fa.candidate_uid == fb.candidate_uid && fa.voted_bcoins == fb.voted_bcoins);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(candidate_uid);
        READWRITE(VARINT(voted_bcoins));
    );

     json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(json_spirit::Pair("candidate_uid", candidate_uid.ToJson()));
        obj.push_back(json_spirit::Pair("voted_bcoins", voted_bcoins));

        return obj;
    }

    string ToString() const {
        string str = strprintf("candidate_uid: %s, voted_bcoins: %lld \n", candidate_uid.ToString(), voted_bcoins);
        return str;
    }

    const CUserID &GetCandidateUid() const { return candidate_uid; }
    uint64_t GetVotedBcoins() const { return voted_bcoins; }
    void SetVotedBcoins(uint64_t votedBcoinsIn) { voted_bcoins = votedBcoinsIn; }

private:
    CUserID candidate_uid;    //!< candidate RegId or PubKey
    uint64_t voted_bcoins;    //!< count of votes to the candidate

};

typedef vector<CRegID> DelegateVector;

struct VoteDelegate {
    CRegID regid;       // the voted delegate regid
    uint64_t votes = 0;     // the received votes

    VoteDelegate() {}
    VoteDelegate(const CRegID &regidIn, uint64_t votesIn) : regid(regidIn), votes(votesIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(regid);
        READWRITE(VARINT(votes));
    )

    friend bool operator==(const VoteDelegate &a, const VoteDelegate &b) {
        return (a.regid == b.regid && a.votes == b.votes);
    }

    string ToString() const {
        return strprintf("regid=%s", regid.ToString()) + ", " +
                strprintf("votes=%llu", votes);
    }
};

enum class VoteDelegateState: uint8_t {
    NONE,           // none, init state
    PENDING,        // pending, wait for activating vote delegates
    ACTIVATED,      // activated, vote delegates is activated
};

typedef vector<VoteDelegate> VoteDelegateVector;
struct PendingDelegates {
    VoteDelegateState state = VoteDelegateState::NONE;  // state
    uint32_t counted_vote_height = 0;                   // counting vote height
    VoteDelegateVector top_vote_delegates;              // top vote delegates

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)state);
        READWRITE(VARINT(counted_vote_height));
        READWRITE(top_vote_delegates);
    )

    bool IsSameDelegates(const DelegateVector &delegates) const {
        if (top_vote_delegates.size() != delegates.size())
            return false;

        for (size_t i = 0; i < top_vote_delegates.size(); i++)
            if (top_vote_delegates[i].regid != delegates[i])
                return false;

        return true;
    }

    DelegateVector GetDelegates() const {
        DelegateVector delegates;
        delegates.resize(top_vote_delegates.size());
        for (size_t i = 0; i < top_vote_delegates.size(); i++) {
            delegates[i] = top_vote_delegates[i].regid;
        }
        return delegates;
    }

    bool IsEmpty() const {
        return state == VoteDelegateState::NONE && counted_vote_height == 0 && top_vote_delegates.empty();
    }

    void SetEmpty() {
        state = VoteDelegateState::NONE;
        counted_vote_height = 0;
        top_vote_delegates.clear();
    }

    string ToString() const {
        string delegatesStr;
        for (const auto &item : top_vote_delegates)
            delegatesStr += "{" + item.ToString() + "},\n";
        return strprintf("state=%d", (int)state/*TODO:... */) + "," +
                strprintf("counted_vote_height=%d", counted_vote_height) + ", " +
                strprintf("delegate_num=%d", top_vote_delegates.size()) + ", " +
                strprintf("top_vote_delegates=[%s]", delegatesStr);
    }
};

#endif //ENTITIES_VOTE_H