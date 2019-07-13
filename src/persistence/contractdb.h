// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CONTRACTDB_H
#define PERSIST_CONTRACTDB_H

#include "accounts/account.h"
#include "accounts/key.h"
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
class CAccountLog;
class CContractDB;
struct CDiskTxPos;

class CContractDBCache {
public:
    CContractDBCache() {}

    CContractDBCache(CDBAccess *pDbAccess):
        scriptCache(pDbAccess),
        txOutputCache(pDbAccess),
        acctTxListCache(pDbAccess),
        txDiskPosCache(pDbAccess),
        contractRelatedKidCache(pDbAccess),
        contractDataCache(pDbAccess),
        contractAccountCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::CONTRACT);
    };

    CContractDBCache(CContractDBCache *pBaseIn):
        scriptCache(pBaseIn->scriptCache),
        txOutputCache(pBaseIn->txOutputCache),
        acctTxListCache(pBaseIn->acctTxListCache),
        txDiskPosCache(pBaseIn->txDiskPosCache),
        contractRelatedKidCache(pBaseIn->contractRelatedKidCache),
        contractDataCache(pBaseIn->contractDataCache),
        contractAccountCache(pBaseIn->contractAccountCache) {};

    void SetBaseView(CContractDBCache *pBaseIn) {
        scriptCache.SetBase(&pBaseIn->scriptCache);
        txOutputCache.SetBase(&pBaseIn->txOutputCache);
        acctTxListCache.SetBase(&pBaseIn->acctTxListCache);
        txDiskPosCache.SetBase(&pBaseIn->txDiskPosCache);
        contractRelatedKidCache.SetBase(&pBaseIn->contractRelatedKidCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
    }

    bool GetContractAccount(const CRegID &contractRegId, const string &accountKey, CAppUserAccount &appAccOut);
    bool SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn, CDBOpLogMap &dbOpLogMap);
    bool UndoContractAccount(CDBOpLogMap &dbOpLogMap);

    bool GetContractScript(const CRegID &contractRegId, string &contractScript);
    bool SetContractScript(const CRegID &contractRegId, const string &contractScript);
    bool HaveContractScript(const CRegID &contractRegId);
    bool EraseContractScript(const CRegID &contractRegId);

    bool GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData);
    bool SetContractData(const CRegID &contractRegId, const string &contractKey, const string &contractData,
                         CDBOpLogMap &dbOpLogMap);
    bool HaveContractData(const CRegID &contractRegId, const string &contractKey);
    bool EraseContractData(const CRegID &contractRegId, const string &contractKey, CDBOpLogMap &dbOpLogMap);
    bool UndoContractData(CDBOpLogMap &dbOpLogMap);

    bool SetTxRelAccout(const uint256 &txid, const set<CKeyID> &relAccount);
    bool GetTxRelAccount(const uint256 &txid, set<CKeyID> &relAccount);
    bool EraseTxRelAccout(const uint256 &txid);

    bool Flush();
//    bool Flush(IContractView *pView);
    unsigned int GetCacheSize();
    Object ToJsonObj() const;
//	IContractView * GetBaseScriptDB() { return pBase; }
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list, CDBOpLogMap &dbOpLogMap);

    string ToString();

    bool WriteTxOutput(const uint256 &txid, const vector<CVmOperate> &vOutput, CDBOpLogMap &dbOpLogMap);
    bool GetTxOutput(const uint256 &txid, vector<CVmOperate> &vOutput);
    bool UndoTxOutput(CDBOpLogMap &dbOpLogMap);

    bool GetTxHashByAddress(const CKeyID &keyId, uint32_t height, map<string, string > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, uint32_t height, uint32_t index, const uint256 &txid, CDBOpLogMap &dbOpLogMap);
    bool UndoTxHashByAddress(CDBOpLogMap &dbOpLogMap);
    bool GetAllContractAcc(const CRegID &contractRegId, map<string, string > &mapAcc);

private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // contractRegId -> script content
    CDBMultiValueCache< dbk::CONTRACT_DEF,         string,                   string >               scriptCache;
    // txId -> vector<CVmOperate>
    CDBMultiValueCache< dbk::CONTRACT_TX_OUT,      uint256,                  vector<CVmOperate> >   txOutputCache;
    // keyId, height, index -> txid
    CDBMultiValueCache< dbk::LIST_KEYID_TX,        tuple<CKeyID, uint32_t, uint32_t>,  uint256 >    acctTxListCache;
    // txId -> DiskTxPos
    CDBMultiValueCache< dbk::TXID_DISKINDEX,       uint256,                  CDiskTxPos >           txDiskPosCache;
    // contractTxId -> relatedAccounts
    CDBMultiValueCache< dbk::CONTRACT_RELATED_KID, uint256,                  set<CKeyID> >          contractRelatedKidCache;
    // pair<contractRegId, contractKey> -> scriptData
    CDBMultiValueCache< dbk::CONTRACT_DATA,        pair<string, string>,     string >               contractDataCache;
    // pair<contractRegId, accountKey> -> appUserAccount
    CDBMultiValueCache< dbk::CONTRACT_ACCOUNT,     pair<string, string>,     CAppUserAccount >      contractAccountCache;
};

#endif  // PERSIST_CONTRACTDB_H