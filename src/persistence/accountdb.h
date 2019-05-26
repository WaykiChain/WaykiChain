// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_ACCOUNTDB_H
#define PERSIST_ACCOUNTDB_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "commons/arith_uint256.h"
#include "leveldbwrapper.h"
#include "accounts/account.h"

class uint256;
class CKeyID;

class IAccountView {
public:
    virtual bool GetAccount(const CKeyID &keyId, CAccount &account) = 0;
    virtual bool GetAccount(const vector<unsigned char> &accountRegId, CAccount &account) = 0;
    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account) = 0;
    virtual bool SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account) = 0;
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
    virtual bool EraseKeyIdByRegId(const vector<unsigned char> &accountRegId) = 0;

    // virtual bool SaveAccountInfo(const vector<unsigned char> &accountId, const CKeyID &keyId,
    // const CAccount &account) = 0;
    virtual std::tuple<uint64_t, uint64_t> TraverseAccount() = 0;
    virtual Object ToJsonObj(char prefix)                    = 0;

    virtual ~IAccountView() {};
};

class CAccountCache : public IAccountView {
protected:
    IAccountView *pBase;

public:
    uint256 blockHash;
    map<CKeyID, CAccount> mapKeyId2Account;              // <KeyID -> Account>
    map<vector<unsigned char>, CKeyID> mapRegId2KeyId;   // <RegID -> KeyID>
    map<vector<unsigned char>, CKeyID> mapNickId2KeyId;  // <NickID -> KeyID>

public:
    virtual bool GetAccount(const CKeyID &keyId, CAccount &account);
    virtual bool GetAccount(const vector<unsigned char> &accountRegId, CAccount &account);
    virtual bool GetAccount(const CUserID &userId, CAccount &account);
    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account);
    virtual bool SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account);
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
    virtual bool EraseKeyIdByRegId(const vector<unsigned char> &accountRegId);

    // virtual bool SaveAccountInfo(const vector<unsigned char> &accountId, const CKeyID &keyId, const CAccount &account);
    virtual bool SaveAccountInfo(const CAccount &account);
    virtual std::tuple<uint64_t, uint64_t> TraverseAccount();
    virtual Object ToJsonObj(char prefix) { return Object(); }

public:
    CAccountCache(IAccountView &view): pBase(&view), blockHash(uint256()) {}

    CAccountCache(IAccountView *pAccountView, CAccountCache *pAccountCache): pBase(pAccountView) {
        blockHash        = pAccountCache->blockHash;
        mapKeyId2Account = pAccountCache->mapKeyId2Account;
        mapRegId2KeyId   = pAccountCache->mapRegId2KeyId;
        mapNickId2KeyId  = pAccountCache->mapNickId2KeyId;
    }

    ~CAccountCache() {}

    bool GetUserId(const string &addr, CUserID &userId);
    bool GetRegId(const CKeyID &keyId, CRegID &regId);
    bool GetRegId(const CUserID &userId, CRegID &regId) const;
    bool RegIDIsMature(const CRegID &regId) const;
    bool GetKeyId(const CUserID &userId, CKeyID &keyId);
    bool EraseAccountByKeyId(const CUserID &userId);
    bool EraseKeyId(const CUserID &userId);
    bool HaveAccount(const CUserID &userId);
    int64_t GetFreeBCoins(const CUserID &userId) const;
    bool Flush();
    unsigned int GetCacheSize();
    Object ToJsonObj() const;
    void SetBaseView(IAccountView *pBaseIn) { pBase = pBaseIn; };
};

class CAccountDB : public IAccountView {
private:
    CLevelDBWrapper db;

public:
    CAccountDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false) :
                db( GetDataDir() / "blocks" / "account", nCacheSize, fMemory, fWipe ) {}

    CAccountDB(const string &name, size_t nCacheSize, bool fMemory, bool fWipe) :
                db( GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe ) {}

private:
    CAccountDB(const CAccountDB &);
    void operator=(const CAccountDB &);

public:
    bool GetAccount(const CKeyID &keyId, CAccount &account);
    bool GetAccount(const vector<unsigned char> &accountId, CAccount &account);
    bool GetAccount(const CUserID &userId, CAccount &account);

    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account);
    // virtual bool SetAccount(const CUserID &userId, const CAccount &account) {};
    virtual bool SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account);

    bool HaveAccount(const CKeyID &keyId);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>, CKeyID> &mapKeyIds, const uint256 &hashBlock);
    bool BatchWrite(const vector<CAccount> &vAccounts);
    bool EraseAccountByKeyId(const CKeyID &keyId);
    bool SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId);
    bool GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId);
    bool EraseKeyIdByRegId(const vector<unsigned char> &accountRegId);

    bool SaveAccountInfo(const CAccount &account);
    std::tuple<uint64_t, uint64_t> TraverseAccount();
    int64_t GetDbCount() { return db.GetDbCount(); }
    Object ToJsonObj(char Prefix);
};

#endif  // PERSIST_ACCOUNTDB_H
