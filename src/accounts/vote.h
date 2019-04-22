// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef VOTE_H
#define VOTE_H

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "chainparams.h"
#include "crypto/hash.h"

/**
 * Usage:   vote fund types
 */
enum FundType: unsigned char {
    FREEDOM = 1,    //!< FREEDOM
    REWARD_FUND,    //!< REWARD_FUND
    NULL_FUNDTYPE,  //!< NULL_FUNDTYPE
};

enum OperType: unsigned char {
    ADD_FREE   = 1,  //!< add money to freedom
    MINUS_FREE = 2,  //!< minus money from freedom
    NULL_OPERTYPE,   //!< invalid operate type
};

enum VoteOperType: unsigned char {
    ADD_FUND   = 1,  //!< add operate
    MINUS_FUND = 2,  //!< minus operate
    NULL_OPER,       //!< invalid
};

class CVoteFund {
private:
    CID voteId;             //!< candidate RegId or PubKey
    uint64_t voteCount;     //!< count of votes to the candidate

    uint256 sigHash;        //!< only in memory

public:
    CID GetVoteId() { return voteId; }
    uint64_t GetVoteCount() { return voteCount; }

    void SetVoteCount(uint64_t votes) { voteCount = votes; }
    void SetVoteId(CID voteIdIn) { voteId = voteIdIn; }

public:
    CVoteFund() {
        voteId = CID();
        voteCount  = 0;
    }
    CVoteFund(uint64_t voteCountIn) {
        voteId = CID();
        voteCount  = voteCountIn;
    }
    CVoteFund(CID voteIdIn, uint64_t voteCountIn) {
        voteId = voteIdIn;
        voteCount  = voteCountIn;
    }
    CVoteFund(const CVoteFund &fund) {
        voteId = fund.voteId;
        voteCount  = fund.voteCount;
    }
    CVoteFund &operator=(const CVoteFund &fund) {
        if (this == &fund)
            return *this;

        this->voteId = fund.voteId;
        this->voteCount  = fund.voteCount;
        return *this;
    }
    ~CVoteFund() {}

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(voteCount) << voteId;
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();  // Truly need to write the sigHash.
        }

        return sigHash;
    }

    friend bool operator<(const CVoteFund &fa, const CVoteFund &fb) {
        return (fa.voteCount <= fb.voteCount);
    }
    friend bool operator>(const CVoteFund &fa, const CVoteFund &fb) {
        return !operator<(fa, fb);
    }
    friend bool operator==(const CVoteFund &fa, const CVoteFund &fb) {
        return (fa.voteId == fb.voteId && fa.voteCount == fb.voteCount);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(voteId);
        READWRITE(VARINT(voteCount));
    )

    string ToString() const {
        string str("");
        string idType = voteId.GetIDName();
        str = idType + voteId.ToString();

        str += " votes:";
        str += strprintf("%s", voteCount);
        str += "\n";
        return str;
    }

    Object ToJson() const {
        Object obj;
        obj.push_back(Pair("voteId", this->voteId.ToJson()));
        obj.push_back(Pair("votes", voteCount));
        return obj;
    }
};


class COperVoteFund {
public:
    static const string voteOperTypeArray[3];

public:
    unsigned char operType;  //!<1:ADD_FUND 2:MINUS_FUND
    CVoteFund fund;

    IMPLEMENT_SERIALIZE(
        READWRITE(operType);
        READWRITE(fund);)
public:
    COperVoteFund() {
        operType = NULL_OPER;
    }
    COperVoteFund(unsigned char nType, const CVoteFund &operFund) {
        operType = nType;
        fund     = operFund;
    }

    string ToString() const { 
        return strprintf("operVoteType=%s %s", voteOperTypeArray[operType], fund.ToString()); 
    }

    Object ToJson() const {
        Object obj;
        string sOperType;
        if (operType >= 3) {
            sOperType = "INVALID_OPER_TYPE";
            LogPrint("ERROR", "Delegate Vote Tx contains invalid operType: %d", operType);
        } else {
            sOperType = voteOperTypeArray[operType];
        }
        obj.push_back(Pair("operType", sOperType));
        obj.push_back(Pair("voteFund", fund.ToJson()));
        return obj;
    }
};

#endif