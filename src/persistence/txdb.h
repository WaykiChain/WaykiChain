// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_TXDB_H
#define PERSIST_TXDB_H

#include "vm/appaccount.h"
#include "commons/serialize.h"
#include "json/json_spirit_value.h"

#include <map>
#include <vector>

using namespace std;
using namespace json_spirit;

class CBlock;
class CPricePoint;
class CCoinPriceType;


class CTransactionCache {
private:
    map<uint256, UnorderedHashSet> mapBlockTxHashSet;  // map: BlockHash ->TxhashSet

public:
    bool HaveTx(const uint256 &txHash);
    bool IsContainBlock(const CBlock &block);
    bool AddBlockToCache(const CBlock &block);
    bool DeleteBlockFromCache(const CBlock &block);
    void AddTxHashCache(const uint256 &blockHash, const UnorderedHashSet &vTxHash);
    void Clear();
    Object ToJsonObj() const;
    int GetSize();
    const map<uint256, UnorderedHashSet> &GetTxHashCache();
    void SetTxHashCache(const map<uint256, UnorderedHashSet> &mapCache);
};

// Price Points in two consecutive blocks
class CConsecutiveBlockPrice {
private:
    map<int, set<uint64_t>> mapBlockPrice;    // height -> PricePointSet
    int lastBlockHeight;
    int currBlockHeight;
    uint64_t lastBlockMediaPrice;
    uint64_t currBlockMediaPrice;

public:
    void AddPricePoint(const int height, const uint64_t price);
    uint64_t ComputeBlockMediaPrice(const int blockHeight);
};

class CPricePointCache {
private:
    map<CCoinPriceType, CConsecutiveBlockPrice> mapCoinPricePointSet; // coinPriceType -> consecutiveBlockPrice

public:
    void AddBlockPricePoint(const int blocHeight, const CPricePoint pp);
    uint64_t GetMedianPrice(const CCoinPriceType coinPriceType);
};
#endif // PERSIST_TXDB_H