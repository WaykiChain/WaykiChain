// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CONTRACTDB_H
#define PERSIST_CONTRACTDB_H

#include "commons/arith_uint256.h"
#include "leveldbwrapper.h"
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

class CContractDBOperLog {
public:
    vector<unsigned char> vKey;
    vector<unsigned char> vValue;

    CContractDBOperLog(const vector<unsigned char> &vKeyIn, const vector<unsigned char> &vValueIn) {
        vKey   = vKeyIn;
        vValue = vValueIn;
    }
    CContractDBOperLog() {
        vKey.clear();
        vValue.clear();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vKey);
        READWRITE(vValue);)

    string ToString() const {
        string str;
        str += strprintf("vKey: %s, vValue: %s", HexStr(vKey), HexStr(vValue));
        return str;
    }

    friend bool operator<(const CContractDBOperLog &log1, const CContractDBOperLog &log2) {
        return log1.vKey < log2.vKey;
    }
};


class IContractView {
public:
    virtual bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) = 0;
    virtual bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) = 0;
    virtual bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb) = 0;
    virtual bool EraseKey(const vector<unsigned char> &vKey) = 0;
    virtual bool HaveData(const vector<unsigned char> &vKey) = 0;
    virtual bool GetScript(const int nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue) = 0;
    virtual bool GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId, const int &nIndex,
                                vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) = 0;
    virtual Object ToJsonObj(string prefix) { return Object(); } //FIXME: useless prefix

    // virtual bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) = 0;
    // virtual bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CContractDBOperLog> &vTxIndexOperDB) = 0;
    // virtual bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CContractDBOperLog &operLog) = 0;
    // virtual bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) = 0;
    virtual bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash) = 0;
    // virtual bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CContractDBOperLog &operLog) = 0;
    virtual bool GetAllContractAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc) = 0;

    virtual ~IContractView(){};
};

class CContractCache : public IContractView {
protected:
    IContractView *pBase;

public:
    map<vector<unsigned char>, vector<unsigned char> > mapContractDb;
    /*取脚本 时 第一个vector 是scriptKey = "def" + "scriptid";
      取应用账户时第一个vector是scriptKey = "acct" + "scriptid"+"_" + "accUserId";
      取脚本总条数时第一个vector是scriptKey ="snum",
      取脚本数据总条数时第一个vector是scriptKey ="sdnum";
      取脚本数据时第一个vector是scriptKey ="data" + "vScriptId" + "_" + "vScriptKey"
      取交易关联账户时第一个vector是scriptKey ="tx" + "txHash"
    */
public:
    CContractCache(IContractView &pBaseIn): pBase(&pBaseIn) { mapContractDb.clear(); };

    bool GetScript(const CRegID &scriptId, vector<unsigned char> &vValue);
    bool GetScript(const int nIndex, CRegID &scriptId, vector<unsigned char> &vValue);
    bool GetScriptAcc(const CRegID &scriptId, const vector<unsigned char> &vKey, CAppUserAccount &appAccOut);
    bool SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccIn, CContractDBOperLog &operlog);
    bool EraseScriptAcc(const CRegID &scriptId, const vector<unsigned char> &vKey);
    bool SetScript(const CRegID &scriptId, const vector<unsigned char> &vValue);
    bool HaveScript(const CRegID &scriptId);
    bool EraseScript(const CRegID &scriptId);
    bool GetContractItemCount(const CRegID &scriptId, int &nCount);
    bool EraseAppData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey, CContractDBOperLog &operLog);
    bool HaveScriptData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const vector<unsigned char> &vScriptKey,
                         vector<unsigned char> &vScriptData);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex,
                         vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData);
    bool SetContractData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey,
                         const vector<unsigned char> &vScriptData, CContractDBOperLog &operLog);
    bool SetDelegateData(const CAccount &delegateAcct, CContractDBOperLog &operLog);
    bool SetDelegateData(const vector<unsigned char> &vKey);
    bool EraseDelegateData(const CAccountLog &delegateAcct, CContractDBOperLog &operLog);
    bool EraseDelegateData(const vector<unsigned char> &vKey);
    bool UndoScriptData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue);
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
    unsigned int GetCacheSize();
    Object ToJsonObj() const;
	IContractView * GetBaseScriptDB() { return pBase; }
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CContractDBOperLog> &vTxIndexOperDB);
    void SetBaseView(IContractView *pBaseIn) { pBase = pBaseIn; };
    string ToString();
    bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CContractDBOperLog &operLog);
    bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput);
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CContractDBOperLog &operLog);
    bool GetAllContractAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc);

private:
    bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue);
    bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue);
    bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb);
    bool EraseKey(const vector<unsigned char> &vKey);
    bool HaveData(const vector<unsigned char> &vKey);

    /**
     * @brief Get script content from scriptdb by scriptid
     * @param vScriptId
     * @param vValue
     * @return true if get script succeed,otherwise false
     */
    bool GetScript(const vector<unsigned char> &vScriptId, vector<unsigned char> &vValue);
    /**
     * @brief Get Script content from scriptdb by index
     * @param nIndex the value must be non-negative
     * @param vScriptId
     * @param vValue
     * @return true if get script succeed, otherwise false
     */
    bool GetScript(const int nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue);
    /**
     * @brief Save script content to scriptdb
     * @param vScriptId
     * @param vValue
     * @return true if save succeed, otherwise false
     */
    bool SetScript(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vValue);
    /**
     * @brief Detect if scriptdb contains the script by scriptid
     * @param vScriptId
     * @return true if contains script, otherwise false
     */
    bool HaveScript(const vector<unsigned char> &vScriptId);
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
    bool EraseScript(const vector<unsigned char> &vScriptId);
    /**
     * @brief Get total number of contract data elements in contract db
     * @param vScriptId
     * @param nCount
     * @return true if get succeed, otherwise false
     */
    bool GetContractItemCount(const vector<unsigned char> &vScriptId, int &nCount);
    /**
     * @brief Save count of the Contract's data into contract db
     * @param vScriptId
     * @param nCount
     * @return true if save succeed, otherwise false
     */
    bool SetContractItemCount(const vector<unsigned char> &vScriptId, int nCount);
    /**
     * @brief Delete the item of the scirpt's data by scriptId and scriptKey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if delete succeed, otherwise false
     */
    bool EraseAppData(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey, CContractDBOperLog &operLog);

    bool EraseAppData(const vector<unsigned char> &vKey);
    /**
     * @brief Detect if scriptdb contains the item of script's data by scriptid and scriptkey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @return true if contains the item, otherwise false
     */
    bool HaveScriptData(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey);
    /**
     * @brief Get smart contract App data and valid height by scriptid and scriptkey
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @param vScriptData
     * @param nHeight valide height of script data
     * @return true if get succeed, otherwise false
     */
    bool GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey,
                         vector<unsigned char> &vScriptData);
    /**
     * @brief Get smart contract app data and valid height by scriptid and nIndex
     * @param vScriptId
     * @param nIndex get first script data will be 0, otherwise be 1
     * @param vScriptKey must be 8 bytes, get first script data will be empty, otherwise get next scirpt data will be previous script key
     * @param vScriptData
     * @param nHeight valid height of script data
     * @return true if get succeed, otherwise false
     */
    bool GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId, const int &nIndex, vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData);
    /**
     * @brief Save script data and valid height into script db
     * @param vScriptId
     * @param vScriptKey must be 8 bytes
     * @param vScriptData
     * @param nHeight valide height of script data
     * @return true if save succeed, otherwise false
     */
    bool SetContractData(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey,
                         const vector<unsigned char> &vScriptData, CContractDBOperLog &operLog);
};

class CContractDB : public IContractView {
private:
    CLevelDBWrapper db;

public:
    CContractDB(const string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false) :
        db(GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe) {}

    CContractDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false) :
        db(GetDataDir() / "blocks" / "script", nCacheSize, fMemory, fWipe) {}

private:
    CContractDB(const CContractDB &);
    void operator=(const CContractDB &);

public:
    bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) { return db.Read(vKey, vValue); };
    bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) { return db.Write(vKey, vValue); };

    bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb);
    bool EraseKey(const vector<unsigned char> &vKey);
    bool HaveData(const vector<unsigned char> &vKey);
    bool GetScript(const int nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue);
    bool GetContractData(const int curBlockHeight, const vector<unsigned char> &vScriptId, const int &nIndex,
                        vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData);
    int64_t GetDbCount() { return db.GetDbCount(); }
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash);
    Object ToJsonObj(string Prefix);
    bool GetAllContractAcc(const CRegID &contractRegId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc);
};

#endif  // PERSIST_CONTRACTDB_H
