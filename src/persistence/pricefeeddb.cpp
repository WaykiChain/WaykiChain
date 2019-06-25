// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeeddb.h"

void CConsecutiveBlockPrice::AddUserPrice(const int blockHeight, const CRegID &regId, const uint64_t price) {
    mapBlockUserPrices[blockHeight][regId.ToRawString()] = price;
}

void CConsecutiveBlockPrice::DeleteUserPrice(const int blockHeight) {
    mapBlockUserPrices.erase(blockHeight);
}

uint64_t CConsecutiveBlockPrice::ComputeBlockMedianPrice(const int blockHeight) {
    // TODO: parameterize 11.
    assert(blockHeight >= 11);
    vector<uint64_t> prices;
    for (int height = blockHeight; height > blockHeight - 11; -- height) {
        if (mapBlockUserPrices.count(height) != 0) {
            for (auto userPrice : mapBlockUserPrices[height]) {
                prices.push_back(userPrice.second);
            }
        }
    }

    return ComputeMedianNumber(prices);
}

bool CConsecutiveBlockPrice::ExistBlockUserPrice(const int blockHeight, const CRegID &regId) {
    if (mapBlockUserPrices.count(blockHeight) == 0)
        return false;

    return mapBlockUserPrices[blockHeight].count(regId.ToRawString());
}

uint64_t CConsecutiveBlockPrice::ComputeMedianNumber(vector<uint64_t> &numbers) {
    unsigned int size = numbers.size();
    if (size < 2) {
        return size == 0 ? 0 : numbers[0];
    }
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size / 2 - 1] + numbers[size / 2]) / 2 : numbers[size / 2];
}

bool CPricePointCache::AddBlockPricePointInBatch(const int blockHeight, const CRegID &regId,
                                                 const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps) {
        CConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPriceType().ToString()];
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

void CPricePointCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapCoinPricePointCache);
    mapCoinPricePointCache.clear();
}

void CPricePointCache::ComputeBlockMedianPrice(const int blockHeight) {
    bcoinMedianPrice = ComputeBlockMedianPrice(blockHeight, CCoinPriceType(CoinType::WICC, PriceType::USD));
    fcoinMedianPrice = ComputeBlockMedianPrice(blockHeight, CCoinPriceType(CoinType::MICC, PriceType::USD));
}

uint64_t CPricePointCache::ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType) {
    return mapCoinPricePointCache.count(coinPriceType.ToString())
               ? mapCoinPricePointCache[coinPriceType.ToString()].ComputeBlockMedianPrice(blockHeight)
               : 0;
}

void CPricePointCache::BatchWrite(const map<string, CConsecutiveBlockPrice> &mapCoinPricePointCacheIn) {
    // TODO:
}