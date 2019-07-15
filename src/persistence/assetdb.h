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
#include "accounts/asset.h"
#include "dbconf.h"
#include "dbaccess.h"

class uint256;
class CKeyID;

class CAssetDBCache {
public:


public:
    CAssetDBCache() {}

    CAssetDBCache(CDBAccess *pDbAccess) : assetCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ASSET);
    }

    ~CAssetDBCache() {}

    bool Flush();

private:
/*  CDBScalarValueCache     prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    // <asset_symbole -> Asset>
    CDBScalarValueCache< dbk::ASSET,     CAsset>        assetCache;
};

#endif  // PERSIST_ACCOUNTDB_H
