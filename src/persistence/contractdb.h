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

    bool GetScript(const CRegID &scriptId, string &value);
    bool GetScript(const int nIndex, CRegID &scriptId, string &value);
    bool GetScriptAcc(const CRegID &scriptId, const string &key, CAppUserAccount &appAccOut);
    bool SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccIn, CDbOpLog &operlog);
    bool EraseScriptAcc(const CRegID &scriptId, const string &key);
    bool SetScript(const CRegID &scriptId, const string &value);
    bool HaveScript(const CRegID &scriptId);
    bool EraseScript(const CRegID &scriptId);
    bool GetContractItemCount(const CRegID &scriptId, int &nCount);
    bool EraseAppData(const CRegID &scriptId, const string &contractKey, CDbOpLog &operLog);
    bool HaveScriptData(const CRegID &scriptId, const string &contractKey);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const string &contractKey,
                         string &vScriptData);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex,
                         string &contractKey, string &vScriptData);
    bool SetContractData(const CRegID &scriptId, const string &contractKey,
                         const string &vScriptData, CDbOpLog &operLog);
    bool SetDelegateData(const CAccount &delegateAcct, CDbOpLog &operLog);
    bool SetDelegateData(const string &key);
    bool EraseDelegateData(const CAccountLog &delegateAcct, CDbOpLog &operLog);
    bool EraseDelegateData(const string &key);
    bool UndoScriptData(const string &key, const string &value);

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
    bool Flush(IContractView *pView);
    unsigned int GetCacheSize();
    Object ToJsonObj() const;
	IContractView * GetBaseScriptDB() { return pBase; }
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CDbOpLog> &vTxIndexOperDB);
    void SetBaseView(IContractView *pBaseIn) { pBase = pBaseIn; };
    string ToString();
    bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CDbOpLog &operLog);
    bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput);
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<string, string > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CDbOpLog &operLog);
    bool GetAllContractAcc(const CRegID &scriptId, map<string, string > &mapAcc);

private:
    bool GetData(const string &key, string &value);
    bool SetData(const string &key, const string &value);
    bool BatchWrite(const map<string, string > &mapContractDb);
    bool EraseKey(const string &key);
    bool HaveData(const string &key);

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
    /**
     * @brief Delete the item of the scirpt's data by scriptId and scriptKey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if delete succeed, otherwise false
     */
    bool EraseAppData(const string &contractRegId, const string &contractKey, CDbOpLog &operLog);

    bool EraseAppData(const string &key);
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
    bool GetContractData(const int nCurBlockHeight, const string &contractRegId, const int &nIndex, string &contractKey, string &vScriptData);
    /**
     * @brief Save script data and valid height into script db
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @param vScriptData
     * @param nHeight valide height of script data
     * @return true if save succeed, otherwise false
     */
    bool SetContractData(const string &contractRegId, const string &contractKey,
                         const string &vScriptData, CDbOpLog &operLog);
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
    void operator=(const CContractDB &);

public:
    bool GetData(const string &key, string &value) { return db.Read(key, value); };
    bool SetData(const string &key, const string &value) { return db.Write(key, value); };

    bool BatchWrite(const map<string, string > &mapContractDb);
    bool EraseKey(const string &key);
    bool HaveData(const string &key);
    bool GetScript(const int nIndex, string &contractRegId, string &value);
    bool GetContractData(const int curBlockHeight, const string &contractRegId, const int &nIndex,
                        string &contractKey, string &vScriptData);
    int64_t GetDbCount() { return db.GetDbCount(); }
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<string, string > &mapTxHash);
    Object ToJsonObj(string Prefix);
    bool GetAllContractAcc(const CRegID &contractRegId, map<string, string > &mapAcc);
};

#endif  // PERSIST_CONTRACTDB_H
