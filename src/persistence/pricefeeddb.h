// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_PRICEFEED_H
#define PERSIST_PRICEFEED_H

#include "accounts/account.h"
#include "accounts/id.h"
#include "commons/serialize.h"
#include "tx/tx.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

// Price Points in 11 consecutive blocks
class CConsecutiveBlockPrice {
public:
    void AddUserPrice(const int blockHeight, const CRegID &regId, const uint64_t price);
    // delete user price by specific block height.
    void DeleteUserPrice(const int blockHeight);
    bool ExistBlockUserPrice(const int blockHeight, const CRegID &regId);
    uint64_t ComputeBlockMedianPrice(const int blockHeight);
    uint64_t GetLastBlockMedianPrice();

public:
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);

private:
    map<int, map<string, uint64_t>> mapBlockUserPrices;  // height -> { regId -> price }
};

class CPricePointCache {
public:
    CPricePointCache() : pBase(nullptr) {}
    CPricePointCache(CPricePointCache *pBaseIn) : pBase(pBaseIn) {}

public:
    bool AddBlockPricePointInBatch(const int blockHeight, const CRegID &regId, const vector<CPricePoint> &pps);
    // delete block price point by specific block height.
    bool DeleteBlockPricePoint(const int blockHeight);

    void ComputeBlockMedianPrice(const int blockHeight);
    uint64_t GetBcoinMedianPrice() { return bcoinMedianPrice; }
    uint64_t GetFcoinMedianPrice() { return fcoinMedianPrice; }

    void Flush();

private:
    uint64_t ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType);
    void BatchWrite(const map<string, CConsecutiveBlockPrice> &mapCoinPricePointCacheIn);

private:
    uint64_t bcoinMedianPrice;  // against scoin
    uint64_t fcoinMedianPrice;  // against scoin

    map<string, CConsecutiveBlockPrice> mapCoinPricePointCache;  // coinPriceType -> consecutiveBlockPrice
    CPricePointCache *pBase;
};

#endif  // PERSIST_PRICEFEED_H