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

void CPricePointMemCache::SetLatestBlockMedianPricePoints(
    const map<CoinPricePair, uint64_t> &latestBlockMedianPricePointsIn) {
    latestBlockMedianPricePoints = latestBlockMedianPricePointsIn;
    string prices;
    for (const auto &item: latestBlockMedianPricePointsIn) {
        prices += strprintf("{%s/%s -> %llu}", std::get<0>(item.first), std::get<1>(item.first), item.second);
    }

    LogPrint("PRICEFEED", "CPricePointMemCache::SetLatestBlockMedianPricePoints, %s\n", prices);
}

bool CPricePointMemCache::AddBlockPricePointInBatch(const int32_t blockHeight, const CRegID &regId,
                                                    const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps) {
        if (ExistBlockUserPrice(blockHeight, regId, pp.GetCoinPricePair())) {
            LogPrint("PRICEFEED", "CPricePointMemCache::AddBlockPricePointInBatch, existed block user price, "
                     "height: %d, redId: %s, pricePoint: %s", blockHeight, regId.ToString(), pp.ToString());
            return false;
        }

        CConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPricePair()];
        cbp.AddUserPrice(blockHeight, regId, pp.GetPrice());
    }

    return true;
}

bool CPricePointMemCache::ExistBlockUserPrice(const int32_t blockHeight, const CRegID &regId,
                                              const CoinPricePair &coinPricePair) {
    if (mapCoinPricePointCache.count(coinPricePair) &&
        mapCoinPricePointCache[coinPricePair].ExistBlockUserPrice(blockHeight, regId))
        return true;

    if (pBase)
        return pBase->ExistBlockUserPrice(blockHeight, regId, coinPricePair);
    else
        return false;
}

bool CPricePointMemCache::AddBlockToCache(const CBlock &block) {
    // index[0]: block reward transaction
    // index[1]: block median price transaction
    // index[2 - n]: price feed transactions if existed.

    if (block.vptx.size() < 3) {
        return true;
    }

    // More than 3 transactions in the block.
    for (uint32_t i = 2; i < block.vptx.size(); ++ i) {
        if (!block.vptx[i]->IsPriceFeedTx()) {
            break;
        }

        CPriceFeedTx *priceFeedTx = (CPriceFeedTx *)block.vptx[i].get();
        AddBlockPricePointInBatch(block.GetHeight(), priceFeedTx->txUid.get<CRegID>(), priceFeedTx->price_points);
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
                mapCoinPricePointCache[item.first /* CoinPricePair */].mapBlockUserPrices.erase(
                    userPrice.first /* height */);
            } else {
                // typedef map<int32_t /* block height */, map<CRegID, uint64_t /* price */>> BlockUserPriceMap;
                for (const auto &priceItem : userPrice.second) {
                    mapCoinPricePointCache[item.first /* CoinPricePair */]
                        .mapBlockUserPrices[userPrice.first /* height */]
                        .emplace(priceItem.first /* CRegID */, priceItem.second /* price */);
                }
            }
        }
    }
}

void CPricePointMemCache::SetBaseViewPtr(CPricePointMemCache *pBaseIn) {
    pBase                        = pBaseIn;
    latestBlockMedianPricePoints = pBaseIn->latestBlockMedianPricePoints;
}

void CPricePointMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapCoinPricePointCache);
    mapCoinPricePointCache.clear();

    pBase->latestBlockMedianPricePoints = latestBlockMedianPricePoints;
    latestBlockMedianPricePoints.clear();
}

bool CPricePointMemCache::GetBlockUserPrices(const CoinPricePair &coinPricePair, set<int32_t> &expired,
                                             BlockUserPriceMap &blockUserPrices) {
    const auto &iter = mapCoinPricePointCache.find(coinPricePair);
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
        return pBase->GetBlockUserPrices(coinPricePair, expired, blockUserPrices);
    }

    return true;
}

bool CPricePointMemCache::GetBlockUserPrices(const CoinPricePair &coinPricePair, BlockUserPriceMap &blockUserPrices) {
    set<int32_t /* block height */> expired;
    if (!GetBlockUserPrices(coinPricePair, expired, blockUserPrices)) {
        // TODO: log
        return false;
    }

    return true;
}

uint64_t CPricePointMemCache::ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                                      const CoinPricePair &coinPricePair) {
    // 1. merge block user prices with base cache.
    BlockUserPriceMap blockUserPrices;
    if (!GetBlockUserPrices(coinPricePair, blockUserPrices) || blockUserPrices.empty()) {
        return 0;
    }

    // 2. compute block median price.
    return ComputeBlockMedianPrice(blockHeight, slideWindowBlockCount, blockUserPrices);
}

uint64_t CPricePointMemCache::ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                                      const BlockUserPriceMap &blockUserPrices) {
    // for (const auto &item : blockUserPrices) {
    //     string price;
    //     for (const auto &userPrice : item.second) {
    //         price += strprintf("{user:%s, price:%lld}", userPrice.first.ToString(), userPrice.second);
    //     }

    //     LogPrint("PRICEFEED", "CPricePointMemCache::ComputeBlockMedianPrice, height: %d, userPrice: %s\n",
    //              item.first, price);
    // }

    vector<uint64_t> prices;
    int32_t beginBlockHeight = std::max((blockHeight - slideWindowBlockCount), uint64_t(0));
    for (int32_t height = blockHeight; height > beginBlockHeight; --height) {
        const auto &iter = blockUserPrices.find(height);
        if (iter != blockUserPrices.end()) {
            for (const auto &userPrice : iter->second) {
                prices.push_back(userPrice.second);
            }
        }
    }

    // for (const auto &item : prices) {
    //     LogPrint("PRICEFEED", "CPricePointMemCache::ComputeBlockMedianPrice, found a candidate price: %llu\n", item);
    // }

    uint64_t medianPrice = ComputeMedianNumber(prices);
    LogPrint("PRICEFEED", "CPricePointMemCache::ComputeBlockMedianPrice, computed median number: %llu\n", medianPrice);

    return medianPrice;
}

uint64_t CPricePointMemCache::ComputeMedianNumber(vector<uint64_t> &numbers) {
    int32_t size = numbers.size();
    if (size < 2) {
        return size == 0 ? 0 : numbers[0];
    }
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size / 2 - 1] + numbers[size / 2]) / 2 : numbers[size / 2];
}

uint64_t CPricePointMemCache::GetMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                             const CoinPricePair &coinPricePair) {
    uint64_t medianPrice = ComputeBlockMedianPrice(blockHeight, slideWindowBlockCount, coinPricePair);

    if (medianPrice == 0) {
        medianPrice =
            latestBlockMedianPricePoints.count(coinPricePair) ? latestBlockMedianPricePoints[coinPricePair] : 0;
        LogPrint("PRICEFEED", "CPricePointMemCache::GetMedianPrice, use previous block median price: %s/%s -> %llu\n",
                 std::get<0>(coinPricePair), std::get<1>(coinPricePair), medianPrice);
    }

    return medianPrice;
}

uint64_t CPricePointMemCache::GetBcoinMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount) {
    return GetMedianPrice(blockHeight, slideWindowBlockCount, CoinPricePair(SYMB::WICC, SYMB::USD));
}

uint64_t CPricePointMemCache::GetFcoinMedianPrice(const int32_t blockHeight, const uint64_t slideWindowBlockCount) {
    return GetMedianPrice(blockHeight, slideWindowBlockCount, CoinPricePair(SYMB::WGRT, SYMB::USD));
}

bool CPricePointMemCache::GetBlockMedianPricePoints(const int32_t blockHeight, const uint64_t slideWindowBlockCount,
                                                    map<CoinPricePair, uint64_t> &mapMedianPricePoints) {
    CoinPricePair bcoinPricePair(SYMB::WICC, SYMB::USD);
    uint64_t bcoinMedianPrice = GetMedianPrice(blockHeight, slideWindowBlockCount,bcoinPricePair);
    mapMedianPricePoints.emplace(bcoinPricePair, bcoinMedianPrice);
    LogPrint("PRICEFEED", "CPricePointMemCache::GetBlockMedianPricePoints, %s/%s -> %llu\n", SYMB::WICC, SYMB::USD,
             bcoinMedianPrice);

    CoinPricePair fcoinPricePair(SYMB::WGRT, SYMB::USD);
    uint64_t fcoinMedianPrice = GetMedianPrice(blockHeight, slideWindowBlockCount, fcoinPricePair);
    mapMedianPricePoints.emplace(fcoinPricePair, fcoinMedianPrice);
    LogPrint("PRICEFEED", "CPricePointMemCache::GetBlockMedianPricePoints, %s/%s -> %llu\n", SYMB::WGRT, SYMB::USD,
             fcoinMedianPrice);

    // Update latest block median price points.
    SetLatestBlockMedianPricePoints(mapMedianPricePoints);

    return true;
}
