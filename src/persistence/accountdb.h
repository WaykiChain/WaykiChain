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
#include "dbconf.h"
#include "dbaccess.h"

class uint256;
class CKeyID;

/*
class IAccountView {
public:
    virtual bool GetAccount(const CKeyID &keyId, CAccount &account) = 0;
    virtual bool GetAccount(const CRegID &regId, CAccount &account) = 0;
    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account) = 0;
    virtual bool SetAccount(const CRegID &regId, const CAccount &account) = 0;
    // virtual bool SetAccount(const CUserID &userId, const CAccount &account) = 0;
    virtual bool HaveAccount(const CKeyID &keyId) = 0;
    virtual uint256 GetBestBlock() = 0;
    virtual bool SetBestBlock(const uint256 &blockHash) = 0;
    virtual bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<CRegID,
                            CKeyID> &mapKeyIds, const uint256 &blockHash) = 0;
    virtual bool BatchWrite(const vector<CAccount> &vAccounts) = 0;
    virtual bool EraseAccountByKeyId(const CKeyID &keyId) = 0;
    virtual bool SetKeyId(const CRegID &regId, const CKeyID &keyId) = 0;
    virtual bool GetKeyId(const CRegID &regId, CKeyID &keyId) = 0;
    virtual bool EraseKeyIdByRegId(const CRegID &regId) = 0;

    // virtual bool SaveAccount(const CRegID &regId, const CKeyID &keyId,
    // const CAccount &account) = 0;
    virtual std::tuple<uint64_t, uint64_t> TraverseAccount() = 0;
    virtual Object ToJsonObj(dbk::PrefixType prefix = dbk::EMPTY) = 0;

    virtual ~IAccountView() {};
};
*/

class CAccountDBCache {
public:
    bool GetFcoinGenesisAccount(CAccount &fcoinGenesisAccount) const;

    bool GetAccount(const CKeyID &keyId, CAccount &account) const;
    bool GetAccount(const CRegID &regId, CAccount &account) const;
    bool GetAccount(const CUserID &userId, CAccount &account) const;
    bool SetAccount(const CKeyID &keyId, const CAccount &account);
    bool SetAccount(const CRegID &regId, const CAccount &account);
    bool SetAccount(const CUserID &userId, const CAccount &account);
    bool HaveAccount(const CKeyID &keyId) const;
    uint256 GetBestBlock() const;
    bool SetBestBlock(const uint256 &blockHash);
    bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<CRegID,
                            CKeyID> &mapKeyIds, const uint256 &blockHash);
    bool BatchWrite(const vector<CAccount> &vAccounts);
    bool EraseAccountByKeyId(const CKeyID &keyId);
    bool SetKeyId(const CRegID &regId, const CKeyID &keyId);
    bool SetKeyId(const CUserID &userId, const CKeyID &keyId);
    bool GetKeyId(const CRegID &regId, CKeyID &keyId) const;
    bool GetKeyId(const CUserID &userId, CKeyID &keyId) const;
    bool EraseKeyIdByRegId(const CRegID &regId);

    bool SaveAccount(const CAccount &account);
    std::tuple<uint64_t, uint64_t> TraverseAccount();

public:
    CAccountDBCache() {}

    CAccountDBCache(CDBAccess *pDbAccess):
        blockHashCache(pDbAccess),
        keyId2AccountCache(pDbAccess),
        regId2KeyIdCache(pDbAccess),
        nickId2KeyIdCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ACCOUNT);
    }

    CAccountDBCache(CAccountDBCache *pBase):
        blockHashCache(pBase->blockHashCache),
        keyId2AccountCache(pBase->keyId2AccountCache),
        regId2KeyIdCache(pBase->regId2KeyIdCache),
        nickId2KeyIdCache(pBase->nickId2KeyIdCache) {}

    ~CAccountDBCache() {}

    bool GetUserId(const string &addr, CUserID &userId) const;
    bool GetRegId(const CKeyID &keyId, CRegID &regId) const;
    bool GetRegId(const CUserID &userId, CRegID &regId) const;
    bool RegIDIsMature(const CRegID &regId) const;
    bool EraseAccountByKeyId(const CUserID &userId);
    bool EraseKeyId(const CUserID &userId);
    bool HaveAccount(const CUserID &userId) const;
    int64_t GetFreeBcoins(const CUserID &userId) const;
    bool Flush();
    unsigned int GetCacheSize() const;
    Object ToJsonObj(dbk::PrefixType prefix = dbk::EMPTY);
    void SetBaseView(CAccountDBCache *pBaseIn) {
        blockHashCache.SetBase(&pBaseIn->blockHashCache);
        keyId2AccountCache.SetBase(&pBaseIn->keyId2AccountCache);
        regId2KeyIdCache.SetBase(&pBaseIn->regId2KeyIdCache);
        nickId2KeyIdCache.SetBase(&pBaseIn->nickId2KeyIdCache);
     };
private:
/*  CDBScalarValueCache     prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    // best blockHash
    CDBScalarValueCache< dbk::BEST_BLOCKHASH,     uint256>        blockHashCache;

/*  CDBScalarValueCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <KeyID -> Account>
    CDBMultiValueCache< dbk::KEYID_ACCOUNT,        CKeyID,       CAccount >       keyId2AccountCache;
    // <RegID str -> KeyID>
    CDBMultiValueCache< dbk::REGID_KEYID,          string,       CKeyID >         regId2KeyIdCache;
    // <NickID -> KeyID>
    CDBMultiValueCache< dbk::NICKID_KEYID,         CNickID,      CKeyID>          nickId2KeyIdCache;
};

/*
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
    bool GetAccount(const CRegID &regId, CAccount &account);
    bool GetAccount(const CUserID &userId, CAccount &account);

    virtual bool SetAccount(const CKeyID &keyId, const CAccount &account);
    // virtual bool SetAccount(const CUserID &userId, const CAccount &account) {};
    virtual bool SetAccount(const CRegID &regId, const CAccount &account);

    bool HaveAccount(const CKeyID &keyId);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<CRegID, CKeyID> &mapKeyIds, const uint256 &hashBlock);
    bool BatchWrite(const vector<CAccount> &vAccounts);
    bool EraseAccountByKeyId(const CKeyID &keyId);
    bool SetKeyId(const CRegID &regId, const CKeyID &keyId);
    bool GetKeyId(const CRegID &regId, CKeyID &keyId);
    bool EraseKeyIdByRegId(const CRegID &regId);

    bool SaveAccount(const CAccount &account);
    std::tuple<uint64_t, uint64_t> TraverseAccount();
    int64_t GetDbCount() { return db.GetDbCount(); }
    Object ToJsonObj(dbk::PrefixType prefix = dbk::EMPTY);
};
*/
#endif  // PERSIST_ACCOUNTDB_H
