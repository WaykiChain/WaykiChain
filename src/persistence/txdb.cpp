// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "config/chainparams.h"
#include "commons/serialize.h"
#include "main.h"
#include "persistence/block.h"
#include "commons/util.h"
#include "vm/luavm/vmrunenv.h"

#include <algorithm>

bool CTxMemCache::IsContainBlock(const CBlock &block) {
    return mapBlockTxHashSet.count(block.GetHash()) || (pBase ? pBase->IsContainBlock(block) : false);
}

bool CTxMemCache::AddBlockToCache(const CBlock &block) {
    UnorderedHashSet txids;
    for (auto &ptx : block.vptx) {
        txids.insert(ptx->GetHash());
    }
    mapBlockTxHashSet[block.GetHash()] = txids;

    return true;
}

bool CTxMemCache::DeleteBlockFromCache(const CBlock &block) {
    if (IsContainBlock(block)) {
        UnorderedHashSet txids;
		mapBlockTxHashSet[block.GetHash()] = txids;
    }

    // On starting node, the memory cache is empty, thus, can not find the
    // specific block as expected.
    return true;
}

bool CTxMemCache::HaveTx(const uint256 &txid) {
    for (auto &item : mapBlockTxHashSet) {
        if (item.second.count(txid)) {
            return true;
        }
    }

    return pBase ? pBase->HaveTx(txid) : false;
}

void CTxMemCache::BatchWrite(const map<uint256, UnorderedHashSet> &mapBlockTxHashSetIn) {
    // If the value is empty, delete it from cache.
    for (const auto &item : mapBlockTxHashSetIn) {
        if (item.second.empty()) {
            mapBlockTxHashSet.erase(item.first);
        } else {
            mapBlockTxHashSet[item.first] = item.second;
        }
    }
}

void CTxMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapBlockTxHashSet);
    mapBlockTxHashSet.clear();
}

void CTxMemCache::Clear() { mapBlockTxHashSet.clear(); }

uint64_t CTxMemCache::GetSize() { return mapBlockTxHashSet.size(); }

Object CTxMemCache::ToJsonObj() const {
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

const map<uint256, UnorderedHashSet> &CTxMemCache::GetTxHashCache() { return mapBlockTxHashSet; }

void CTxMemCache::SetTxHashCache(const map<uint256, UnorderedHashSet> &mapCache) {
    mapBlockTxHashSet = mapCache;
}

string CTxUndo::ToString() const {
    string str;
    str += "txid:" + txid.GetHex() + "\n";
    str += "db_op_log_map:" + dbOpLogMap.ToString();
    return str;
}
