// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CONTRACTDB_H
#define PERSIST_CONTRACTDB_H

#include "entities/account.h"
#include "entities/contract.h"
#include "entities/key.h"
#include "commons/arith_uint256.h"
#include "commons/uint256.h"
#include "dbaccess.h"
#include "dbiterator.h"
#include "persistence/disk.h"
#include "vm/luavm/appaccount.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CVmOperate;
class CKeyID;
class CRegID;
class CAccount;
class CContractDB;
class CCacheWrapper;

struct CDiskTxPos;

typedef dbk::CDBTailKey<MAX_CONTRACT_KEY_SIZE> CDBContractKey;

/* LuaVM/WASM, maintainer regid, contract */
typedef tuple<uint8_t, CRegIDKey, CUniversalContract> UniversalContractStore;

/*  CCompositeKVCache     prefixType                       key                       value         variable           */
/*  -------------------- --------------------         ----------------------------  ---------   --------------------- */
    // pair<contractRegId, contractKey> -> contractData
typedef CCompositeKVCache< dbk::CONTRACT_DATA,        pair<CRegIDKey, CDBContractKey>, string>     DBContractDataCache;

class CDBContractDataIterator: public CDBPrefixIterator<DBContractDataCache, DBContractDataCache::KeyType> {
private:
    typedef typename DBContractDataCache::KeyType KeyType;
    typedef CDBPrefixIterator<DBContractDataCache, DBContractDataCache::KeyType> Base;
public:
    CDBContractDataIterator(DBContractDataCache &dbCache, const CRegID &regidIn,
                            const string &contractKeyPrefix)
        : Base(dbCache, KeyType(CRegIDKey(regidIn), contractKeyPrefix)) {}

    bool SeekUpper(const string *pLastContractKey) {
        if (pLastContractKey == nullptr || db_util::IsEmpty(*pLastContractKey))
            return First();
        if (pLastContractKey->size() > CDBContractKey::MAX_KEY_SIZE)
            return false;
        KeyType lastKey(GetPrefixElement().first, *pLastContractKey);
        return sp_it_Impl->SeekUpper(&lastKey);
    }

    const string& GetContractKey() const {
        return GetKey().second.GetKey();
    }
};

class CContractDBCache {
public:
    CContractDBCache() {}

    CContractDBCache(CDBAccess *pDbAccess):
        contractCache(pDbAccess),
        contractDataCache(pDbAccess),
        contractAccountCache(pDbAccess),
        contractTracesCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::CONTRACT);
    };

    CContractDBCache(CContractDBCache *pBaseIn):
        contractCache(pBaseIn->contractCache),
        contractDataCache(pBaseIn->contractDataCache),
        contractAccountCache(pBaseIn->contractAccountCache),
        contractTracesCache(pBaseIn->contractTracesCache) {};

    bool GetContractAccount(const CRegID &contractRegId, const string &accountKey, CAppUserAccount &appAccOut);
    bool SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn);

    bool GetContract(const CRegID &contractRegId, UniversalContractStore &contract);
    bool SaveContract(const CRegID &contractRegId, const UniversalContractStore &contract);
    bool HasContract(const CRegID &contractRegId);
    bool EraseContract(const CRegID &contractRegId);

    bool GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData);
    bool SetContractData(const CRegID &contractRegId, const string &contractKey, const string &contractData);
    bool HasContractData(const CRegID &contractRegId, const string &contractKey);
    bool EraseContractData(const CRegID &contractRegId, const string &contractKey);

    bool GetContractTraces(const uint256 &txid, string &contractTraces);
    bool SetContractTraces(const uint256 &txid, const string &contractTraces);

    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CContractDBCache *pBaseIn) {
        contractCache.SetBase(&pBaseIn->contractCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
        contractTracesCache.SetBase(&pBaseIn->contractTracesCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        contractCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractDataCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractAccountCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractTracesCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        contractCache.RegisterUndoFunc(undoDataFuncMap);
        contractDataCache.RegisterUndoFunc(undoDataFuncMap);
        contractAccountCache.RegisterUndoFunc(undoDataFuncMap);
        contractTracesCache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CDBContractDataIterator> CreateContractDataIterator(const CRegID &contractRegid,
        const string &contractKeyPrefix);

public:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // contract $RegIdKey -> UniversalContractStore
    CCompositeKVCache< dbk::CONTRACT_DEF,         CRegIDKey,                  UniversalContractStore >   contractCache;

    // pair<contractRegId, contractKey> -> contractData
    DBContractDataCache contractDataCache;

    // pair<contractRegId, accountKey> -> appUserAccount
    CCompositeKVCache< dbk::CONTRACT_ACCOUNT,     pair<CRegIDKey, string>,     CAppUserAccount >           contractAccountCache;

    // txid -> contract_traces
    CCompositeKVCache< dbk::CONTRACT_TRACES,     uint256,                      string >                    contractTracesCache;
};

#endif  // PERSIST_CONTRACTDB_H