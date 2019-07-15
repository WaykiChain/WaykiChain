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

class CAssetDBCache {
public:


public:
    CAssetDBCache() {}

    CAssetDBCache(CDBAccess *pDbAccess):
        blockHashCache(pDbAccess),
        keyId2AccountCache(pDbAccess),
        regId2KeyIdCache(pDbAccess),
        nickId2KeyIdCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ACCOUNT);
    }

    CAssetDBCache(CAccountDBCache *pBase):
        blockHashCache(pBase->blockHashCache),
        keyId2AccountCache(pBase->keyId2AccountCache),
        regId2KeyIdCache(pBase->regId2KeyIdCache),
        nickId2KeyIdCache(pBase->nickId2KeyIdCache) {}

    ~CAssetDBCache() {}

    bool Flush();

private:
/*  CDBScalarValueCache     prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    // <asset_symbole -> Asset>
    CDBScalarValueCache< dbk::ASSET,     CAsset>        assetCache;

/*  CDBScalarValueCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <KeyID -> Account>
    CDBMultiValueCache< dbk::KEYID_ACCOUNT,        CKeyID,       CAccount >       keyId2AccountCache;
    // <RegID str -> KeyID>
    CDBMultiValueCache< dbk::REGID_KEYID,          string,       CKeyID >         regId2KeyIdCache;
    // <NickID -> KeyID>
    CDBMultiValueCache< dbk::NICKID_KEYID,         CNickID,      CKeyID>          nickId2KeyIdCache;
};

#endif  // PERSIST_ACCOUNTDB_H
