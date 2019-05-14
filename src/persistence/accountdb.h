// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2016 The Coin developers
// Copyright (c) 2014-2019 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef PERSIST_ACCOUNTDB_H
#define PERSIST_ACCOUNTDB_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "commons/arith_uint256.h"
#include "leveldbwrapper.h"
#include "main.h"

class uint256;
class CKeyID;


class CAccountView {
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

    virtual ~CAccountView() {};
};

class CAccountViewCache : public CAccountView {
protected:
    CAccountView *pBase;

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
    CAccountViewCache(CAccountView &view): pBase(&view), blockHash(uint256()) {}
    ~CAccountViewCache() {}

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
    void SetBaseView(CAccountView *pBaseIn) { pBase = pBaseIn; };
};

class CAccountViewDB : public CAccountView {
private:
    CLevelDBWrapper db;

public:
    CAccountViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    CAccountViewDB(const string &name, size_t nCacheSize, bool fMemory, bool fWipe);

private:
    CAccountViewDB(const CAccountViewDB &);
    void operator=(const CAccountViewDB &);

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
