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

bool CTransactionDBCache::IsContainBlock(const CBlock &block) {
    return mapTxHashByBlockHash.count(block.GetHash());
}

bool CTransactionDBCache::AddBlockToCache(const CBlock &block) {
    UnorderedHashSet vTxHash;
    vTxHash.clear();
    for (auto &ptx : block.vptx) {
        vTxHash.insert(ptx->GetHash());
    }
    mapTxHashByBlockHash[block.GetHash()] = vTxHash;

    return true;
}

bool CTransactionDBCache::DeleteBlockFromCache(const CBlock &block) {
    if (IsContainBlock(block)) {
        mapTxHashByBlockHash.erase(block.GetHash());
    }

    return true;
}

bool CTransactionDBCache::HaveTx(const uint256 &txHash) {
    for (auto &item : mapTxHashByBlockHash) {
        if (item.second.count(txHash)) {
            return true;
        }
    }

    return false;
}

void CTransactionDBCache::AddTxHashCache(const uint256 &blockHash, const UnorderedHashSet &vTxHash) {
    mapTxHashByBlockHash[blockHash] = vTxHash;
}

void CTransactionDBCache::Clear() { mapTxHashByBlockHash.clear(); }

int CTransactionDBCache::GetSize() { return mapTxHashByBlockHash.size(); }

Object CTransactionDBCache::ToJsonObj() const {
    Array txArray;
    for (auto &item : mapTxHashByBlockHash) {
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

const map<uint256, UnorderedHashSet> &CTransactionDBCache::GetTxHashCache() { return mapTxHashByBlockHash; }

void CTransactionDBCache::SetTxHashCache(const map<uint256, UnorderedHashSet> &mapCache) {
    mapTxHashByBlockHash = mapCache;
}