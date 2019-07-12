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
        contractItemCountCache(pDbAccess),
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
        contractItemCountCache(pBaseIn->contractItemCountCache),
        contractAccountCache(pBaseIn->contractAccountCache) {};

    bool GetScript(const CRegID &scriptId, string &value);
    bool GetScript(const int nIndex, CRegID &scriptId, string &value);

    bool GetScriptAcc(const CRegID &scriptId, const string &key, CAppUserAccount &appAccOut);
    bool SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccIn, CDBOpLogMap &dbOpLogMap);
    bool UndoScriptAcc(CDBOpLogMap &dbOpLogMap);

    bool SetScript(const CRegID &scriptId, const string &value);
    bool HaveScript(const CRegID &scriptId);
    bool EraseScript(const CRegID &scriptId);
    bool GetContractItemCount(const CRegID &scriptId, int &nCount);
    bool HaveScriptData(const CRegID &scriptId, const string &contractKey);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const string &contractKey,
                         string &vScriptData);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex,
                         string &contractKey, string &vScriptData);
    bool SetContractData(const CRegID &scriptId, const string &contractKey,
                         const string &vScriptData, CDBOpLogMap &dbOpLogMap);
    bool EraseContractData(const CRegID &scriptId, const string &contractKey, CDBOpLogMap &dbOpLogMap);
    bool UndoContractData(CDBOpLogMap &dbOpLogMap);

    /**
     * @brief Get all number of scripts in scriptdb
     * @param nCount
     * @return true if get succeed, otherwise false
     */
    bool GetScriptCount(int &nCount);
    bool SetTxRelAccout(const uint256 &txHash, const set<CKeyID> &relAccount);
    bool GetTxRelAccount(const uint256 &txHash, set<CKeyID> &relAccount);
    bool EraseTxRelAccout(const uint256 &txHash);
    /**
     * @brief write all data in the caches to script db
     * @return
     */
    bool Flush();
//    bool Flush(IContractView *pView);
    unsigned int GetCacheSize();
    Object ToJsonObj() const;
//	IContractView * GetBaseScriptDB() { return pBase; }
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list, CDBOpLogMap &dbOpLogMap);

    void SetBaseView(CContractDBCache *pBaseIn) {
        scriptCache.SetBase(&pBaseIn->scriptCache);
        txOutputCache.SetBase(&pBaseIn->txOutputCache);
        acctTxListCache.SetBase(&pBaseIn->acctTxListCache);
        txDiskPosCache.SetBase(&pBaseIn->txDiskPosCache);
        contractRelatedKidCache.SetBase(&pBaseIn->contractRelatedKidCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractItemCountCache.SetBase(&pBaseIn->contractItemCountCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
     }

    string ToString();

    bool WriteTxOutput(const uint256 &txid, const vector<CVmOperate> &vOutput, CDBOpLogMap &dbOpLogMap);
    bool GetTxOutput(const uint256 &txid, vector<CVmOperate> &vOutput);
    bool UndoTxOutput(CDBOpLogMap &dbOpLogMap);

    bool GetTxHashByAddress(const CKeyID &keyId, uint32_t height, map<string, string > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, uint32_t height, uint32_t index, const uint256 &txid, CDBOpLogMap &dbOpLogMap);
    bool UndoTxHashByAddress(CDBOpLogMap &dbOpLogMap);
    bool GetAllContractAcc(const CRegID &scriptId, map<string, string > &mapAcc);

private:
    bool GetData(const string &key, string &value);
    bool SetData(const string &key, const string &value);
//    bool BatchWrite(const map<string, string > &mapContractDb);
//    bool EraseKey(const string &key);
//    bool HaveData(const string &key);

    /**
     * @brief Get script content from scriptdb by scriptid
     * @param vScriptId
     * @param vValue
     * @return true if get script succeed,otherwise false
     */
    bool GetScript(const string &contractRegId, string &value);
    /**
     * @brief Get Script content from scriptdb by index
     * @param nIndex the value must be non-negative
     * @param vScriptId
     * @param vValue
     * @return true if get script succeed, otherwise false
     */
    bool GetScript(const int nIndex, string &contractRegId, string &value);
    /**
     * @brief Save script content to scriptdb
     * @param vScriptId
     * @param vValue
     * @return true if save succeed, otherwise false
     */
    bool SetScript(const string &contractRegId, const string &value);
    /**
     * @brief Detect if scriptdb contains the script by scriptid
     * @param vScriptId
     * @return true if contains script, otherwise false
     */
    bool HaveScript(const string &vScriptId);
    /**
     * @brief Save all number of scripts in scriptdb
     * @param nCount
     * @return true if save count succeed, otherwise false
     */
    bool SetScriptCount(const int nCount);
    /**
     * @brief Delete script from script db by scriptId
     * @param vScriptId
     * @return true if delete succeed, otherwise false
     */
    bool EraseScript(const string &vScriptId);
    /**
     * @brief Get total number of contract data elements in contract db
     * @param vScriptId
     * @param nCount
     * @return true if get succeed, otherwise false
     */
    bool GetContractItemCount(const string &contractRegId, int &nCount);
    /**
     * @brief Save count of the Contract's data into contract db
     * @param vScriptId
     * @param nCount
     * @return true if save succeed, otherwise false
     */
    bool SetContractItemCount(const string &contractRegId, int nCount);
    bool IncContractItemCount(const string &contractRegId, int count);
    /**
     * @brief Delete the item of the script's data by scriptId and scriptKey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if delete succeed, otherwise false
     */
    bool EraseContractData(const string &contractRegId, const string &contractKey, CDBOpLogMap &dbOpLogMap);

    bool EraseContractData(const string &key);
    /**
     * @brief Detect if scriptdb contains the item of script's data by scriptid and scriptkey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if contains the item, otherwise false
     */
    bool HaveScriptData(const string &contractRegId, const string &contractKey);
    /**
     * @brief Get smart contract App data and valid height by scriptid and scriptkey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @param vScriptData
     * @param nHeight valide height of script data
     * @return true if get succeed, otherwise false
     */
    bool GetContractData(const int nCurBlockHeight, const string &contractRegId, const string &contractKey,
                         string &vScriptData);
    /**
     * @brief Get smart contract app data and valid height by scriptid and nIndex
     * @param vScriptId
     * @param nIndex get first script data will be 0, otherwise be 1
     * @param vScriptKey must be 8 bytes, get first script data will be empty, otherwise get next scirpt data will be previous script key
     * @param vScriptData
     * @param nHeight valid height of script data
     * @return true if get succeed, otherwise false
     */
    bool GetContractData(const int nCurBlockHeight, const string &contractRegId, const int &nIndex, string &contractKey,
                         string &vScriptData);
    /**
     * @brief Save script data and valid height into script db
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @param vScriptData
     * @param nHeight valide height of script data
     * @return true if save succeed, otherwise false
     */
    bool SetContractData(const string &contractRegId, const string &contractKey,
                         const string &vScriptData, CDBOpLogMap &dbOpLogMap);
private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // scriptRegId -> script content
    CDBMultiValueCache< dbk::CONTRACT_DEF,         string,                   string >               scriptCache;
    // txId -> vector<CVmOperate>
    CDBMultiValueCache< dbk::CONTRACT_TX_OUT,      uint256,                  vector<CVmOperate> >   txOutputCache;
    // keyId,height,index -> txid
    CDBMultiValueCache< dbk::LIST_KEYID_TX,        tuple<CKeyID, uint32_t, uint32_t>,  uint256 >    acctTxListCache;
    // txId -> DiskTxPos
    CDBMultiValueCache< dbk::TXID_DISKINDEX,       uint256,                  CDiskTxPos >           txDiskPosCache;
    // contractTxId -> relatedAccounts
    CDBMultiValueCache< dbk::CONTRACT_RELATED_KID, uint256,                  set<CKeyID> >          contractRelatedKidCache;
    // pair<scriptId, scriptKey> -> scriptData
    CDBMultiValueCache< dbk::CONTRACT_DATA,        pair<string, string>,     string >               contractDataCache;
    // scriptId -> contractItemCount
    CDBMultiValueCache< dbk::CONTRACT_ITEM_NUM,    string,                   CDBCountValue >        contractItemCountCache;
    // scriptId -> contractItemCount
    CDBMultiValueCache< dbk::CONTRACT_ACCOUNT,     pair<string, string>,     CAppUserAccount >      contractAccountCache;
};

#endif  // PERSIST_CONTRACTDB_H