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


struct AxcSwapCoinPair {

    TokenSymbol peer_token_symbol;
    TokenSymbol self_token_symbol;
    ChainType   peer_chain_type;

    AxcSwapCoinPair() {}
    AxcSwapCoinPair(TokenSymbol peerSymbol, TokenSymbol selfSymbol, ChainType peerType):
            peer_token_symbol(peerSymbol),self_token_symbol(selfSymbol), peer_chain_type(peerType){};

    Object ToJson() {
        Object o;
        o.push_back(Pair("peer_token_symbol", peer_token_symbol));
        o.push_back(Pair("self_token_symbol", self_token_symbol));

        string chainTypeString;
        auto itr = kChainTypeNameMap.find(peer_chain_type);
        if (itr != kChainTypeNameMap.end())
            o.push_back(Pair("peer_chain_type", itr->second));
        else
            o.push_back(Pair("peer_chain_type", peer_chain_type));
        return o;

    }
};

/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset$tokenSymbol -> asset>
typedef CCompositeKVCache< dbk::ASSET,         TokenSymbol,        CAsset>      DbAssetCache;

class CUserAssetsIterator: public CDbIterator<DbAssetCache> {
public:
    typedef CDbIterator<DbAssetCache> Base;
    using Base::Base;

    const CAsset& GetAsset() const {
        return GetValue();
    }
};


class CAssetDbCache {
public:
    CAssetDbCache() {}

    CAssetDbCache(CDBAccess *pDbAccess) : asset_cache(pDbAccess),
                                          axc_swap_coin_ps_cache(pDbAccess),
                                          axc_swap_coin_sp_cache(pDbAccess)  {
        assert(pDbAccess->GetDbNameType() == DBNameType::ASSET);
    };

    CAssetDbCache(CAssetDbCache *pBaseIn) : asset_cache(pBaseIn->asset_cache),
                                            axc_swap_coin_ps_cache(pBaseIn->axc_swap_coin_ps_cache),
                                            axc_swap_coin_sp_cache(pBaseIn->axc_swap_coin_sp_cache) {};

    ~CAssetDbCache() {}

public:
    bool GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset);
    bool SetAsset(const CAsset &asset);
    bool HasAsset(const TokenSymbol &tokenSymbol);

    bool CheckAsset(const TokenSymbol &symbol, CAsset &asset);
    bool CheckAsset(const TokenSymbol &symbol, const uint64_t permsSum = 0);

    bool Flush() {
        asset_cache.Flush();
        axc_swap_coin_ps_cache.Flush();
        axc_swap_coin_sp_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const { return asset_cache.GetCacheSize()
                                         + axc_swap_coin_ps_cache.GetCacheSize()
                                         + axc_swap_coin_sp_cache.GetCacheSize(); }

    void SetBaseViewPtr(CAssetDbCache *pBaseIn) {
        asset_cache.SetBase(&pBaseIn->asset_cache);
        axc_swap_coin_ps_cache.SetBase(&pBaseIn->axc_swap_coin_ps_cache);
        axc_swap_coin_sp_cache.SetBase(&pBaseIn->axc_swap_coin_sp_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        asset_cache.SetDbOpLogMap(pDbOpLogMapIn);
        axc_swap_coin_ps_cache.SetDbOpLogMap(pDbOpLogMapIn);
        axc_swap_coin_sp_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        asset_cache.RegisterUndoFunc(undoDataFuncMap);
        axc_swap_coin_sp_cache.RegisterUndoFunc(undoDataFuncMap);
        axc_swap_coin_ps_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CUserAssetsIterator> CreateUserAssetsIterator() {
        return make_shared<CUserAssetsIterator>(asset_cache);
    }

    bool GetAssetTokensByPerm( const AssetPermType& permType, set<TokenSymbol> &symbolSet);
public:
    // check functions for price feed
    bool CheckPriceFeedBaseSymbol(const TokenSymbol &baseSymbol);
    bool CheckPriceFeedQuoteSymbol(const TokenSymbol &quoteSymbol);
    // check functions for dex order
    bool CheckDexBaseSymbol(const TokenSymbol &baseSymbol);
    bool CheckDexQuoteSymbol(const TokenSymbol &baseSymbol);

    bool AddAxcSwapPair(TokenSymbol peerSymbol, TokenSymbol selfSymbol, ChainType peerType);
    bool EraseAxcSwapPair(TokenSymbol peerSymbol);
    bool HasAxcCoinPairByPeerSymbol(TokenSymbol peerSymbol);
    bool GetAxcCoinPairBySelfSymbol(TokenSymbol token, AxcSwapCoinPair& p);
    bool GetAxcCoinPairByPeerSymbol(TokenSymbol token, AxcSwapCoinPair& p);
public:
/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset_tokenSymbol -> asset>
    DbAssetCache   asset_cache;

    //peer_symbol -> pair<self_symbol, chainType>
    CCompositeKVCache<dbk::AXC_COIN_PEERTOSELF,         string,            pair<string, uint8_t>>  axc_swap_coin_ps_cache;

    //self_symbol -> pair<peer_symbol, chainType>
    CCompositeKVCache<dbk::AXC_COIN_SELFTOPEER,         string,            pair<string, uint8_t>>  axc_swap_coin_sp_cache;

};

#endif  // PERSIST_ASSETDB_H
