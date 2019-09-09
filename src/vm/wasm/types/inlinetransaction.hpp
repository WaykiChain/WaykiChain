#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
// #include <datastream.hpp>

#include "commons/serialize.h"

using namespace std;

namespace wasm {

    struct permission {
        uint64_t account;
        uint64_t perm;
    };

    struct CInlineTransaction {

// public:
// 	CInlineTransaction(){};
// 	~CInlineTransaction(){};

    public:
        uint64_t contract;
        uint64_t action;
        std::vector <uint64_t> authorization;
        std::vector<char> data;

        IMPLEMENT_SERIALIZE (
        READWRITE(contract );
        READWRITE(action);
        READWRITE(authorization);
        READWRITE(data);
        )


    };


}//wasm