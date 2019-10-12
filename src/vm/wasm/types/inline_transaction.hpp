#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "commons/serialize.h"

using namespace std;

namespace wasm {

    struct permission {
        uint64_t account;
        uint64_t perm;
        
        IMPLEMENT_SERIALIZE (
        READWRITE(VARINT(account) );
        READWRITE(VARINT(perm));
        )

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