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
    CAccountDBCache() {}

    CAccountDBCache(CDBAccess *pDbAccess):
        blockHashCache(pDbAccess),
        regId2KeyIdCache(pDbAccess),
        nickId2KeyIdCache(pDbAccess),
        accountCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ACCOUNT);
    }

    CAccountDBCache(CAccountDBCache *pBase):
        blockHashCache(pBase->blockHashCache),
        regId2KeyIdCache(pBase->regId2KeyIdCache),
        nickId2KeyIdCache(pBase->nickId2KeyIdCache),
        accountCache(pBase->accountCache) {}

    ~CAccountDBCache() {}

public:
    bool GetFcoinGenesisAccount(CAccount &fcoinGenesisAccount) const;

    bool GetAccount(const CKeyID &keyId,    CAccount &account) const;
    bool GetAccount(const CRegID &regId,    CAccount &account) const;
    bool GetAccount(const CUserID &uid,     CAccount &account) const;

    bool SetAccount(const CKeyID &keyId,    const CAccount &account);
    bool SetAccount(const CRegID &regId,    const CAccount &account);
    bool SetAccount(const CUserID &uid,     const CAccount &account);
    bool SaveAccount(const CAccount &account);

    bool HaveAccount(const CKeyID &keyId) const;
    bool HaveAccount(const CUserID &userId) const;

    bool EraseAccount(const CKeyID &keyId);
    bool EraseAccount(const CUserID &userId);

    uint256 GetBestBlock() const;
    bool SetBestBlock(const uint256 &blockHash);

    bool BatchWrite(const map<CKeyID, CAccount> &mapAccounts,
                    const map<CRegID, CKeyID> &mapKeyIds,
                    const uint256 &blockHash);

    bool BatchWrite(const vector<CAccount> &accounts);

    bool SetKeyId(const CRegID &regId,  const CKeyID &keyId);
    bool SetKeyId(const CUserID &uid,   const CKeyID &keyId);
    bool GetKeyId(const CRegID &regId,  CKeyID &keyId) const;
    bool GetKeyId(const CUserID &uid,   CKeyID &keyId) const;

    bool EraseKeyId(const CRegID &regId);
    bool EraseKeyId(const CUserID &userId);

    std::tuple<uint64_t, uint64_t> TraverseAccount();

    bool GetUserId(const string &addr, CUserID &userId) const;
    bool GetRegId(const CKeyID &keyId, CRegID &regId) const;
    bool GetRegId(const CUserID &userId, CRegID &regId) const;

    uint32_t GetCacheSize() const;
    Object ToJsonObj(dbk::PrefixType prefix = dbk::EMPTY);

    void SetBaseViewPtr(CAccountDBCache *pBaseIn) {
        blockHashCache.SetBase(&pBaseIn->blockHashCache);
        accountCache.SetBase(&pBaseIn->accountCache);
        regId2KeyIdCache.SetBase(&pBaseIn->regId2KeyIdCache);
        nickId2KeyIdCache.SetBase(&pBaseIn->nickId2KeyIdCache);
    };

    uint64_t GetAccountFreeAmount(const CKeyID &keyId, const TokenSymbol &tokenSymbol);

    bool Flush();

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        blockHashCache.SetDbOpLogMap(pDbOpLogMapIn);
        accountCache.SetDbOpLogMap(pDbOpLogMapIn);
        regId2KeyIdCache.SetDbOpLogMap(pDbOpLogMapIn);
        nickId2KeyIdCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return blockHashCache.UndoDatas() &&
               accountCache.UndoDatas() &&
               regId2KeyIdCache.UndoDatas() &&
               nickId2KeyIdCache.UndoDatas();
    }
private:
    //TODO: move it to other dbcache file
/*  CSimpleKVCache     prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    // best blockHash
    CSimpleKVCache< dbk::BEST_BLOCKHASH,      uint256>        blockHashCache;



/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <prefix$RegID -> KeyID>
    CCompositeKVCache< dbk::REGID_KEYID,          string,       CKeyID >         regId2KeyIdCache;
    // <prefix$NickID -> KeyID>
    CCompositeKVCache< dbk::NICKID_KEYID,         CNickID,      CKeyID>          nickId2KeyIdCache;
    // <prefix$KeyID -> Account>
    CCompositeKVCache< dbk::KEYID_ACCOUNT,        CKeyID,       CAccount>        accountCache;

};

#endif  // PERSIST_ACCOUNTDB_H
