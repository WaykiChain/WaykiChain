#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "commons/serialize.h"
#include "wasm/wasm_serialize_reflect.hpp"

using namespace std;

namespace wasm {

    struct signature_pair {
        uint64_t          account;
        vector<uint8_t>   signature;

        IMPLEMENT_SERIALIZE (
            READWRITE(VARINT(account));
            READWRITE(signature);
            )

        WASM_REFLECT( signature_pair, (account)(signature) )

    };

    struct permission {
        uint64_t account;
        uint64_t perm;

        IMPLEMENT_SERIALIZE (
            READWRITE(VARINT(account));
            READWRITE(VARINT(perm));
        )

        bool operator == ( const permission& p )
        {
            return account == p.account && perm == p.perm;
        }

        WASM_REFLECT( permission, (account)(perm) )

    };

    struct inline_transaction {
        uint64_t                 contract;
        uint64_t                 action;
        std::vector <permission> authorization;
        std::vector <char>       data;

        IMPLEMENT_SERIALIZE (
            READWRITE(VARINT(contract));
            READWRITE(VARINT(action));
            READWRITE(authorization);
            READWRITE(data);
        )

        WASM_REFLECT( inline_transaction, (contract)(action)(authorization)(data) )

    };


}//wasm