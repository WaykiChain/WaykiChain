// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_ASSETDB_H
#define PERSIST_ASSETDB_H

#include "commons/arith_uint256.h"
#include "dbconf.h"
#include "dbcache.h"
#include "dbiterator.h"
#include "entities/asset.h"
#include "entities/proposal.h"
#include "leveldbwrapper.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CAxcSwapPairStore {
public:
    ProposalOperateType status = ProposalOperateType::NULL_PROPOSAL_OP;
    TokenSymbol         peer_symbol;
    ChainType           peer_chain_type = ChainType::NULL_CHAIN_TYPE;

    IMPLEMENT_SERIALIZE(
            READWRITE_ENUM(status, uint8_t);
            READWRITE(peer_symbol);
            READWRITE_ENUM(peer_chain_type, uint8_t);
    )

    string GetSelfSymbol() const {
        return "m" + peer_symbol;
    }

    bool IsEmpty() const {
        return status == 0;
    }

    void SetEmpty() {
        *this = CAxcSwapPairStore();
    }

    string ToString() const {
        return strprintf("status=%d, peer_symbol=%s, self_symbol=%s, ",
            kProposalOperateTypeHelper.GetName(status), peer_symbol, GetSelfSymbol(), kChainTypeHelper.GetName(peer_chain_type));
    }

    Object ToJson() {
        Object obj;
        obj.push_back(Pair("status", kProposalOperateTypeHelper.GetName(status)));
        obj.push_back(Pair("peer_symbol", peer_symbol));
        obj.push_back(Pair("self_symbol", GetSelfSymbol()));
        obj.push_back(Pair("peer_chain_type", kChainTypeHelper.GetName(peer_chain_type)));
        return obj;

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

    bool SetAxcSwapPair(const CAxcSwapPairStore &swapPair);
    bool HasAxcCoinPairByPeerSymbol(TokenSymbol peerSymbol);
    bool GetAxcCoinPairBySelfSymbol(TokenSymbol selfSymbol, CAxcSwapPairStore& swapPair);
    bool GetAxcCoinPairByPeerSymbol(TokenSymbol peerSymbol, CAxcSwapPairStore& swapPair);

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
    bool CheckPriceFeedQuoteSymbol(const TokenSymbol &quoteSymbol);
    // check functions for dex order
    bool CheckDexBaseSymbol(const TokenSymbol &baseSymbol);
    bool CheckDexQuoteSymbol(const TokenSymbol &baseSymbol);

public:
/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset_tokenSymbol -> asset>
    DbAssetCache   asset_cache;

    //peer_symbol -> CAxcCoinPairStore
    CCompositeKVCache<dbk::AXC_COIN_PEERTOSELF,         string,            CAxcSwapPairStore>  axc_swap_coin_ps_cache;

    //self_symbol -> CAxcCoinPairStore
    CCompositeKVCache<dbk::AXC_COIN_SELFTOPEER,         string,            CAxcSwapPairStore>  axc_swap_coin_sp_cache;

};

#endif  // PERSIST_ASSETDB_H
