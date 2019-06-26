// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeeddb.h"

void CConsecutiveBlockPrice::AddUserPrice(const int blockHeight, const CRegID &regId, const uint64_t price) {
    mapBlockUserPrices[blockHeight][regId] = price;
}

void CConsecutiveBlockPrice::DeleteUserPrice(const int blockHeight) { mapBlockUserPrices.erase(blockHeight); }

bool CConsecutiveBlockPrice::ExistBlockUserPrice(const int blockHeight, const CRegID &regId) {
    if (mapBlockUserPrices.count(blockHeight) == 0)
        return false;

    return mapBlockUserPrices[blockHeight].count(regId);
}

bool CPricePointCache::AddBlockPricePointInBatch(const int blockHeight, const CRegID &regId,
                                                 const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps) {
        CConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPriceType()];
        if (cbp.ExistBlockUserPrice(blockHeight, regId))
            return false;

        cbp.AddUserPrice(blockHeight, regId, pp.GetPrice());
    }

    return true;
}

bool CPricePointCache::DeleteBlockPricePoint(const int blockHeight) {
    for (auto &item : mapCoinPricePointCache) {
        item.second.DeleteUserPrice(blockHeight);
    }

    return true;
}

void CPricePointCache::BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn) {
    for (const auto &item : mapCoinPricePointCacheIn) {
        const auto &mapBlockUserPrices = item.second.mapBlockUserPrices;
        for (const auto &userPrice : mapBlockUserPrices) {
            if (userPrice.second.empty()) {
                mapCoinPricePointCache[item.first /* CCoinPriceType */].mapBlockUserPrices.erase(
                    userPrice.first /* height */);
            } else {
                mapCoinPricePointCache[item.first /* CCoinPriceType */].mapBlockUserPrices[userPrice.first /* height */] =
                    userPrice.second;
            }
        }
    }
}

void CPricePointCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapCoinPricePointCache);
    mapCoinPricePointCache.clear();
}

bool CPricePointCache::GetBlockUserPrices(CCoinPriceType coinPriceType, set<int> &expired,
                                          BlockUserPriceMap &blockUserPrices) {
    const auto &iter = mapCoinPricePointCache.find(coinPriceType);
    if (iter != mapCoinPricePointCache.end()) {
        const auto &mapBlockUserPrices = iter->second.mapBlockUserPrices;
        for (const auto &item : mapBlockUserPrices) {
            if (item.second.empty()) {
                expired.insert(item.first);
            } else if (expired.count(item.first) || blockUserPrices.count(item.first)) {
                // TODO: log
                continue;
            } else {
                // Got a valid item.
                blockUserPrices[item.first] = item.second;
            }
        }
    }

    if (pBase != nullptr) {
        return pBase->GetBlockUserPrices(coinPriceType, expired, blockUserPrices);
    }

    return true;
}

bool CPricePointCache::GetBlockUserPrices(CCoinPriceType coinPriceType, BlockUserPriceMap &blockUserPrices) {
    set<int /* block height */> expired;
    if (!GetBlockUserPrices(coinPriceType, expired, blockUserPrices)) {
        // TODO: log
        return false;
    }

    return true;
}

uint64_t CPricePointCache::ComputeBlockMedianPrice(const int blockHeight, const CCoinPriceType &coinPriceType) {
    // 1. merge block user prices with base cache.
    BlockUserPriceMap blockUserPrices;
    if (!GetBlockUserPrices(coinPriceType, blockUserPrices) || blockUserPrices.empty()) {
        return 0;
    }

    // 2. compute block median price.
    return ComputeBlockMedianPrice(blockHeight, blockUserPrices);
}

uint64_t CPricePointCache::ComputeBlockMedianPrice(const int blockHeight, const BlockUserPriceMap &blockUserPrices) {
    // TODO: parameterize 11.
    assert(blockHeight >= 11);
    vector<uint64_t> prices;
    for (int height = blockHeight; height > blockHeight - 11; --height) {
        const auto &iter = blockUserPrices.find(height);
        if (iter != blockUserPrices.end()) {
            for (const auto &userPrice : iter->second) {
                prices.push_back(userPrice.second);
            }
        }
    }

    return ComputeMedianNumber(prices);
}

uint64_t CPricePointCache::ComputeMedianNumber(vector<uint64_t> &numbers) {
    unsigned int size = numbers.size();
    if (size < 2) {
        return size == 0 ? 0 : numbers[0];
    }
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size / 2 - 1] + numbers[size / 2]) / 2 : numbers[size / 2];
}