// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CONTRACT_H
#define ENTITIES_CONTRACT_H

#include "commons/serialize.h"
#include "config/version.h"

#include <string>

using namespace std;

/**
 *  lua contract - for blockchain tx serialization/deserialization purpose
 *      - This is a backward compability implentation,
 *      - Only universal contract tx will be allowed after the software fork height
 */
class CLuaContract {
public:
    string code;  //!< Contract code
    string memo;  //!< Contract description

public:
    CLuaContract() { };
    CLuaContract(const string codeIn, const string memoIn): code(codeIn), memo(memoIn) { };

public:
    inline uint32_t GetContractSize() const { return GetContractSize(SER_DISK, CLIENT_VERSION); }

    inline uint32_t GetContractSize(int32_t nType, int32_t nVersion) const {
        uint32_t sz = ::GetSerializeSize(code, nType, nVersion);
        sz += ::GetSerializeSize(memo, nType, nVersion);
        return sz;
    }

    inline uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const {
        uint32_t sz = GetContractSize(nType, nVersion);
        return GetSizeOfCompactSize(sz) + sz;
    }

    template <typename Stream>
    void Serialize(Stream &s, int32_t nType, int32_t nVersion) const {
        uint32_t sz = GetContractSize(nType, nVersion);
        WriteCompactSize(s, sz);
        s << code << memo;
    }

    template <typename Stream>
    void Unserialize(Stream &s, int32_t nType, int32_t nVersion) {
        uint32_t sz = ReadCompactSize(s);
        s >> code >> memo;
        if (sz != GetContractSize(nType, nVersion)) {
            assert(false && "contractSize != SerializeSize(code) + SerializeSize(memo)");
        }
    }

    bool IsValid();
};

/** ###################################### Universal Contract ######################################*/
enum VMType : uint8_t {
    NULL_VM     = 0,
    LUA_VM      = 1,
    WASM_VM     = 2,
    EVM         = 3
};

/**
 * Used for both blockchain tx (new tx only) and levelDB Persistence (both old & new tx)
 *   serialization/deserialization purposes
 */
class CUniversalContract  {
public:
    VMType vm_type;
    bool upgradable;    //!< if true, the contract can be upgraded otherwise cannot anyhow.
    string code;        //!< Contract code
    string memo;        //!< Contract description
    string abi;         //!< ABI for contract invocation

public:
    CUniversalContract(): vm_type(NULL_VM) {}

    CUniversalContract(const string &codeIn, const string &memoIn)
        : vm_type(LUA_VM), upgradable(true), code(codeIn), memo(memoIn), abi("") {}

    CUniversalContract(const string &codeIn, const string &memoIn, const string &abiIn)
        : vm_type(LUA_VM), upgradable(true), code(codeIn), memo(memoIn), abi(abiIn) {}

    CUniversalContract(const bool upgradableIn, const string &codeIn, const string &memoIn, const string &abiIn)
        : vm_type(LUA_VM), upgradable(upgradableIn), code(codeIn), memo(memoIn), abi(abiIn) {}

    CUniversalContract(VMType vmTypeIn, bool upgradableIn, const string &codeIn, const string &memoIn,
                       const string &abiIn)
        : vm_type(vmTypeIn), upgradable(upgradableIn), code(codeIn), memo(memoIn), abi(abiIn) {}

public:
    inline uint32_t GetContractSize() const { return GetSerializeSize(SER_DISK, CLIENT_VERSION); }

    bool IsEmpty() const { return vm_type == VMType::NULL_VM && code.empty() && memo.empty() && abi.empty(); }

    void SetEmpty() {
        vm_type = VMType::NULL_VM;
        code.clear();
        memo.clear();
        abi.clear();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) vm_type);
        READWRITE(upgradable);
        READWRITE(code);
        READWRITE(memo);
        READWRITE(abi);
    )

    bool IsValid();
};

#endif  // ENTITIES_CONTRACT_H