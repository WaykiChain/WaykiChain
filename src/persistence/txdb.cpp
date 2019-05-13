// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain Developers
// Copyright (c) 2019- The WaykiChain CoreDev
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "txdb.h"

#include "chainparams.h"
#include "main.h"
#include "commons/serialize.h"
#include "util.h"
#include "vm/vmrunenv.h"

#include <algorithm>

// bool CTransactionDBView::HaveTx(const uint256 &txHash) { return false; }
// bool CTransactionDBView::IsContainBlock(const CBlock &block) { return false; }
// bool CTransactionDBView::AddBlockToCache(const CBlock &block) { return false; }
// bool CTransactionDBView::DeleteBlockFromCache(const CBlock &block) { return false; }
// bool CTransactionDBView::BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHash) { return false; }

// CTransactionDBViewBacked::CTransactionDBViewBacked(CTransactionDBView &transactionView) {
//     pBase = &transactionView;
// }

// bool CTransactionDBViewBacked::HaveTx(const uint256 &txHash) {
//     return pBase->HaveTx(txHash);
// }

// bool CTransactionDBViewBacked::IsContainBlock(const CBlock &block) {
//     return pBase->IsContainBlock(block);
// }

// bool CTransactionDBViewBacked::AddBlockToCache(const CBlock &block) {
//     return pBase->AddBlockToCache(block);
// }

// bool CTransactionDBViewBacked::DeleteBlockFromCache(const CBlock &block) {
//     return pBase->DeleteBlockFromCache(block);
// }

// bool CTransactionDBViewBacked::BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHashIn) {
//     return pBase->BatchWrite(mapTxHashByBlockHashIn);
// }

bool CTransactionDBCache::IsContainBlock(const CBlock &block) {
    //(mapTxHashByBlockHash.count(block.GetHash()) > 0 && mapTxHashByBlockHash[block.GetHash()].size() > 0)
    return ( IsInMap(mapTxHashByBlockHash, block.GetHash()) || pBase->IsContainBlock(block) );
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
        UnorderedHashSet vTxHash;
        vTxHash.clear();
        mapTxHashByBlockHash[block.GetHash()] = vTxHash;
    }
    return true;
}

bool CTransactionDBCache::HaveTx(const uint256 &txHash) {
    for (auto &item : mapTxHashByBlockHash) {
        if (item.second.find(txHash) != item.second.end()) {
            return true;
        }
    }

    return false;
}

map<uint256, UnorderedHashSet> CTransactionDBCache::GetTxHashCache() {
    return mapTxHashByBlockHash;
}

bool CTransactionDBCache::BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHashIn) {
    for (auto &item : mapTxHashByBlockHashIn) {
        mapTxHashByBlockHash[item.first] = item.second;
    }
    return true;
}

bool CTransactionDBCache::Flush() {
    map<uint256, UnorderedHashSet>::iterator iter = mapTxHashByBlockHash.begin();
    for (; iter != mapTxHashByBlockHash.end();) {
        if (iter->second.empty()) {
            mapTxHashByBlockHash.erase(iter++);
        } else {
            iter++;
        }
    }

    return true;
}

void CTransactionDBCache::AddTxHashCache(const uint256 &blockHash, const UnorderedHashSet &vTxHash) {
    mapTxHashByBlockHash[blockHash] = vTxHash;
}

void CTransactionDBCache::Clear() {
    mapTxHashByBlockHash.clear();
}

int CTransactionDBCache::GetSize() {
    int iCount(0);
    for (auto &i : mapTxHashByBlockHash) {
        if (!i.second.empty())
            ++iCount;
    }
    return iCount;
}

bool CTransactionDBCache::IsInMap(const map<uint256, UnorderedHashSet> &mMap, const uint256 &blockHash) const {
    if (blockHash == uint256())
        return false;
    auto te = mMap.find(blockHash);
    if (te != mMap.end()) {
        return !te->second.empty();
    }

    return false;
}

Object CTransactionDBCache::ToJsonObj() const {
    Array deletedobjArray;
    Array inCacheObjArray;
    for (auto &item : mapTxHashByBlockHash) {
        Object obj;
        obj.push_back(Pair("blockhash", item.first.ToString()));

        Array objTxInBlock;
        for (const auto &itemTx : item.second) {
            Object objTxHash;
            objTxHash.push_back(Pair("txhash", itemTx.ToString()));
            objTxInBlock.push_back(objTxHash);
        }
        obj.push_back(Pair("txHashes", objTxInBlock));
        if (item.second.size() > 0) {
            inCacheObjArray.push_back(obj);
        } else {
            deletedobjArray.push_back(obj);
        }
    }
    Object temobj;
    temobj.push_back(Pair("incachblock", inCacheObjArray));
    //	temobj.push_back(Pair("removecachblock", deletedobjArray));
    Object retobj;
    retobj.push_back(Pair("mapTxHashByBlockHash", temobj));
    return retobj;
}

const map<uint256, UnorderedHashSet> &CTransactionDBCache::GetCacheMap() {
    return mapTxHashByBlockHash;
}

void CTransactionDBCache::SetCacheMap(const map<uint256, UnorderedHashSet> &mapCache) {
    mapTxHashByBlockHash = mapCache;
}