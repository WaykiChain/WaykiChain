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

bool CTxMemCache::IsContainBlock(const CBlock &block) {
    return mapBlockTxHashSet.count(block.GetHash()) || (pBase ? pBase->IsContainBlock(block) : false);
}

bool CTxMemCache::AddBlockToCache(const CBlock &block) {
    UnorderedHashSet vTxHash;
    for (auto &ptx : block.vptx) {
        vTxHash.insert(ptx->GetHash());
    }
    mapBlockTxHashSet[block.GetHash()] = vTxHash;

    return true;
}

bool CTxMemCache::DeleteBlockFromCache(const CBlock &block) {
    if (IsContainBlock(block)) {
        UnorderedHashSet txHash;
		mapBlockTxHashSet[block.GetHash()] = txHash;

        return true;
    }

    LogPrint("ERROR", "failed to delete transactions in block: %s", block.GetHash().GetHex());
    return false;
}

bool CTxMemCache::HaveTx(const uint256 &txHash) {
    for (auto &item : mapBlockTxHashSet) {
        if (item.second.count(txHash)) {
            return true;
        }
    }

    return pBase ? pBase->HaveTx(txHash) : false;
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

void CTxMemCache::Flush(CTxMemCache *pBaseIn) {
    pBaseIn->BatchWrite(mapBlockTxHashSet);
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

void CConsecutiveBlockPrice::AddUserPrice(const int blockHeight, const CRegID &regId, const uint64_t price) {
    mapBlockUserPrices[blockHeight][regId.ToRawString()] = price;
}

void CConsecutiveBlockPrice::DeleteUserPrice(const int blockHeight) {
    mapBlockUserPrices.erase(blockHeight);
}

uint64_t CConsecutiveBlockPrice::ComputeBlockMedianPrice(const int blockHeight) {
    // TODO: parameterize 11.
    assert(blockHeight >= 11);
    vector<uint64_t> prices;
    for (int height = blockHeight; height > blockHeight - 11; -- height) {
        if (mapBlockUserPrices.count(height) != 0) {
            for (auto userPrice : mapBlockUserPrices[height]) {
                prices.push_back(userPrice.second);
            }
        }
    }

    return ComputeMedianNumber(prices);
}

bool CConsecutiveBlockPrice::ExistBlockUserPrice(const int blockHeight, const CRegID &regId) {
    if (mapBlockUserPrices.count(blockHeight) == 0)
        return false;

    return mapBlockUserPrices[blockHeight].count(regId.ToRawString());
}

uint64_t CConsecutiveBlockPrice::ComputeMedianNumber(vector<uint64_t> &numbers) {
    unsigned int size = numbers.size();
    if (size < 2) {
        return size == 0 ? 0 : numbers[0];
    }
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size / 2 - 1] + numbers[size / 2]) / 2 : numbers[size / 2];
}

bool CPricePointCache::AddBlockPricePointInBatch(const int blockHeight, const CRegID &regId,
                                                 const vector<CPricePoint> &pps) {
    for (CPricePoint pp : pps) {
        CConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPriceType().ToString()];
        if (cbp.ExistBlockUserPrice(blockHeight, regId))
            return false;

        cbp.AddUserPrice(blockHeight, regId, pp.GetPrice());
    }

    return true;
}

bool CPricePointCache::DeleteBlockPricePoint(const int blockHeight) {
    for (auto &item : mapCoinPricePointCache) {
        item.second.DeleteUserPrice(blockHeight);
    }

    return true;
}

void CPricePointCache::ComputeBlockMedianPrice(const int blockHeight) {
    bcoinMedianPrice = ComputeBlockMedianPrice(blockHeight, CCoinPriceType(CoinType::WICC, PriceType::USD));
    fcoinMedianPrice = ComputeBlockMedianPrice(blockHeight, CCoinPriceType(CoinType::MICC, PriceType::USD));
}

uint64_t CPricePointCache::ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType) {
    return mapCoinPricePointCache.count(coinPriceType.ToString())
               ? mapCoinPricePointCache[coinPriceType.ToString()].ComputeBlockMedianPrice(blockHeight)
               : 0;
}

string CTxUndo::ToString() const {
    string str;
    string strTxid("txid:");
    strTxid += txHash.GetHex();

    str += strTxid + "\n";

    string strAccountLog("list account log:");
    for (auto iterLog : accountLogs) {
        strAccountLog += iterLog.ToString();
        strAccountLog += ";";
    }

    str += strAccountLog + "\n";

    str += "list db log:" + dbOpLogMap.ToString();

    return str;
}

bool CTxUndo::GetAccountLog(const CKeyID &keyId, CAccountLog &accountLog) {
    for (auto iterLog : accountLogs) {
        if (iterLog.keyID == keyId) {
            accountLog = iterLog;
            return true;
        }
    }
    return false;
}

bool ReadTxFromDisk(const CTxCord txCord, std::shared_ptr<CBaseTx> ptrBaseTx) {
    // TODO: read tx from disk
    return false;
}
