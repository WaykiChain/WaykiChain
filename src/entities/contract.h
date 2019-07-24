// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CONTRACT_H
#define ENTITIES_CONTRACT_H

#include "id.h"

#include <string>

enum VMType : uint8_t {
    NULL_VM     = 0,
    LUA_VM      = 1,
    WASM_VM     = 2,
    EVM         = 3
};

class CContract {

public:
    VMType vm_type;
    string code;
    string abi;
    string memo;

public:
    CContract(): vm_type(NULL_VM) {}

    CContract(VMType vmTypeIn, string codeIn) :
        vm_type(vmTypeIn), code(codeIn), abi(""), memo("") { }; //for backward compatibility

    CContract(VMType vmTypeIn, string codeIn, string abiIn, string memoIn) :
        vm_type(vmTypeIn), code(codeIn), abi(abiIn), memo(memoIn) { };

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << (uint8_t&)vm_type << code << abi << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) vm_type);
        READWRITE(code);
        READWRITE(abi);
        READWRITE(memo);)

private:
    mutable uint256 sigHash;  //!< only in memory
};

#endif //ENTITIES_CONTRACT_H