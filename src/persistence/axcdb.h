// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_AXCDB_H
#define PERSIST_AXCDB_H

#include <cstdint>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <tuple>
#include <algorithm>

#include "commons/serialize.h"
#include "dbaccess.h"
#include "entities/proposal.h"

using namespace std;

struct AxcSwapCoinPair {

    TokenSymbol peer_token_symbol;
    TokenSymbol self_token_symbol;
    ChainType   peer_chain_type;

    AxcSwapCoinPair() {}
    AxcSwapCoinPair(TokenSymbol peerSymbol, TokenSymbol selfSymbol, ChainType peerType):
                    peer_token_symbol(peerSymbol),self_token_symbol(selfSymbol), peer_chain_type(peerType){}
};

class CAxcDBCache {
public:
    CAxcDBCache() {}
    CAxcDBCache(CDBAccess *pDbAccess) : axc_swapin_cache(pDbAccess),
                                        axc_swap_coin_ps_cache(pDbAccess),
                                        axc_swap_coin_sp_cache(pDbAccess) {}
    CAxcDBCache(CAxcDBCache *pBaseIn) : axc_swapin_cache(pBaseIn->axc_swapin_cache),
                                        axc_swap_coin_ps_cache(pBaseIn->axc_swap_coin_ps_cache),
                                        axc_swap_coin_sp_cache(pBaseIn->axc_swap_coin_sp_cache){};

    bool Flush() {
        axc_swapin_cache.Flush();
        axc_swap_coin_ps_cache.Flush();
        axc_swap_coin_sp_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return axc_swapin_cache.GetCacheSize()
               + axc_swap_coin_ps_cache.GetCacheSize()
               + axc_swap_coin_sp_cache.GetCacheSize();
    }

    void SetBaseViewPtr(CAxcDBCache *pBaseIn) {
        axc_swapin_cache.SetBase(&pBaseIn->axc_swapin_cache);
        axc_swap_coin_ps_cache.SetBase(&pBaseIn->axc_swap_coin_ps_cache);
        axc_swap_coin_sp_cache.SetBase(&pBaseIn->axc_swap_coin_sp_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        axc_swapin_cache.SetDbOpLogMap(pDbOpLogMapIn);
        axc_swap_coin_ps_cache.SetDbOpLogMap(pDbOpLogMapIn);
        axc_swap_coin_sp_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool SetSwapInMintRecord(ChainType peerChainType, const string& peerChainTxId, const uint64_t mintAmount) {
        if (kChainTypeNameMap.find(peerChainType) == kChainTypeNameMap.end())
            return false;

        string key = kChainTypeNameMap.at(peerChainType) + peerChainTxId;
        return axc_swapin_cache.SetData(key, CVarIntValue(mintAmount));
    }

    bool GetSwapInMintRecord(ChainType peerChainType, const string& peerChainTxId, uint64_t &mintAmount) {
        if (kChainTypeNameMap.find(peerChainType) == kChainTypeNameMap.end())
            return false;

        string key = kChainTypeNameMap.at(peerChainType) + peerChainTxId;
        CVarIntValue<uint64_t> amount;
        if (!axc_swapin_cache.GetData(key, amount))
            return false;

        mintAmount = amount.get();
        return true;
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        axc_swapin_cache.RegisterUndoFunc(undoDataFuncMap);
        axc_swap_coin_sp_cache.RegisterUndoFunc(undoDataFuncMap);
        axc_swap_coin_ps_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    bool AddAxcSwapPair(TokenSymbol peerSymbol, TokenSymbol selfSymbol, ChainType peerType ) {
        return axc_swap_coin_ps_cache.SetData(peerSymbol, std::make_pair(selfSymbol, peerType)) &&
        axc_swap_coin_sp_cache.SetData(selfSymbol, std::make_pair(peerSymbol, peerType));
    }

    bool EraseAxcSwapPair(TokenSymbol peerSymbol ) {
        pair<string, uint8_t> data;
        if(!axc_swap_coin_ps_cache.GetData(peerSymbol, data)){

        }
        return axc_swap_coin_ps_cache.EraseData(peerSymbol) && axc_swap_coin_sp_cache.EraseData(data.first);
    }

    bool HasAxcCoinPairByPeerSymbol(TokenSymbol peerSymbol) {
        AxcSwapCoinPair p;
        return GetAxcCoinPairByPeerSymbol(peerSymbol, p);
    }

    bool GetAxcCoinPairBySelfSymbol(TokenSymbol token, AxcSwapCoinPair& p) {

        auto itr = kXChainSwapOutTokenMap.find(token);
        if(itr != kXChainSwapOutTokenMap.end() ){
            p = AxcSwapCoinPair(itr->second.first, itr->first, itr->second.second);
            return true;
        }

        pair<string, uint8_t> data;
        if(axc_swap_coin_sp_cache.GetData(token, data)){
            p = AxcSwapCoinPair(token, TokenSymbol(data.first), ChainType(data.second));
            return true;
        }

        return false;
    }

    bool GetAxcCoinPairByPeerSymbol(TokenSymbol token, AxcSwapCoinPair& p) {

        auto itr = kXChainSwapInTokenMap.find(token);
        if(itr != kXChainSwapInTokenMap.end() ){
            p = AxcSwapCoinPair(itr->first, itr->second.first,  itr->second.second);
            return true;
        }

        pair<string, uint8_t> data;
        if(axc_swap_coin_ps_cache.GetData(token, data)){
            p = AxcSwapCoinPair(TokenSymbol(data.first),  token, ChainType(data.second));
            return true;
        }

        return false;
    }



public:
/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */


/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
//swap_in$peer_chain_txid -> coin_amount_to_mint
CCompositeKVCache<dbk::AXC_SWAP_IN,           string,                  CVarIntValue<uint64_t> > axc_swapin_cache;

//peer_symbol -> pair<self_symbol, chainType>
CCompositeKVCache<dbk::AXC_COIN_PEERTOSELF,         string,            pair<string, uint8_t>>  axc_swap_coin_ps_cache;

//self_symbol -> pair<peer_symbol, chainType>
CCompositeKVCache<dbk::AXC_COIN_SELFTOPEER,         string,            pair<string, uint8_t>>  axc_swap_coin_sp_cache;


};

#endif //PERSIST_AXCDB_H