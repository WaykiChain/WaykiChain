// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeeddb.h"

#include "main.h"
#include "tx/pricefeedtx.h"
#include "config/scoin.h"

void CConsecutiveBlockPrice::AddUserPrice(const int32_t blockHeight, const CRegID &regId, const uint64_t price) {
    mapBlockUserPrices[blockHeight][regId] = price;
}

void CConsecutiveBlockPrice::DeleteUserPrice(const int32_t blockHeight) {
    // Marked the value empty, the base cache will delete it when Flush() is called.
    mapBlockUserPrices[blockHeight].clear();
}

bool CConsecutiveBlockPrice::ExistBlockUserPrice(const int32_t blockHeight, const CRegID &regId) {
    if (mapBlockUserPrices.count(blockHeight) == 0)
        return false;

    return mapBlockUserPrices[blockHeight].count(regId);
}

bool CPricePointMemCache::AddBlockPricePointInBatch(const int32_t blockHeight, const CRegID &regId,
                                                 const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps) {
        CConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPriceType()];
        if (cbp.ExistBlockUserPrice(blockHeight, regId))
            return false;

        cbp.AddUserPrice(blockHeight, regId, pp.GetPrice());
    }

    return true;
}

bool CPricePointMemCache::AddBlockToCache(const CBlock &block) {
    // index[0]: block reward transaction
    // index[1]: block median price transaction
    // index[2 - n]: price feed transactions if existed.

    if (block.vptx.size() < 3) {
        return true;
    }

    // More than 3 transactions in the block.
    for (auto &pTx : block.vptx) {
        if (pTx->nTxType != PRICE_FEED_TX) {
            break;
        }

        CPriceFeedTx *priceFeedTx = (CPriceFeedTx *)pTx.get();
        AddBlockPricePointInBatch(block.GetHeight(), priceFeedTx->txUid.get<CRegID>(), priceFeedTx->pricePoints);
    }

    return true;
}

bool CPricePointMemCache::DeleteBlockPricePoint(const int32_t blockHeight) {
    for (auto &item : mapCoinPricePointCache) {
        item.second.DeleteUserPrice(blockHeight);
    }

    return true;
}

bool CPricePointMemCache::DeleteBlockFromCache(const CBlock &block) {
    return DeleteBlockPricePoint(block.GetHeight());
}

void CPricePointMemCache::BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn) {
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

void CPricePointMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapCoinPricePointCache);
    mapCoinPricePointCache.clear();
}

bool CPricePointMemCache::GetBlockUserPrices(CCoinPriceType coinPriceType, set<int32_t> &expired,
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

bool CPricePointMemCache::GetBlockUserPrices(CCoinPriceType coinPriceType, BlockUserPriceMap &blockUserPrices) {
    set<int32_t /* block height */> expired;
    if (!GetBlockUserPrices(coinPriceType, expired, blockUserPrices)) {
        // TODO: log
        return false;
    }

    return true;
}

uint64_t CPricePointMemCache::ComputeBlockMedianPrice(const int32_t blockHeight, const CCoinPriceType &coinPriceType) {
    // 1. merge block user prices with base cache.
    BlockUserPriceMap blockUserPrices;
    if (!GetBlockUserPrices(coinPriceType, blockUserPrices) || blockUserPrices.empty()) {
        return 0;
    }

    // 2. compute block median price.
    return ComputeBlockMedianPrice(blockHeight, blockUserPrices);
}

uint64_t CPricePointMemCache::ComputeBlockMedianPrice(const int32_t blockHeight, const BlockUserPriceMap &blockUserPrices) {
    assert(blockHeight >= kMedianPriceSlideWindowBlockCount);

    vector<uint64_t> prices;
    int32_t beginBlockHeight = blockHeight - kMedianPriceSlideWindowBlockCount;
    for (int32_t height = blockHeight; height > beginBlockHeight; --height) {
        const auto &iter = blockUserPrices.find(height);
        if (iter != blockUserPrices.end()) {
            for (const auto &userPrice : iter->second) {
                prices.push_back(userPrice.second);
            }
        }
    }

    return ComputeMedianNumber(prices);
}

uint64_t CPricePointMemCache::ComputeMedianNumber(vector<uint64_t> &numbers) {
    int32_t size = numbers.size();
    if (size < 2) {
        return size == 0 ? 0 : numbers[0];
    }
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size / 2 - 1] + numbers[size / 2]) / 2 : numbers[size / 2];
}

uint64_t CPricePointMemCache::GetBcoinMedianPrice() {
    return ComputeBlockMedianPrice(chainActive.Height(), CCoinPriceType(CoinType::WICC, PriceType::USD));
}

uint64_t CPricePointMemCache::GetFcoinMedianPrice() {
    return ComputeBlockMedianPrice(chainActive.Height(), CCoinPriceType(CoinType::WGRT, PriceType::USD));
}