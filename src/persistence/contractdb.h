// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CONTRACTDB_H
#define PERSIST_CONTRACTDB_H

#include "commons/arith_uint256.h"
#include "dbconf.h"
#include "persistence/leveldbwrapper.h"
#include "vm/appaccount.h"

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

class IContractView {
public:
    virtual bool GetData(const string &key, string &value) = 0;
    virtual bool SetData(const string &key, const string &value) = 0;
    virtual bool BatchWrite(const map<string, string > &mapContractDb) = 0;
    virtual bool EraseKey(const string &key) = 0;
    virtual bool HaveData(const string &key) = 0;
    virtual bool GetScript(const int nIndex, string &contractRegId, string &value) = 0;
    virtual bool GetContractData(const int nCurBlockHeight, const string &contractRegId, const int &nIndex,
                                string &contractKey, string &contractData) = 0;
    virtual Object ToJsonObj(string prefix) { return Object(); } //FIXME: useless prefix

    // virtual bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) = 0;
    // virtual bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CDbOpLog> &vTxIndexOperDB) = 0;
    // virtual bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CDbOpLog &operLog) = 0;
    // virtual bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) = 0;
    virtual bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<string, string > &mapTxHash) = 0;
    // virtual bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CDbOpLog &operLog) = 0;
    virtual bool GetAllContractAcc(const CRegID &scriptId, map<string, string > &mapAcc) = 0;

    virtual ~IContractView(){};
};

class CContractCache : public IContractView {
protected:
    IContractView *pBase;

public:
    map<string, string > mapContractDb;

public:
    CContractCache(): pBase(nullptr) {}

    CContractCache(IContractView &pBaseIn): pBase(&pBaseIn) { mapContractDb.clear(); };

    bool GetScript(const CRegID &scriptId, string &value) { return true; }
    bool GetScript(const int nIndex, CRegID &scriptId, string &value) { return true; }
    bool GetScriptAcc(const CRegID &scriptId, const string &key, CAppUserAccount &appAccOut) { return true; }
    bool SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccIn, CDbOpLog &operlog) { return true; }
    bool EraseScriptAcc(const CRegID &scriptId, const string &key) { return true; }
    bool SetScript(const CRegID &scriptId, const string &value) { return true; }
    bool HaveScript(const CRegID &scriptId) { return true; }
    bool EraseScript(const CRegID &scriptId) { return true; }
    bool GetContractItemCount(const CRegID &scriptId, int &nCount) { return true; }
    bool EraseAppData(const CRegID &scriptId, const string &contractKey, CDbOpLog &operLog) { return true; }
    bool HaveScriptData(const CRegID &scriptId, const string &contractKey) { return true; }
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const string &contractKey,
                         string &vScriptData)  { return true; }
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex,
                         string &contractKey, string &vScriptData) { return true; }
    bool SetContractData(const CRegID &scriptId, const string &contractKey,
                         const string &vScriptData, CDbOpLog &operLog) { return true; }
    bool SetDelegateData(const CAccount &delegateAcct, CDbOpLog &operLog) { return true; }
    bool SetDelegateData(const string &key) { return true; }
    bool EraseDelegateData(const CAccountLog &delegateAcct, CDbOpLog &operLog) { return true; }
    bool EraseDelegateData(const string &key) { return true; }
    bool UndoScriptData(const string &key, const string &value) { return true; }

    /**
     * @brief Get all number of scripts in scriptdb
     * @param nCount
     * @return true if get succeed, otherwise false
     */
    bool GetScriptCount(int &nCount) { return true; }
    bool SetTxRelAccout(const uint256 &txHash, const set<CKeyID> &relAccount) { return true; }
    bool GetTxRelAccount(const uint256 &txHash, set<CKeyID> &relAccount) { return true; }
    bool EraseTxRelAccout(const uint256 &txHash) { return true; }
    /**
     * @brief write all data in the caches to script db
     * @return
     */
    bool Flush() { return true; }
    bool Flush(IContractView *pView) { return true; }
    unsigned int GetCacheSize() { return 0; }
    Object ToJsonObj() const { return Object(); }
	IContractView * GetBaseScriptDB() { return pBase; }
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) { return true; }
    bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CDbOpLog> &vTxIndexOperDB) { return true; }
    void SetBaseView(IContractView *pBaseIn) { pBase = pBaseIn; }
    string ToString() { return ""; }
    bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CDbOpLog &operLog) { return true; }
    bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) { return true; }
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<string, string > &mapTxHash) { return true; }
    bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CDbOpLog &operLog) { return true; }
    bool GetAllContractAcc(const CRegID &scriptId, map<string, string > &mapAcc) { return true; }

private:
    bool GetData(const string &key, string &value) { return true; }
    bool SetData(const string &key, const string &value) { return true; }
    bool BatchWrite(const map<string, string > &mapContractDb) { return true; }
    bool EraseKey(const string &key) { return true; }
    bool HaveData(const string &key) { return true; }

    /**
     * @brief Get script content from scriptdb by scriptid
     * @param vScriptId
     * @param vValue
     * @return true if get script succeed,otherwise false
     */
    bool GetScript(const string &contractRegId, string &value) { return true; }
    /**
     * @brief Get Script content from scriptdb by index
     * @param nIndex the value must be non-negative
     * @param vScriptId
     * @param vValue
     * @return true if get script succeed, otherwise false
     */
    bool GetScript(const int nIndex, string &contractRegId, string &value) { return true; }
    /**
     * @brief Save script content to scriptdb
     * @param vScriptId
     * @param vValue
     * @return true if save succeed, otherwise false
     */
    bool SetScript(const string &contractRegId, const string &value) { return true; }
    /**
     * @brief Detect if scriptdb contains the script by scriptid
     * @param vScriptId
     * @return true if contains script, otherwise false
     */
    bool HaveScript(const string &vScriptId) { return true; }
    /**
     * @brief Save all number of scripts in scriptdb
     * @param nCount
     * @return true if save count succeed, otherwise false
     */
    bool SetScriptCount(const int nCount) { return true; }
    /**
     * @brief Delete script from script db by scriptId
     * @param vScriptId
     * @return true if delete succeed, otherwise false
     */
    bool EraseScript(const string &vScriptId) { return true; }
    /**
     * @brief Get total number of contract data elements in contract db
     * @param vScriptId
     * @param nCount
     * @return true if get succeed, otherwise false
     */
    bool GetContractItemCount(const string &contractRegId, int &nCount) { return true; }
    /**
     * @brief Save count of the Contract's data into contract db
     * @param vScriptId
     * @param nCount
     * @return true if save succeed, otherwise false
     */
    bool SetContractItemCount(const string &contractRegId, int nCount) { return true; }
    /**
     * @brief Delete the item of the scirpt's data by scriptId and scriptKey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if delete succeed, otherwise false
     */
    bool EraseAppData(const string &contractRegId, const string &contractKey, CDbOpLog &operLog) { return true; }

    bool EraseAppData(const string &key) { return true; }
    /**
     * @brief Detect if scriptdb contains the item of script's data by scriptid and scriptkey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if contains the item, otherwise false
     */
    bool HaveScriptData(const string &contractRegId, const string &contractKey) { return true; }
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
                         string &vScriptData) { return true; }
    /**
     * @brief Save script data and valid height into script db
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @param vScriptData
     * @param nHeight valide height of script data
     * @return true if save succeed, otherwise false
     */
    bool SetContractData(const string &contractRegId, const string &contractKey,
                         const string &vScriptData, CDbOpLog &operLog) { return true; }
};

class CContractDB : public IContractView {
private:
    CLevelDBWrapper db;

public:
    CContractDB(const string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false) :
        db(GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe) {}

    CContractDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false) :
        db(GetDataDir() / "blocks" / "contract", nCacheSize, fMemory, fWipe) {}

private:
    CContractDB(const CContractDB &);
    void operator=(const CContractDB &) {}

public:
    bool GetData(const string &key, string &value) { return db.Read(key, value); }
    bool SetData(const string &key, const string &value) { return db.Write(key, value); }

    bool BatchWrite(const map<string, string > &mapContractDb) { return true; }
    bool EraseKey(const string &key) { return true; }
    bool HaveData(const string &key) { return true; }
    bool GetScript(const int nIndex, string &contractRegId, string &value) { return true; }
    bool GetContractData(const int curBlockHeight, const string &contractRegId, const int &nIndex,
                        string &contractKey, string &vScriptData) { return true; }
    int64_t GetDbCount() { return db.GetDbCount(); }
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<string, string > &mapTxHash) { return true; }
    Object ToJsonObj(string Prefix) { return Object(); }
    bool GetAllContractAcc(const CRegID &contractRegId, map<string, string > &mapAcc) { return true; }
};

#endif  // PERSIST_CONTRACTDB_H
