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
#include "entities/account.h"
#include "dbconf.h"
#include "dbaccess.h"

class uint256;
class CKeyID;

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
        accountCache(pDbAccess),
        regId2KeyIdCache(pDbAccess),
        nickId2KeyIdCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ACCOUNT);
    }

    CAccountDBCache(CAccountDBCache *pBase):
        blockHashCache(pBase->blockHashCache),
        accountCache(pBase->accountCache),
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
    uint32_t GetCacheSize() const;
    Object ToJsonObj(dbk::PrefixType prefix = dbk::EMPTY);

    void SetBaseViewPtr(CAccountDBCache *pBaseIn) {
        blockHashCache.SetBase(&pBaseIn->blockHashCache);
        accountCache.SetBase(&pBaseIn->accountCache);
        regId2KeyIdCache.SetBase(&pBaseIn->regId2KeyIdCache);
        nickId2KeyIdCache.SetBase(&pBaseIn->nickId2KeyIdCache);
    };

private:
/*  CDBScalarValueCache     prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    // best blockHash
    CDBScalarValueCache< dbk::BEST_BLOCKHASH,      uint256>        blockHashCache;

/*  CDBScalarValueCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <prefix$KeyID -> Account>
    CDBMultiValueCache< dbk::KEYID_ACCOUNT,        CKeyID,       CAccount>       accountCache;
    // <prefix$KeyID -> tokens>
    CDBMultiValueCache< dbk::KEYID_ACCOUNT_TOKEN,  CKeyID,       std::map<TokenSymbol, CAccountToken>> accountTokenCache;
    // <RegID str -> KeyID>
    CDBMultiValueCache< dbk::REGID_KEYID,          CRegID,       CKeyID >         regId2KeyIdCache;
    // <prefix$NickID -> KeyID>
    CDBMultiValueCache< dbk::NICKID_KEYID,         CNickID,      CKeyID>          nickId2KeyIdCache;
};

#endif  // PERSIST_ACCOUNTDB_H
