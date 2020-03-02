// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_PRICEFEED_H
#define PERSIST_PRICEFEED_H

#include "block.h"
#include "commons/serialize.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "entities/id.h"
#include "entities/price.h"
#include "tx/tx.h"
#include "persistence/dbaccess.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

class CConsecutiveBlockPrice;

typedef map<int32_t /* block height */, map<CRegID, uint64_t /* price */>> BlockUserPriceMap;
typedef map<CoinPricePair, CConsecutiveBlockPrice> CoinPricePointMap;

// Price Points in 11 consecutive blocks
class CConsecutiveBlockPrice {
public:
    void AddUserPrice(const int32_t blockHeight, const CRegID &regId, const uint64_t price);
    // delete user price by specific block height.
    void DeleteUserPrice(const int32_t blockHeight);
    bool ExistBlockUserPrice(const int32_t blockHeight, const CRegID &regId);

public:
    BlockUserPriceMap mapBlockUserPrices;
};

class CPricePointMemCache {
public:
    CPricePointMemCache() : pBase(nullptr) {}
    CPricePointMemCache(CPricePointMemCache *pBaseIn)
        : pBase(pBaseIn) {}

public:
    bool AddPrice(const int32_t blockHeight, const CRegID &regId, const vector<CPricePoint> &pps);
    bool AddPriceByBlock(const CBlock &block);
    // delete block price point by specific block height.
    bool DeleteBlockFromCache(const CBlock &block);

    bool CalcBlockMedianPrices(CCacheWrapper &cw, const int32_t blockHeight, PriceMap &medianPrices);

    void SetBaseViewPtr(CPricePointMemCache *pBaseIn);
    void Flush();

private:
    uint64_t GetMedianPrice(const int32_t blockHeight, const uint64_t slideWindow, const CoinPricePair &coinPricePair);

    bool DeleteBlockPricePoint(const int32_t blockHeight);

    bool ExistBlockUserPrice(const int32_t blockHeight, const CRegID &regId, const CoinPricePair &coinPricePair);

    void BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn);

    bool GetBlockUserPrices(const CoinPricePair &coinPricePair, set<int32_t> &expired, BlockUserPriceMap &blockUserPrices);
    bool GetBlockUserPrices(const CoinPricePair &coinPricePair, BlockUserPriceMap &blockUserPrices);

    uint64_t ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindow,
                                     const CoinPricePair &coinPricePair);
    uint64_t ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindow,
                                     const BlockUserPriceMap &blockUserPrices);
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);

private:
    CoinPricePointMap mapCoinPricePointCache;  // coinPriceType -> consecutiveBlockPrice
    CPricePointMemCache *pBase;
    PriceMap latest_median_prices;

};

class CPriceFeedCache {
public:
    CPriceFeedCache() {}
    CPriceFeedCache(CDBAccess *pDbAccess)
    : price_feed_coin_cache(pDbAccess) {};


public:


    bool Flush() {
        price_feed_coin_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return price_feed_coin_cache.GetCacheSize() ;
    }
    void SetBaseViewPtr(CPriceFeedCache *pBaseIn) {
        price_feed_coin_cache.SetBase(&pBaseIn->price_feed_coin_cache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        price_feed_coin_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        price_feed_coin_cache.RegisterUndoFunc(undoDataFuncMap);

    }




    bool AddFeedCoinPair(TokenSymbol feedCoin, TokenSymbol baseCoin) {

        if(feedCoin == SYMB::WICC && baseCoin == SYMB::USD)
            return true ;

        set<pair<TokenSymbol,TokenSymbol>> coinPairs ;
        price_feed_coin_cache.GetData(coinPairs);
        if(coinPairs.count(make_pair(feedCoin, baseCoin)) != 0 )
            return true ;
        coinPairs.insert(make_pair(feedCoin,baseCoin)) ;
        return price_feed_coin_cache.SetData(coinPairs);
    }

    bool EraseFeedCoinPair(TokenSymbol feedCoin, TokenSymbol baseCoin) {

        if(feedCoin == SYMB::WICC && baseCoin == SYMB::USD)
            return true ;

        auto coinPair = std::make_pair(feedCoin, baseCoin);
        set<pair<TokenSymbol,TokenSymbol>> coins ;
        price_feed_coin_cache.GetData(coins);
        if(coins.count(coinPair) == 0 )
            return true ;
        coins.erase(coinPair) ;
        return price_feed_coin_cache.SetData(coins);
    }

    bool HaveFeedCoinPair(TokenSymbol feedCoin,TokenSymbol baseCoin) {
        if(feedCoin == SYMB::WICC && baseCoin == SYMB::USD)
            return true ;

        set<pair<TokenSymbol, TokenSymbol>> coins ;
        price_feed_coin_cache.GetData(coins);
        return (coins.count(make_pair(feedCoin,baseCoin)) != 0 ) ;
    }

    bool GetFeedCoinPairs(set<pair<TokenSymbol,TokenSymbol>>& coinSet) {
        price_feed_coin_cache.GetData(coinSet) ;
        coinSet.insert(make_pair(SYMB::WICC, SYMB::USD));
        return true ;
    }


public:
/*       type               prefixType                      key                        value                variable             */
/*  ----------------   -----------------------------  ---------------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CSimpleKVCache< dbk::PRICE_FEED_COIN,          set<pair<TokenSymbol, TokenSymbol >>>     price_feed_coin_cache;

};

#endif  // PERSIST_PRICEFEED_H