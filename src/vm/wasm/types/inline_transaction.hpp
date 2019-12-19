#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "commons/serialize.h"

using namespace std;

namespace wasm {

    struct signature_pair {
        uint64_t          account;
        vector<uint8_t> signature;

        IMPLEMENT_SERIALIZE (
            READWRITE(VARINT(account ));
            READWRITE(signature);
            )

    };

    struct permission {
        uint64_t account;
        uint64_t perm;
        
        IMPLEMENT_SERIALIZE (
        READWRITE(VARINT(account) );
        READWRITE(VARINT(perm));
        )

        bool operator == ( const permission& p )
        { 
            return account == p.account && perm == p.perm;
        }

    };

    struct inline_transaction {
        uint64_t contract;
        uint64_t action;
        std::vector <permission> authorization;
        std::vector<char> data;

        IMPLEMENT_SERIALIZE (
        READWRITE(VARINT(contract ));
        READWRITE(VARINT(action ));
        READWRITE(authorization);
        READWRITE(data);
        )

    };


}//wasm