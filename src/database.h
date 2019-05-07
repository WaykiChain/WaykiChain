#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <map>
#include <vector>
#include "vm/appaccount.h"
#include "commons/serialize.h"
#include "tx/tx.h"

using namespace std;

class CAccount;
class CKeyID;
class uint256;
struct CDiskTxPos;
class CVmOperate;

class CAccountView {
public:
    virtual bool GetAccount(const CKeyID &keyId, CAccount &account) = 0;
    virtual bool GetAccount(const vector<unsigned char> &accountId, CAccount &account) = 0;
    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account) = 0;
    virtual bool SetAccount(const vector<unsigned char> &accountId, const CAccount &account) = 0;
    // virtual bool SetAccount(const CUserID &userId, const CAccount &account) = 0;
    virtual bool HaveAccount(const CKeyID &keyId) = 0;
    virtual uint256 GetBestBlock() = 0;
    virtual bool SetBestBlock(const uint256 &blockHash) = 0;
    virtual bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>,
                            CKeyID> &mapKeyIds, const uint256 &blockHash) = 0;
    virtual bool BatchWrite(const vector<CAccount> &vAccounts) = 0;
    virtual bool EraseAccountByKeyId(const CKeyID &keyId) = 0;
    virtual bool SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId) = 0;
    virtual bool GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId) = 0;
    virtual bool EraseAccountByRegId(const vector<unsigned char> &accountRegId) = 0;

    // virtual bool SaveAccountInfo(const vector<unsigned char> &accountId, const CKeyID &keyId, const CAccount &account) = 0;
    virtual std::tuple<uint64_t, uint64_t> TraverseAccount() = 0;
    virtual Object ToJsonObj(char prefix) = 0;

    virtual ~CAccountView() {};
};

class CAccountViewCache : public CAccountView {
protected:
    CAccountView *pBase;

public:
    uint256 blockHash;
    map<CKeyID, CAccount> cacheAccounts;                    // <KeyID -> Account>
    map<vector<unsigned char>, CKeyID> cacheRegId2KeyIds;   // <RegID -> KeyID>
    map<vector<unsigned char>, CKeyID> cacheNickId2KeyIds;  // <NickID -> KeyID>

public:
    virtual bool GetAccount(const CKeyID &keyId, CAccount &account);
    virtual bool GetAccount(const vector<unsigned char> &accountId, CAccount &account);
    virtual bool GetAccount(const CUserID &userId, CAccount &account);
    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account);
    virtual bool SetAccount(const vector<unsigned char> &accountId, const CAccount &account);
    virtual bool SetAccount(const CUserID &userId, const CAccount &account);
    virtual bool HaveAccount(const CKeyID &keyId);
    virtual uint256 GetBestBlock();
    virtual bool SetBestBlock(const uint256 &blockHash);
    virtual bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>,
                            CKeyID> &mapKeyIds, const uint256 &blockHash);
    virtual bool BatchWrite(const vector<CAccount> &vAccounts);
    virtual bool EraseAccountByKeyId(const CKeyID &keyId);
    virtual bool SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId);
    virtual bool SetKeyId(const CUserID &userId, const CKeyID &keyId);
    virtual bool GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId);
    virtual bool EraseAccountByRegId(const vector<unsigned char> &accountRegId);

    // virtual bool SaveAccountInfo(const vector<unsigned char> &accountId, const CKeyID &keyId, const CAccount &account);
    virtual bool SaveAccountInfo(const CAccount &account);
    virtual std::tuple<uint64_t, uint64_t> TraverseAccount();
    virtual Object ToJsonObj(char prefix) { return Object(); }

public:
    CAccountViewCache(CAccountView &view): pBase(&view), blockHash(uint256()) {}
    ~CAccountViewCache() {}

    bool GetUserId(const string &addr, CUserID &userId);
    bool GetRegId(const CKeyID &keyId, CRegID &regId);
    bool GetRegId(const CUserID &userId, CRegID &regId) const;
    bool GetKeyId(const CUserID &userId, CKeyID &keyId);
    bool EraseAccountByKeyId(const CUserID &userId);
    bool EraseId(const CUserID &userId);
    bool HaveAccount(const CUserID &userId);
    int64_t GetFreeBCoins(const CUserID &userId) const;
    bool Flush();
    unsigned int GetCacheSize();
    Object ToJsonObj() const;
    void SetBaseView(CAccountView *pBaseIn) { pBase = pBaseIn; };
};

class CScriptDBView {
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
    // virtual bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CScriptDBOperLog> &vTxIndexOperDB) = 0;
    // virtual bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CScriptDBOperLog &operLog) = 0;
    // virtual bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) = 0;
    virtual bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash) = 0;
    // virtual bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CScriptDBOperLog &operLog) = 0;
    virtual bool GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc) = 0;

    virtual ~CScriptDBView(){};
};

class CScriptDBViewCache : public CScriptDBView {
protected:
    CScriptDBView *pBase;

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
    CScriptDBViewCache(CScriptDBView &pBaseIn): pBase(&pBaseIn) { mapContractDb.clear(); };

    bool GetScript(const CRegID &scriptId, vector<unsigned char> &vValue);
    bool GetScript(const int nIndex, CRegID &scriptId, vector<unsigned char> &vValue);
    bool GetScriptAcc(const CRegID &scriptId, const vector<unsigned char> &vKey, CAppUserAccount &appAccOut);    
    bool SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccIn, CScriptDBOperLog &operlog);
    bool EraseScriptAcc(const CRegID &scriptId, const vector<unsigned char> &vKey);
    bool SetScript(const CRegID &scriptId, const vector<unsigned char> &vValue);
    bool HaveScript(const CRegID &scriptId);
    bool EraseScript(const CRegID &scriptId);
    bool GetContractItemCount(const CRegID &scriptId, int &nCount);
    bool EraseAppData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey, CScriptDBOperLog &operLog);
    bool HaveScriptData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const vector<unsigned char> &vScriptKey,
                         vector<unsigned char> &vScriptData);
    bool GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex,
                         vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData);
    bool SetContractData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey,
                         const vector<unsigned char> &vScriptData, CScriptDBOperLog &operLog);
    bool SetDelegateData(const CAccount &delegateAcct, CScriptDBOperLog &operLog);
    bool SetDelegateData(const vector<unsigned char> &vKey);
    bool EraseDelegateData(const CAccountLog &delegateAcct, CScriptDBOperLog &operLog);
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
	CScriptDBView * GetBaseScriptDB() { return pBase; }
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CScriptDBOperLog> &vTxIndexOperDB);
    void SetBaseView(CScriptDBView *pBaseIn) { pBase = pBaseIn; };
    string ToString();
    bool WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CScriptDBOperLog &operLog);
    bool ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput);
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash);
    bool SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CScriptDBOperLog &operLog);
    bool GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc);

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
    bool EraseAppData(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey, CScriptDBOperLog &operLog);

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
                         const vector<unsigned char> &vScriptData, CScriptDBOperLog &operLog);
};

class CTransactionDBView {
public:
    virtual bool IsContainBlock(const CBlock &block) = 0;
    virtual bool BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHashIn) = 0;

    virtual ~CTransactionDBView(){};
};


class CTransactionDBCache : public CTransactionDBView {
protected:
    CTransactionDBView *pBase;

private:
    // CTransactionDBCache(CTransactionDBCache &transactionView);
    map<uint256, UnorderedHashSet> mapTxHashByBlockHash;  // key:block hash  value:tx hash
    bool IsInMap(const map<uint256, UnorderedHashSet> &mMap, const uint256 &hash) const;

public:
    CTransactionDBCache(CTransactionDBView &pBaseIn) : pBase(&pBaseIn) {};

    bool HaveTx(const uint256 &txHash);
    bool IsContainBlock(const CBlock &block);
    bool AddBlockToCache(const CBlock &block);
    bool DeleteBlockFromCache(const CBlock &block);
    map<uint256, UnorderedHashSet> GetTxHashCache();
    bool BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHashIn);
    void AddTxHashCache(const uint256 &blockHash, const UnorderedHashSet &vTxHash);
    bool Flush();
    void Clear();
    Object ToJsonObj() const;
    int GetSize();
    void SetBaseView(CTransactionDBView *pBaseIn) { pBase = pBaseIn; };
    const map<uint256, UnorderedHashSet> &GetCacheMap();
    void SetCacheMap(const map<uint256, UnorderedHashSet> &mapCache);
};

#endif
