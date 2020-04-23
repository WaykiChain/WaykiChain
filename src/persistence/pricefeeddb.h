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

class CSysParamDBCache; // need to read slide window param
class CConsecutiveBlockPrice;

typedef map<HeightType /* block height */, map<CRegID, uint64_t /* price */>> BlockUserPriceMap;
typedef map<PriceCoinPair, CConsecutiveBlockPrice> CoinPricePointMap;

// Price Points in 11 consecutive blocks
class CConsecutiveBlockPrice {
public:
    void AddUserPrice(const HeightType blockHeight, const CRegID &regId, const uint64_t price);
    // delete user price by specific block height.
    void DeleteUserPrice(const HeightType blockHeight);
    bool ExistBlockUserPrice(const HeightType blockHeight, const CRegID &regId);

public:
    BlockUserPriceMap mapBlockUserPrices;
};

class CPricePointMemCache {
public:
    CPricePointMemCache() : pBase(nullptr) {}
    CPricePointMemCache(CPricePointMemCache *pBaseIn)
        : pBase(pBaseIn) {}

public:
    bool ReleadBlocks(CSysParamDBCache &sysParamCache, CBlockIndex *pTipBlockIdx);
    bool PushBlock(CSysParamDBCache &sysParamCache, CBlockIndex *pTipBlockIdx);
    bool UndoBlock(CSysParamDBCache &sysParamCache, CBlockIndex *pTipBlockIdx);
    bool AddPrice(const HeightType blockHeight, const CRegID &regId, const vector<CPricePoint> &pps);

    bool CalcMedianPrices(CCacheWrapper &cw, const HeightType blockHeight, PriceMap &medianPrices);
    bool CalcMedianPriceDetails(CCacheWrapper &cw, const HeightType blockHeight, PriceDetailMap &medianPrices);

    void SetBaseViewPtr(CPricePointMemCache *pBaseIn);
    void Flush();

private:
    CMedianPriceDetail GetMedianPrice(const HeightType blockHeight, const uint64_t slideWindow, const PriceCoinPair &coinPricePair);

    bool AddPriceByBlock(const CBlock &block);
    // delete block price point by specific block height.
    bool DeleteBlockFromCache(const CBlock &block);

    bool DeleteBlockPricePoint(const HeightType blockHeight);

    bool ExistBlockUserPrice(const HeightType blockHeight, const CRegID &regId, const PriceCoinPair &coinPricePair);

    void BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn);

    bool GetBlockUserPrices(const PriceCoinPair &coinPricePair, set<HeightType> &expired, BlockUserPriceMap &blockUserPrices);
    bool GetBlockUserPrices(const PriceCoinPair &coinPricePair, BlockUserPriceMap &blockUserPrices);

    CMedianPriceDetail ComputeBlockMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                     const PriceCoinPair &coinPricePair);
    CMedianPriceDetail ComputeBlockMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                     const BlockUserPriceMap &blockUserPrices);
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);

private:
    CoinPricePointMap mapCoinPricePointCache;  // coinPriceType -> consecutiveBlockPrice
    CPricePointMemCache *pBase;
    PriceDetailMap latest_median_prices;

};

class CPriceFeedCache {
public:
    CPriceFeedCache() {}
    CPriceFeedCache(CDBAccess *pDbAccess)
    : price_feed_coin_pairs_cache(pDbAccess),
      median_price_cache(pDbAccess) {};
public:
    bool Flush() {
        price_feed_coin_pairs_cache.Flush();
        median_price_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return  price_feed_coin_pairs_cache.GetCacheSize() +
                median_price_cache.GetCacheSize();
    }
    void SetBaseViewPtr(CPriceFeedCache *pBaseIn) {
        price_feed_coin_pairs_cache.SetBase(&pBaseIn->price_feed_coin_pairs_cache);
        median_price_cache.SetBase(&pBaseIn->median_price_cache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        price_feed_coin_pairs_cache.SetDbOpLogMap(pDbOpLogMapIn);
        median_price_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        price_feed_coin_pairs_cache.RegisterUndoFunc(undoDataFuncMap);
        median_price_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    bool AddFeedCoinPair(const PriceCoinPair &coinPair);
    bool EraseFeedCoinPair(const PriceCoinPair &coinPair);
    bool HasFeedCoinPair(const PriceCoinPair &coinPair);
    bool GetFeedCoinPairs(set<PriceCoinPair>& coinPairSet);

    bool CheckIsPriceFeeder(const CRegID &candidateRegId);
    bool SetPriceFeeders(const vector<CRegID> &governors);
    bool GetPriceFeeders(vector<CRegID>& priceFeeders);

    uint64_t GetMedianPrice(const PriceCoinPair &coinPricePair);
    bool GetMedianPriceDetail(const PriceCoinPair &coinPricePair, CMedianPriceDetail &priceDetail);
    PriceDetailMap GetMedianPrices() const;
    bool SetMedianPrices(const PriceDetailMap &medianPrices);

public:

/*  CSimpleKVCache          prefixType          value           variable           */
/*  -------------------- --------------------  -------------   --------------------- */
    /////////// PriceFeedDB
    // [prefix] -> feed pair
    CSimpleKVCache< dbk::PRICE_FEED_COIN_PAIRS, set<PriceCoinPair>>   price_feed_coin_pairs_cache;
    // [prefix] -> median price map
    CSimpleKVCache< dbk::MEDIAN_PRICES,     PriceDetailMap>     median_price_cache;
};

#endif  // PERSIST_PRICEFEED_H