// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "chainparams.h"
#include "commons/serialize.h"
#include "main.h"
#include "persistence/block.h"
#include "util.h"
#include "vm/vmrunenv.h"

#include <algorithm>

bool CTransactionCache::IsContainBlock(const CBlock &block) {
    return mapBlockTxHashSet.count(block.GetHash());
}

bool CTransactionCache::AddBlockToCache(const CBlock &block) {
    UnorderedHashSet vTxHash;
    vTxHash.clear();
    for (auto &ptx : block.vptx) {
        vTxHash.insert(ptx->GetHash());
    }
    mapBlockTxHashSet[block.GetHash()] = vTxHash;

    return true;
}

bool CTransactionCache::DeleteBlockFromCache(const CBlock &block) {
    if (IsContainBlock(block)) {
        mapBlockTxHashSet.erase(block.GetHash());
    }

    return true;
}

bool CTransactionCache::HaveTx(const uint256 &txHash) {
    for (auto &item : mapBlockTxHashSet) {
        if (item.second.count(txHash)) {
            return true;
        }
    }

    return false;
}

void CTransactionCache::AddTxHashCache(const uint256 &blockHash, const UnorderedHashSet &vTxHash) {
    mapBlockTxHashSet[blockHash] = vTxHash;
}

void CTransactionCache::Clear() { mapBlockTxHashSet.clear(); }

int CTransactionCache::GetSize() { return mapBlockTxHashSet.size(); }

Object CTransactionCache::ToJsonObj() const {
    Array txArray;
    for (auto &item : mapBlockTxHashSet) {
        Object obj;
        obj.push_back(Pair("block_hash", item.first.ToString()));

        Array txsObj;
        for (const auto &itemTx : item.second) {
            Object txObj;
            txObj.push_back(Pair("tx_hash", itemTx.ToString()));
            txsObj.push_back(txObj);
        }
        obj.push_back(Pair("txs", txsObj));
        txArray.push_back(obj);
    }

    Object txCacheObj;
    txCacheObj.push_back(Pair("tx_cache", txArray));
    return txCacheObj;
}

const map<uint256, UnorderedHashSet> &CTransactionCache::GetTxHashCache() { return mapBlockTxHashSet; }

void CTransactionCache::SetTxHashCache(const map<uint256, UnorderedHashSet> &mapCache) {
    mapBlockTxHashSet = mapCache;
}

/****************************************************************************************************/
void CConsecutiveBlockPrice::AddPricePoint(const int height, const CUserID &txUid, uint64_t price) {
    mapBlockTxUidSet[ height ].insert(txUid.ToString());
    mapBlockPriceList[ height ].push_back(price);
}

uint64_t CConsecutiveBlockPrice::ComputeBlockMedianPrice(const int blockHeight) {
    if (mapBlockPriceList.find(blockHeight) == mapBlockPriceList.end() ||
        mapBlockPriceList[ blockHeight ].size() == 0) {
        currBlockMediaPrice = lastBlockMediaPrice;
        return currBlockMediaPrice;
    }

    if (mapBlockPriceList.find(blockHeight - 1) == mapBlockPriceList.end() ||
        mapBlockPriceList[ blockHeight - 1 ].size() == 0) { // one block cache only
        return ComputeMedianNumber(mapBlockPriceList[ blockHeight ]);
    }

    //two blocks
    vector<uint64_t> v1 = mapBlockPriceList[ blockHeight-1 ];
    vector<uint64_t> v2 = mapBlockPriceList[ blockHeight ];
    v1.insert( v1.end(), v2.begin(), v2.end() );
    return ComputeMedianNumber(v1);
}

bool CConsecutiveBlockPrice::ExistBlockUserPrice(const int height, const CUserID &txUid) {
    if (mapBlockTxUidSet.find(height) == mapBlockTxUidSet.end())
        return false;

    unordered_set<string> uids = mapBlockTxUidSet[ height ];
    return (uids.find(txUid.ToString()) != uids.end());
}

uint64_t CConsecutiveBlockPrice::ComputeMedianNumber(vector<uint64_t> &numbers) {
    unsigned int size = numbers.size();
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size/2 - 1] + numbers[size/2]) / 2 : numbers[size/2];
}

/****************************************************************************************************/
bool CPricePointCache::AddBlockPricePointInBatch(const int blockHeight,
        const CUserID &txUid, const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps ) {
        CConsecutiveBlockPrice cbp = mapCoinPricePointCache[ pp.GetCoinPriceType().ToString() ];
        if (cbp.ExistBlockUserPrice(blockHeight, txUid))
            return false;

        cbp.AddPricePoint(blockHeight, txUid, pp.GetPrice());
    }

    return true;
}

uint64_t CPricePointCache::ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType) {
    CConsecutiveBlockPrice cbp = mapCoinPricePointCache[ coinPriceType.ToString() ];
    uint64_t medianPrice = cbp.ComputeBlockMedianPrice(blockHeight);
    return medianPrice;
}