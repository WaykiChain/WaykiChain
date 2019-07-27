// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CONTRACT_H
#define ENTITIES_CONTRACT_H

#include "id.h"

#include <string>

// lua contract
class CLuaContract {
public:
    string code;  //!< Contract code in Lua script
    string memo;  //!< Contract description

public:
    inline unsigned int GetContractSize() const {
        return GetContractSize(SER_DISK, CLIENT_VERSION);
    }

    inline unsigned int GetContractSize(int nType, int nVersion) const {
        unsigned int sz = ::GetSerializeSize(code, nType, nVersion);
        sz += ::GetSerializeSize(memo, nType, nVersion);
        return sz;
    }

    inline unsigned int GetSerializeSize(int nType, int nVersion) const {
        unsigned int sz = GetContractSize(nType, nVersion);
        return GetSizeOfCompactSize(sz) + sz;
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        unsigned int sz = GetContractSize(nType, nVersion);
        WriteCompactSize(s, sz);
        s << code << memo;
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int sz = ReadCompactSize(s);
        s >> code >> memo;
        if (sz != GetContractSize(nType, nVersion)) {
            assert(false && "contractSize != SerializeSize(code) + SerializeSize(memo)");
        }
    }

public:
    bool IsValid();
    bool IsCheckAccount(void);

};

/** ###################################### Universal Contract ######################################*/
enum VMType : uint8_t {
    NULL_VM     = 0,
    LUA_VM      = 1,
    WASM_VM     = 2,
    EVM         = 3
};

class CUniversalContract {
public:
    VMType vm_type;
    string code;
    string memo;
    string abi;

public:
    CUniversalContract(): vm_type(NULL_VM) {}

    CUniversalContract(VMType vmTypeIn, string codeIn, string memoIn, string abiIn) :
        vm_type(vmTypeIn), code(codeIn), memo(memoIn), abi(abiIn) { };

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << (uint8_t&)vm_type << code << memo << abi;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    bool IsEmpty() const {
        // FIXME:
        return vm_type == VMType::NULL_VM || code.empty();
    }

    void SetEmpty() {
        vm_type = VMType::NULL_VM;
        code.clear();
        memo.clear();
        abi.clear();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) vm_type);
        READWRITE(code);
        READWRITE(memo);
        READWRITE(abi);
    )

private:
    mutable uint256 sigHash;  //!< only in memory
};

#endif //ENTITIES_CONTRACT_H