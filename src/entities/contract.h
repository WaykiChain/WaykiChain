// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CONTRACT_H
#define ENTITIES_CONTRACT_H

#include "id.h"

#include <string>

enum VMType : uint8_t {
    LUA_VM,
    WASM_VM,
    EVM,
    NULL_VM,
};

class CContract {

public:
    VMType vm_type;
    string code;
    string abi;

public:
    CContract(VM_Type vmTypeIn, string codeIn, string abiIn) : 
        vm_type(vmTypeIn), code(codeIn), abi(abiIn) { };

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << vm_type << code << abi;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) vm_type);
        READWRITE(code);
        READWRITE(abi);)

};

#endif //ENTITIES_CONTRACT_H