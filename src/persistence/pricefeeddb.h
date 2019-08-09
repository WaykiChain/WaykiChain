// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_PRICEFEED_H
#define PERSIST_PRICEFEED_H

#include "entities/account.h"
#include "entities/id.h"
#include "block.h"
#include "commons/serialize.h"
#include "tx/tx.h"

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
    map<CoinPricePair, uint64_t> latestBlockMedianPricePoints;

public:
    CPricePointMemCache() : pBase(nullptr) {}
    CPricePointMemCache(CPricePointMemCache *pBaseIn)
        : latestBlockMedianPricePoints(pBase->latestBlockMedianPricePoints), pBase(pBaseIn) {}

public:
    void SetLatestBlockMedianPricePoints(const map<CoinPricePair, uint64_t> &latestBlockMedianPricePoints);
    bool AddBlockPricePointInBatch(const int32_t blockHeight, const CRegID &regId, const vector<CPricePoint> &pps);
    bool AddBlockToCache(const CBlock &block);
    // delete block price point by specific block height.
    bool DeleteBlockPricePoint(const int32_t blockHeight);
    bool DeleteBlockFromCache(const CBlock &block);

    uint64_t GetMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                            const CoinPricePair &coinPricePair);
    uint64_t GetBcoinMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount);
    uint64_t GetFcoinMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount);
    bool GetBlockMedianPricePoints(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                   map<CoinPricePair, uint64_t> &mapMedianPricePoints);

    void SetBaseViewPtr(CPricePointMemCache *pBaseIn);
    void Flush();

private:
    bool ExistBlockUserPrice(const int32_t blockHeight, const CRegID &regId, const CoinPricePair &coinPricePair);

    void BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn);

    bool GetBlockUserPrices(const CoinPricePair &coinPricePair, set<int32_t> &expired, BlockUserPriceMap &blockUserPrices);
    bool GetBlockUserPrices(const CoinPricePair &coinPricePair, BlockUserPriceMap &blockUserPrices);

    uint64_t ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                     const CoinPricePair &coinPricePair);
    uint64_t ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                     const BlockUserPriceMap &blockUserPrices);
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);

private:
    CoinPricePointMap mapCoinPricePointCache;  // coinPriceType -> consecutiveBlockPrice
    CPricePointMemCache *pBase;
};

#endif  // PERSIST_PRICEFEED_H