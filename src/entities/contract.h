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

    bool IsEmpty() const {
        // FIXME:
        return vm_type == VMType::NULL_VM || code.empty();
    }

    void SetEmpty() {
        vm_type = VMType::NULL_VM;
        code.clear();
        abi.clear();
        memo.clear();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) vm_type);
        READWRITE(code);
        READWRITE(abi);
        READWRITE(memo);)

private:
    mutable uint256 sigHash;  //!< only in memory
};

// lua contract
class CLuaContract {
public:
    string code;  //!< Lua code
    string memo;  //!< Describe the binary code action
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
public:;
    bool IsValid();
    bool IsCheckAccount(void);
};

#endif //ENTITIES_CONTRACT_H