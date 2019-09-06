// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_ASSETDB_H
#define PERSIST_ASSETDB_H

#include "entities/asset.h"
#include "leveldbwrapper.h"
#include "entities/asset.h"
#include "entities/asset.h"
#include "commons/arith_uint256.h"
#include "dbconf.h"
#include "dbaccess.h"
#include "dbiterator.h"

#include <map>
#include <string>
#include <utility>
#include <vector>


/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset_tokenSymbol -> asset>
typedef CCompositeKVCache< dbk::ASSET,         TokenSymbol,        CAsset>      DBAssetCache;


typedef CDBListGetter<DBAssetCache> CUserAssetsGetter;

class CAssetDBCache {
public:
    CAssetDBCache() {}

    CAssetDBCache(CDBAccess *pDbAccess) : assetCache(pDbAccess), assetTradingPairCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ASSET);
    }

    ~CAssetDBCache() {}

public:
    bool GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset);
    bool HaveAsset(const TokenSymbol &tokenSymbol);
    bool SaveAsset(const CAsset &asset);
    bool ExistAssetSymbol(const TokenSymbol &tokenSymbol);

    bool AddAssetTradingPair(const CAssetTradingPair &assetTradingPair);
    bool ExistAssetTradingPair(const CAssetTradingPair &TradingPair);
    bool EraseAssetTradingPair(const CAssetTradingPair &assetTradingPair);

    bool Flush();

    void SetBaseViewPtr(CAssetDBCache *pBaseIn) {
        assetCache.SetBase(&pBaseIn->assetCache);
        assetTradingPairCache.SetBase(&pBaseIn->assetTradingPairCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        assetCache.SetDbOpLogMap(pDbOpLogMapIn);
        assetTradingPairCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return assetCache.UndoDatas() &&
               assetTradingPairCache.UndoDatas();
    }

    shared_ptr<CUserAssetsGetter> CreateUserAssetsGetter() {
        return make_shared<CUserAssetsGetter>(assetCache);
    }
private:
/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset_tokenSymbol -> asset>
    DBAssetCache   assetCache;
    // <asset_trading_pair -> 1>
    CCompositeKVCache< dbk::ASSET_TRADING_PAIR, CAssetTradingPair,  uint8_t>        assetTradingPairCache;
};

#endif  // PERSIST_ASSETDB_H
