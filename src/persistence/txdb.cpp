// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "config/chainparams.h"
#include "commons/serialize.h"
#include "main.h"
#include "persistence/block.h"
#include "commons/util/util.h"
#include "vm/luavm/luavmrunenv.h"

#include <algorithm>

bool CTxMemCache::AddBlockTx(const CBlock &block) {
    for (auto &ptx : block.vptx) {
        txids[ptx->GetHash()] = true;
    }
    return true;
}

bool CTxMemCache::RemoveBlockTx(const CBlock &block) {
    for (auto &ptx : block.vptx) {
        if (pBase == nullptr) {
            txids.erase(ptx->GetHash());
        } else {
            txids[ptx->GetHash()] = false;
        }
    }
    return true;
}

bool CTxMemCache::HasTx(const uint256 &txid) {
    auto it = txids.find(txid);
    if (it != txids.end()) {
        return it->second;
    }

    if (pBase != nullptr) {
        return pBase->HasTx(txid);
    }
    return false;
}

void CTxMemCache::BatchWrite(const TxIdMap &txidsIn) {
    for (const auto &item : txidsIn) {
        if (pBase == nullptr && !item.second) {
            txids.erase(item.first);
        } else {
            txids[item.first] = item.second;

        }
    }
}

void CTxMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(txids);
    txids.clear();
}

void CTxMemCache::Clear() { txids.clear(); }

uint64_t CTxMemCache::GetSize() { return txids.size(); }

Object CTxMemCache::ToJsonObj() const {
    Array txArray;
    for (auto &item : txids) {
        Object obj;
        obj.push_back(Pair("txid", item.first.ToString()));
        obj.push_back(Pair("existed", item.second));
        txArray.push_back(obj);
    }

    Object txCacheObj;
    txCacheObj.push_back(Pair("tx_cache", txArray));
    return txCacheObj;
}
