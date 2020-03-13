// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeeddb.h"

#include "config/scoin.h"
#include "main.h"
#include "tx/pricefeedtx.h"

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

bool CPricePointMemCache::AddPrice(const int32_t blockHeight, const CRegID &regId,
                                                    const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps) {
        if (ExistBlockUserPrice(blockHeight, regId, pp.GetCoinPricePair())) {
            LogPrint(BCLog::PRICEFEED,
                     "CPricePointMemCache::AddPrice, existed block user price, "
                     "height: %d, redId: %s, pricePoint: %s\n",
                     blockHeight, regId.ToString(), pp.ToString());
            return false;
        }

        CConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPricePair()];
        cbp.AddUserPrice(blockHeight, regId, pp.GetPrice());
        LogPrint(BCLog::PRICEFEED,
                 "CPricePointMemCache::AddPrice, add block user price, "
                 "height: %d, redId: %s, pricePoint: %s\n",
                 blockHeight, regId.ToString(), pp.ToString());
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

bool CPricePointMemCache::AddPriceByBlock(const CBlock &block) {
    // index[0]: block reward transaction
    // index[1 ~ n - 1]: price feed transactions if existing
    // index[n]: block median price transaction

    if (block.vptx.size() < 3) {
        return true;
    }

    // More than 3 transactions in the block.
    for (uint32_t i = 1; i < block.vptx.size(); ++i) {
        if (!block.vptx[i]->IsPriceFeedTx()) {
            break;
        }

        CPriceFeedTx *priceFeedTx = (CPriceFeedTx *)block.vptx[i].get();
        AddPrice(block.GetHeight(), priceFeedTx->txUid.get<CRegID>(), priceFeedTx->price_points);
    }

    return true;
}

bool CPricePointMemCache::DeleteBlockPricePoint(const int32_t blockHeight) {
    if (mapCoinPricePointCache.empty()) {
        // TODO: multi stable coin
        mapCoinPricePointCache[CoinPricePair(SYMB::WICC, SYMB::USD)].DeleteUserPrice(blockHeight);
        mapCoinPricePointCache[CoinPricePair(SYMB::WGRT, SYMB::USD)].DeleteUserPrice(blockHeight);
    } else {
        for (auto &item : mapCoinPricePointCache) {
            item.second.DeleteUserPrice(blockHeight);
        }
    }

    return true;
}

bool CPricePointMemCache::DeleteBlockFromCache(const CBlock &block) { return DeleteBlockPricePoint(block.GetHeight()); }

void CPricePointMemCache::BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn) {
    for (const auto &item : mapCoinPricePointCacheIn) {
        // map<int32_t /* block height */, map<CRegID, uint64_t /* price */>>
        const auto &mapBlockUserPrices = item.second.mapBlockUserPrices;
        for (const auto &userPrice : mapBlockUserPrices) {
            if (userPrice.second.empty()) {
                mapCoinPricePointCache[item.first /* CoinPricePair */].mapBlockUserPrices.erase(userPrice.first /* height */);
            } else {
                // map<CRegID, uint64_t /* price */>;
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
}

void CPricePointMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapCoinPricePointCache);
    mapCoinPricePointCache.clear();
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

uint64_t CPricePointMemCache::ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindow,
                                                      const CoinPricePair &coinPricePair) {
    // 1. merge block user prices with base cache.
    BlockUserPriceMap blockUserPrices;
    if (!GetBlockUserPrices(coinPricePair, blockUserPrices) || blockUserPrices.empty()) {
        return 0;
    }

    // 2. compute block median price.
    return ComputeBlockMedianPrice(blockHeight, slideWindow, blockUserPrices);
}

uint64_t CPricePointMemCache::ComputeBlockMedianPrice(const int32_t blockHeight, const uint64_t slideWindow,
                                                      const BlockUserPriceMap &blockUserPrices) {
    // for (const auto &item : blockUserPrices) {
    //     string price;
    //     for (const auto &userPrice : item.second) {
    //         price += strprintf("{user:%s, price:%lld}", userPrice.first.ToString(), userPrice.second);
    //     }

    //     LogPrint(BCLog::PRICEFEED, "CPricePointMemCache::ComputeBlockMedianPrice, height: %d, userPrice: %s\n",
    //              item.first, price);
    // }

    vector<uint64_t> prices;
    int32_t beginBlockHeight = std::max<int32_t>((blockHeight - slideWindow), 0);
    for (int32_t height = blockHeight; height > beginBlockHeight; --height) {
        const auto &iter = blockUserPrices.find(height);
        if (iter != blockUserPrices.end()) {
            for (const auto &userPrice : iter->second) {
                prices.push_back(userPrice.second);
            }
        }
    }

    // for (const auto &item : prices) {
    //     LogPrint(BCLog::PRICEFEED, "CPricePointMemCache::ComputeBlockMedianPrice, found a candidate price: %llu\n", item);
    // }

    uint64_t medianPrice = ComputeMedianNumber(prices);
    LogPrint(BCLog::PRICEFEED,
             "CPricePointMemCache::ComputeBlockMedianPrice, blockHeight: %d, computed median number: %llu\n",
             blockHeight, medianPrice);

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

uint64_t CPricePointMemCache::GetMedianPrice(const int32_t blockHeight, const uint64_t slideWindow,
                                             const CoinPricePair &coinPricePair) {
    uint64_t medianPrice = ComputeBlockMedianPrice(blockHeight, slideWindow, coinPricePair);

    if (medianPrice == 0) {
        auto it = latest_median_prices.find(coinPricePair);
        medianPrice = it != latest_median_prices.end() ? it->second : 0;

        LogPrint(BCLog::PRICEFEED,
                 "CPricePointMemCache::GetMedianPrice, use previous block median price: blockHeight: %d, "
                 "price: %s/%s -> %llu\n",
                 blockHeight, std::get<0>(coinPricePair), std::get<1>(coinPricePair), medianPrice);
    }

    return medianPrice;
}

bool CPricePointMemCache::CalcBlockMedianPrices(CCacheWrapper &cw, const int32_t blockHeight,
                                                PriceMap &medianPrices) {
    // TODO: support more price pair
    uint64_t slideWindow = 0;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindow)) {
        return ERRORMSG("%s, read sys param MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error", __func__);
    }

    latest_median_prices = cw.priceFeedCache.GetMedianPrices();

    CoinPricePair bcoinPricePair(SYMB::WICC, SYMB::USD);
    uint64_t bcoinMedianPrice = GetMedianPrice(blockHeight, slideWindow, bcoinPricePair);
    medianPrices.emplace(bcoinPricePair, bcoinMedianPrice);
    LogPrint(BCLog::PRICEFEED, "CPricePointMemCache::CalcBlockMedianPrices, blockHeight: %d, price: %s/%s -> %llu\n",
             blockHeight, SYMB::WICC, SYMB::USD, bcoinMedianPrice);

    CoinPricePair fcoinPricePair(SYMB::WGRT, SYMB::USD);
    uint64_t fcoinMedianPrice = GetMedianPrice(blockHeight, slideWindow, fcoinPricePair);
    medianPrices.emplace(fcoinPricePair, fcoinMedianPrice);
    LogPrint(BCLog::PRICEFEED, "CPricePointMemCache::CalcBlockMedianPrices, blockHeight: %d, price: %s/%s -> %llu\n",
             blockHeight, SYMB::WGRT, SYMB::USD, fcoinMedianPrice);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// CPriceFeedCache

uint64_t CPriceFeedCache::GetMedianPrice(const CoinPricePair &coinPricePair) const {
    PriceMap medianPrices;
    if (median_price_cache.GetData(medianPrices)) {
        auto it = medianPrices.find(coinPricePair);
        if (it != medianPrices.end())
            return it->second;
    }
    return 0;
}

PriceMap CPriceFeedCache::GetMedianPrices() const {
    PriceMap medianPrices;
    if (median_price_cache.GetData(medianPrices)) {
        return medianPrices;
    }
    return {};
}

bool CPriceFeedCache::SetMedianPrices(const PriceMap &medianPrices) {
    return median_price_cache.SetData(medianPrices);
}

bool CPriceFeedCache::AddFeedCoinPair(TokenSymbol feedCoin, TokenSymbol quoteCoin) {
    if(feedCoin == SYMB::WICC && quoteCoin == SYMB::USD)
        return true ;

    set<pair<TokenSymbol,TokenSymbol>> coinPairs;
    price_feed_coin_cache.GetData(coinPairs);
    if(coinPairs.count(make_pair(feedCoin, quoteCoin)) > 0 )
        return true;

    coinPairs.insert(make_pair(feedCoin, quoteCoin));
    return price_feed_coin_cache.SetData(coinPairs);
}

bool CPriceFeedCache::EraseFeedCoinPair(TokenSymbol feedCoin, TokenSymbol quoteCoin) {
    if(feedCoin == SYMB::WICC && quoteCoin == SYMB::USD)
        return true ;

    auto coinPair = std::make_pair(feedCoin, quoteCoin);
    set<pair<TokenSymbol,TokenSymbol>> coins ;
    price_feed_coin_cache.GetData(coins);
    if(coins.count(coinPair) == 0 )
        return true ;

    coins.erase(coinPair) ;
    return price_feed_coin_cache.SetData(coins);
}

bool CPriceFeedCache::HasFeedCoinPair(TokenSymbol feedCoin,TokenSymbol quoteCoin) {
    // WICC:USD is the default staked coin pair of cdp
    // WGRT:USD is need by forced-liquidate cdp for inflating WGRT
    if((feedCoin == SYMB::WICC || feedCoin == SYMB::WGRT) && quoteCoin == SYMB::USD)
        return true ;

    set<pair<TokenSymbol, TokenSymbol>> coins ;
    price_feed_coin_cache.GetData(coins);
    return (coins.count(make_pair(feedCoin,quoteCoin)) != 0 ) ;
}

bool CPriceFeedCache::GetFeedCoinPairs(set<pair<TokenSymbol,TokenSymbol>>& coinSet) {
    price_feed_coin_cache.GetData(coinSet) ;
    coinSet.insert(make_pair(SYMB::WICC, SYMB::USD));
    return true ;
}
