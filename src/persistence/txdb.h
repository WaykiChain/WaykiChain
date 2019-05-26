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

class CBlockPricePointSet {
private:
    set<CPricePoint> set1;
    set<CPricePoint> set2;

    set<CPricePoint> *pLastPricePointSet;
    set<CPricePoint> *pCurrPricePointSet;

    uint64_t lastMedianPrice;
    uint64_t currMedianPrice;

public:
    CBlockPricePointSet(): pLastPricePointSet(&set1), pCurrPricePointSet(&set2) {}

};

class CPricePointCache {
private:
    map<CCoinPriceType, CBlockPricePointSet> mapCoinPricePointSet;

public:
    uint64_t GetMedianPrice(CCoinPriceType coinPriceType);

    void AddPricePoint(CPricePoint pp);
};
#endif // PERSIST_TXDB_H