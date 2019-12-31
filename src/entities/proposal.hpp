// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_PROPOSAL_H
#define ENTITIES_PROPOSAL_H

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

class CProposal {
private:
    uint64_t expire_block_height;
    list<string, uint64_t> paramValues;
    mutable uint256 sigHash;  //!< only in memory

public:
    CProposal() {};
    
    CProposal(uint64_t expireBlockHeightIn, list<string, uint64_t> &paramValuesIn):
         expire_block_height(expireBlockHeightIn), paramValues(paramValuesIn) {};

    ~CProposal() {}

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);

            ss << VARINT(expire_block_height) << paramValues;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expire_block_height));
        READWRITE(paramValues);
    );


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