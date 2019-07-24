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
class CAccount;
class CContractDB;
struct CDiskTxPos;

class CContractDBCache {
public:
    CContractDBCache() {}

    CContractDBCache(CDBAccess *pDbAccess):
        contractCache(pDbAccess),
        txOutputCache(pDbAccess),
        acctTxListCache(pDbAccess),
        txDiskPosCache(pDbAccess),
        contractRelatedKidCache(pDbAccess),
        contractDataCache(pDbAccess),
        contractAccountCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::CONTRACT);
    };

    CContractDBCache(CContractDBCache *pBaseIn):
        contractCache(pBaseIn->contractCache),
        txOutputCache(pBaseIn->txOutputCache),
        acctTxListCache(pBaseIn->acctTxListCache),
        txDiskPosCache(pBaseIn->txDiskPosCache),
        contractRelatedKidCache(pBaseIn->contractRelatedKidCache),
        contractDataCache(pBaseIn->contractDataCache),
        contractAccountCache(pBaseIn->contractAccountCache) {};

    bool GetContractAccount(const CRegID &contractRegId, const string &accountKey, CAppUserAccount &appAccOut);
    bool SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn, CDBOpLogMap &dbOpLogMap);
    bool UndoContractAccount(CDBOpLogMap &dbOpLogMap);

    bool GetContract(const CRegID &contractRegId, CContract &contract);
    bool SaveContract(const CRegID &contractRegId, const CContract &contract);
    bool HaveContractt(const CRegID &contractRegId);
    bool EraseContract(const CRegID &contractRegId);

    bool GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData);
    bool SetContractData(const CRegID &contractRegId, const string &contractKey, const string &contractData,
                         CDBOpLogMap &dbOpLogMap);
    bool HaveContractData(const CRegID &contractRegId, const string &contractKey);
    bool EraseContractData(const CRegID &contractRegId, const string &contractKey, CDBOpLogMap &dbOpLogMap);
    bool UndoContractData(CDBOpLogMap &dbOpLogMap);

    bool GetContractScripts(map<string, string> &contractScript);
    // Usage: acquire all data related to the specific contract.
    bool GetContractData(const CRegID &contractRegId, vector<std::pair<string, string>> &contractData);

    bool SetTxRelAccout(const uint256 &txid, const set<CKeyID> &relAccount);
    bool GetTxRelAccount(const uint256 &txid, set<CKeyID> &relAccount);
    bool EraseTxRelAccout(const uint256 &txid);

    bool Flush();
    uint32_t GetCacheSize() const;

    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list, CDBOpLogMap &dbOpLogMap);
    bool WriteTxOutput(const uint256 &txid, const vector<CVmOperate> &vOutput, CDBOpLogMap &dbOpLogMap);
    bool GetTxOutput(const uint256 &txid, vector<CVmOperate> &vOutput);
    bool UndoTxOutput(CDBOpLogMap &dbOpLogMap);

    bool GetTxHashByAddress(const CKeyID &keyId, uint32_t height, map<string, string > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, uint32_t height, uint32_t index, const uint256 &txid, CDBOpLogMap &dbOpLogMap);
    bool UndoTxHashByAddress(CDBOpLogMap &dbOpLogMap);
    bool GetContractAccounts(const CRegID &contractRegId, map<string, string > &mapAcc);

    void SetBaseViewPtr(CContractDBCache *pBaseIn) {
        contractCache.SetBase(&pBaseIn->contractCache);
        txOutputCache.SetBase(&pBaseIn->txOutputCache);
        acctTxListCache.SetBase(&pBaseIn->acctTxListCache);
        txDiskPosCache.SetBase(&pBaseIn->txDiskPosCache);
        contractRelatedKidCache.SetBase(&pBaseIn->contractRelatedKidCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
    };

private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // contractRegId -> Contract
    CCompositKVCache< dbk::CONTRACT_DEF,         CRegID,                   CContract >             contractCache;
    // txId -> vector<CVmOperate>
    CCompositKVCache< dbk::CONTRACT_TX_OUT,      uint256,                  vector<CVmOperate> >   txOutputCache;
    // keyId, height, index -> txid
    CCompositKVCache< dbk::LIST_KEYID_TX,        tuple<CKeyID, uint32_t, uint32_t>,  uint256 >    acctTxListCache;
    // txId -> DiskTxPos
    CCompositKVCache< dbk::TXID_DISKINDEX,       uint256,                  CDiskTxPos >           txDiskPosCache;
    // contractTxId -> relatedAccounts
    CCompositKVCache< dbk::CONTRACT_RELATED_KID, uint256,                  set<CKeyID> >          contractRelatedKidCache;
    // pair<contractRegId, contractKey> -> scriptData
    CCompositKVCache< dbk::CONTRACT_DATA,        pair<string, string>,     string >               contractDataCache;
    // pair<contractRegId, accountKey> -> appUserAccount
    CCompositKVCache< dbk::CONTRACT_ACCOUNT,     pair<string, string>,     CAppUserAccount >      contractAccountCache;
};

#endif  // PERSIST_CONTRACTDB_H