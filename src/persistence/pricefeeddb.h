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
    : price_feed_coin_cache(pDbAccess),
      medianPricesCache(pDbAccess),
      price_feeders_cache(pDbAccess) {};
public:
    bool Flush() {
        price_feed_coin_cache.Flush();
        medianPricesCache.Flush();
        price_feeders_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return  price_feed_coin_cache.GetCacheSize() +
                medianPricesCache.GetCacheSize() +
                price_feeders_cache.GetCacheSize();
    }
    void SetBaseViewPtr(CPriceFeedCache *pBaseIn) {
        price_feed_coin_cache.SetBase(&pBaseIn->price_feed_coin_cache);
        medianPricesCache.SetBase(&pBaseIn->medianPricesCache);
        price_feeders_cache.SetBase(&pBaseIn->price_feeders_cache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        price_feed_coin_cache.SetDbOpLogMap(pDbOpLogMapIn);
        medianPricesCache.SetDbOpLogMap(pDbOpLogMapIn);
        price_feeders_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        price_feed_coin_cache.RegisterUndoFunc(undoDataFuncMap);
        medianPricesCache.RegisterUndoFunc(undoDataFuncMap);
        price_feeders_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    bool AddFeedCoinPair(TokenSymbol feedCoin, TokenSymbol baseCoin) ;
    bool EraseFeedCoinPair(TokenSymbol feedCoin, TokenSymbol baseCoin) ;
    bool HasFeedCoinPair(TokenSymbol feedCoin,TokenSymbol baseCoin) ;
    bool GetFeedCoinPairs(set<pair<TokenSymbol,TokenSymbol>>& coinSet) ;

    bool CheckIsPriceFeeder(const CRegID &candidateRegId) ;
    bool SetPriceFeeders(const vector<CRegID> &governors) ;
    bool GetPriceFeeders(vector<CRegID>& priceFeeders) ;

    uint64_t GetMedianPrice(const CoinPricePair &coinPricePair) const;
    PriceMap GetMedianPrices() const;
    bool SetMedianPrices(const PriceMap &medianPrices);

public:

/*  CSimpleKVCache          prefixType          value           variable           */
/*  -------------------- --------------------  -------------   --------------------- */
    /////////// PriceFeedDB
    // [prefix] -> feed pair
    CSimpleKVCache< dbk::PRICE_FEED_COIN,      set<pair<TokenSymbol, TokenSymbol >>>     price_feed_coin_cache;
    // [prefix] -> median price map
    CSimpleKVCache< dbk::MEDIAN_PRICES,        PriceMap>     medianPricesCache;
    // [prefix] -> price feeders
    CSimpleKVCache< dbk::PRICE_FEEDERS,        vector<CRegID>>  price_feeders_cache ;

};

#endif  // PERSIST_PRICEFEED_H