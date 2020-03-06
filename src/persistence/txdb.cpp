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
        txids.insert(ptx->GetHash());
    }
    return true;
}

bool CTxMemCache::RemoveBlockTx(const CBlock &block) {
    for (auto &ptx : block.vptx) {
        txids.erase(ptx->GetHash());
    }
    return true;
}

bool CTxMemCache::HasTx(const uint256 &txid) {
    bool found = txids.count(txid) > 0;
    if (found)
        return true;
    else if (pBase == nullptr) {
        return false;
    } else
        return pBase->HasTx(txid);
}

void CTxMemCache::BatchWrite(const UnorderedHashSet &txidsIn) {
    txids.clear();

    for (const auto &txid : txidsIn) {
        txids.insert(txid);
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
    for (auto &txid : txids) {
        txArray.push_back(txid.ToString());
    }

    Object txCacheObj;
    txCacheObj.push_back(Pair("tx_cache", txArray));
    return txCacheObj;
}
