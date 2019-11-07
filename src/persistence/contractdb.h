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

/*  CCompositeKVCache     prefixType                       key                       value         variable           */
/*  -------------------- --------------------         ----------------------------  ---------   --------------------- */
    // pair<contractRegId, contractKey> -> contractData
typedef CCompositeKVCache< dbk::CONTRACT_DATA,        pair<string, CDBContractKey>, string>     DBContractDataCache;

// prefix: pair<contractRegId, contractKey>, support to match part of cotractKey
class CDBContractDatasGetter: public CDBListGetter<DBContractDataCache, pair<string, CDBContractKey>> {
public:
    typedef CDBListGetter<DBContractDataCache, pair<string, CDBContractKey>> ListGetter;
    using ListGetter::ListGetter;
public:
    const string& GetKey(const ListGetter::DataListItem &item) const {
        return item.first.second.GetKey();
    }

    const string& GetValue(const ListGetter::DataListItem &item) {
        return item.second;
    }
};

class CContractDBCache {
public:
    CContractDBCache() {}

    CContractDBCache(CDBAccess *pDbAccess):
        contractCache(pDbAccess),
        contractDataCache(pDbAccess),
        contractAccountCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::CONTRACT);
    };

    CContractDBCache(CContractDBCache *pBaseIn):
        contractCache(pBaseIn->contractCache),
        contractDataCache(pBaseIn->contractDataCache),
        contractAccountCache(pBaseIn->contractAccountCache) {};

    bool GetContractAccount(const CRegID &contractRegId, const string &accountKey, CAppUserAccount &appAccOut);
    bool SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn);

    bool GetContract(const CRegID &contractRegId, CUniversalContract &contract);
    bool GetContract(const CNickID &contractNickId, CCacheWrapper &cw, CUniversalContract &contract);
    bool GetContracts(map<string, CUniversalContract> &contracts);
    bool SaveContract(const CRegID &contractRegId, const CUniversalContract &contract);
    bool SaveContract(const CNickID &contractNickId, CCacheWrapper &cw, const CUniversalContract &contract);
    bool HaveContract(const CRegID &contractRegId);
    bool EraseContract(const CRegID &contractRegId);

    bool GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData);
    bool SetContractData(const CRegID &contractRegId, const string &contractKey, const string &contractData);
    bool HaveContractData(const CRegID &contractRegId, const string &contractKey);
    bool EraseContractData(const CRegID &contractRegId, const string &contractKey);

    bool GetContractData(const CNickID &contractNickId, CCacheWrapper& cw, const string &contractKey, string &contractData);
    bool SetContractData(const CNickID &contractNickId, CCacheWrapper& cw, const string &contractKey, const string &contractData);
    bool HaveContractData(const CNickID &contractNickId, CCacheWrapper& cw, const string &contractKey);
    bool EraseContractData(const CNickID &contractNickId,CCacheWrapper& cw, const string &contractKey);

    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CContractDBCache *pBaseIn) {
        contractCache.SetBase(&pBaseIn->contractCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        contractCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractDataCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractAccountCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoData() {
        return contractCache.UndoData() &&
               contractDataCache.UndoData() &&
               contractAccountCache.UndoData();
    }

    shared_ptr<CDBContractDatasGetter> CreateContractDatasGetter(const CRegID &contractRegid,
        const string &contractKeyPrefix, uint32_t count, const string &lastKey);


    shared_ptr<CDBContractDatasGetter> CreateContractDatasGetter(const CNickID &contractNickId,CCacheWrapper &cw,
            const string &contractKeyPrefix, uint32_t count, const string &lastKey);
private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // contract $RegId.ToRawString() -> Contract
    CCompositeKVCache< dbk::CONTRACT_DEF,         string,                   CUniversalContract >   contractCache;

    // pair<contractRegId, contractKey> -> contractData
    DBContractDataCache contractDataCache;
    // pair<contractRegId, accountKey> -> appUserAccount
    CCompositeKVCache< dbk::CONTRACT_ACCOUNT,     pair<string, string>,     CAppUserAccount >      contractAccountCache;
};

#endif  // PERSIST_CONTRACTDB_H