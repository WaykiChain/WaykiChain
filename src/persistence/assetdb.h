// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_ASSETDB_H
#define PERSIST_ASSETDB_H

#include "commons/arith_uint256.h"
#include "dbconf.h"
#include "dbaccess.h"
#include "dbiterator.h"
#include "entities/asset.h"
#include "leveldbwrapper.h"

#include <map>
#include <string>
#include <utility>
#include <vector>


/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset$tokenSymbol -> asset>
typedef CCompositeKVCache< dbk::ASSET,         TokenSymbol,        CAsset>      DbAssetCache;
    // [prefix]{$perm}{$asset_symbol} --> $assetStatus
typedef CCompositeKVCache< dbk::PERM_ASSETS,  pair<CFixedUInt64, TokenSymbol>,  uint8_t>  PermAssetsCache;

class CUserAssetsIterator: public CDbIterator<DbAssetCache> {
public:
    typedef CDbIterator<DbAssetCache> Base;
    using Base::Base;

    const CAsset& GetAsset() const {
        return GetValue();
    }
};

using CPermAssetsIterator = CDBPrefixIterator<PermAssetsCache, CFixedUInt64>;

class CAssetDbCache {
public:
    CAssetDbCache() {}

    CAssetDbCache(CDBAccess *pDbAccess) : assetCache(pDbAccess), perm_assets_cache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ASSET);
    }

    ~CAssetDbCache() {}

public:
    bool GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset);
    bool SetAsset(const CAsset &asset);
    bool HasAsset(const TokenSymbol &tokenSymbol);

    bool CheckAsset(const TokenSymbol &symbol, uint64_t permsSum = 0);

    bool SetAssetPerms(const CAsset &oldAsset, const CAsset &newAsset);

    bool Flush() {
        assetCache.Flush();
        perm_assets_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const { return assetCache.GetCacheSize() + perm_assets_cache.GetCacheSize(); }

    void SetBaseViewPtr(CAssetDbCache *pBaseIn) {
        assetCache.SetBase(&pBaseIn->assetCache);
        perm_assets_cache.SetBase(&pBaseIn->perm_assets_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        assetCache.SetDbOpLogMap(pDbOpLogMapIn);
        perm_assets_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        assetCache.RegisterUndoFunc(undoDataFuncMap);
        perm_assets_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CUserAssetsIterator> CreateUserAssetsIterator() {
        return make_shared<CUserAssetsIterator>(assetCache);
    }

    void GetDexQuoteSymbolSet(set<TokenSymbol> &symbolSet);
public:
    // check functions for price feed
    bool CheckPriceFeedBaseSymbol(const TokenSymbol &baseSymbol);
    bool CheckPriceFeedQuoteSymbol(const TokenSymbol &quoteSymbol);
    // check functions for dex order
    bool CheckDexBaseSymbol(const TokenSymbol &baseSymbol);
    bool CheckDexQuoteSymbol(const TokenSymbol &baseSymbol);
public:
/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset_tokenSymbol -> asset>
    DbAssetCache   assetCache;
    // [prefix]{$perm}{$asset_symbol} --> $assetStatus
    PermAssetsCache      perm_assets_cache;
};

#endif  // PERSIST_ASSETDB_H
