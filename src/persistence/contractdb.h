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
struct CDiskTxPos;

class CContractDBCache {
public:
    CContractDBCache() {}

    CContractDBCache(CDBAccess *pDbAccess):
        contractCache(pDbAccess),
        txOutputCache(pDbAccess),
        txDiskPosCache(pDbAccess),
        contractRelatedKidCache(pDbAccess),
        contractDataCache(pDbAccess),
        contractAccountCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::CONTRACT);
    };

    CContractDBCache(CContractDBCache *pBaseIn):
        contractCache(pBaseIn->contractCache),
        txOutputCache(pBaseIn->txOutputCache),
        txDiskPosCache(pBaseIn->txDiskPosCache),
        contractRelatedKidCache(pBaseIn->contractRelatedKidCache),
        contractDataCache(pBaseIn->contractDataCache),
        contractAccountCache(pBaseIn->contractAccountCache) {};

    bool GetContractAccount(const CRegID &contractRegId, const string &accountKey, CAppUserAccount &appAccOut);
    bool SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn);

    bool GetContract(const CRegID &contractRegId, CUniversalContract &contract);
    bool GetContracts(map<string, CUniversalContract> &contracts);
    bool SaveContract(const CRegID &contractRegId, const CUniversalContract &contract);
    bool HaveContract(const CRegID &contractRegId);
    bool EraseContract(const CRegID &contractRegId);

    bool GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData);
    bool SetContractData(const CRegID &contractRegId, const string &contractKey, const string &contractData);
    bool HaveContractData(const CRegID &contractRegId, const string &contractKey);
    bool EraseContractData(const CRegID &contractRegId, const string &contractKey);

    // Usage: acquire all data related to the specific contract.
    bool GetContractData(const CRegID &contractRegId, vector<std::pair<string, string>> &contractData);

    bool SetTxRelAccout(const uint256 &txid, const set<CKeyID> &relAccount);
    bool GetTxRelAccount(const uint256 &txid, set<CKeyID> &relAccount);
    bool EraseTxRelAccout(const uint256 &txid);

    bool Flush();
    uint32_t GetCacheSize() const;

    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool SetTxIndex(const uint256 &txid, const CDiskTxPos &pos);
    bool WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list);
    bool WriteTxOutput(const uint256 &txid, const vector<CVmOperate> &vOutput);
    bool GetTxOutput(const uint256 &txid, vector<CVmOperate> &vOutput);

    bool GetTxHashByAddress(const CKeyID &keyId, uint32_t height, map<string, string > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, uint32_t height, uint32_t index, const uint256 &txid);
    bool GetContractAccounts(const CRegID &contractRegId, map<string, string > &mapAcc);

    void SetBaseViewPtr(CContractDBCache *pBaseIn) {
        contractCache.SetBase(&pBaseIn->contractCache);
        txOutputCache.SetBase(&pBaseIn->txOutputCache);
        txDiskPosCache.SetBase(&pBaseIn->txDiskPosCache);
        contractRelatedKidCache.SetBase(&pBaseIn->contractRelatedKidCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        contractCache.SetDbOpLogMap(pDbOpLogMapIn);
        txOutputCache.SetDbOpLogMap(pDbOpLogMapIn);
        txDiskPosCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractRelatedKidCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractDataCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractAccountCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return contractCache.UndoDatas() &&
               txOutputCache.UndoDatas() &&
               txDiskPosCache.UndoDatas() &&
               contractRelatedKidCache.UndoDatas() &&
               contractDataCache.UndoDatas() &&
               contractAccountCache.UndoDatas();
    }
private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // contract $RegId.ToRawString() -> Contract
    CCompositeKVCache< dbk::CONTRACT_DEF,         string,                   CUniversalContract >   contractCache;
    // txId -> vector<CVmOperate>
    CCompositeKVCache< dbk::CONTRACT_TX_OUT,      uint256,                  vector<CVmOperate> >   txOutputCache;
    // txId -> DiskTxPos
    CCompositeKVCache< dbk::TXID_DISKINDEX,       uint256,                  CDiskTxPos >           txDiskPosCache;
    // contractTxId -> set<CKeyID>
    CCompositeKVCache< dbk::CONTRACT_RELATED_KID, uint256,                  set<CKeyID> >          contractRelatedKidCache;
    // pair<contractRegId, contractKey> -> scriptData
    CCompositeKVCache< dbk::CONTRACT_DATA,        pair<string, string>,     string >               contractDataCache;
    // pair<contractRegId, accountKey> -> appUserAccount
    CCompositeKVCache< dbk::CONTRACT_ACCOUNT,     pair<string, string>,     CAppUserAccount >      contractAccountCache;
};

#endif  // PERSIST_CONTRACTDB_H