// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_PROPOSAL_H
#define ENTITIES_PROPOSAL_H

#include <string>
#include <vector>
#include <unordered_map>
#include "commons/serialize.h"

class CCacheWrapper ;

class CProposal {
public:

    int8_t needGovernerCount = 0;
    int32_t expire_block_height = 0;
    vector<std::pair<string, uint64_t>> paramValues;
    mutable uint256 sigHash;  //!< only in memory

public:
    CProposal() {};
    
    CProposal(uint64_t expireBlockHeightIn, vector<std::pair<string, uint64_t>> &paramValuesIn):
         expire_block_height(expireBlockHeightIn), paramValues(paramValuesIn) {};

    ~CProposal() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expire_block_height));
        READWRITE(paramValues);
        READWRITE(needGovernerCount);
    );

    bool IsEmpty() const { return paramValues.size()== 0 && expire_block_height == 0 && needGovernerCount == 0 ; };
    void SetEmpty() {
        expire_block_height = 0 ;
        needGovernerCount = 0 ;
        paramValues.clear();
    }

    bool ExecuteProposal(CCacheWrapper &cw);
    // json_spirit::Object ToJson() const {
    //     json_spirit::Object obj;
    //     return obj;
    // }

    // string ToString() const {
    //     string str = strprintf("voteType: %s, candidateUid: %s, votes: %lld\n", GetVoteType(voteType),
    //                            candidateUid.ToString(), votedBcoins);
    //     return str;
    // }

};

#endif //ENTITIES_PROPOSAL_H