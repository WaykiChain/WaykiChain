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
    return mapBlockTxHashSet.count(block.GetHash()) || (pBase ? pBase->IsContainBlock(block) : false);
}

bool CTransactionCache::AddBlockToCache(const CBlock &block) {
    UnorderedHashSet vTxHash;
    for (auto &ptx : block.vptx) {
        vTxHash.insert(ptx->GetHash());
    }
    mapBlockTxHashSet[block.GetHash()] = vTxHash;

    return true;
}

bool CTransactionCache::DeleteBlockFromCache(const CBlock &block) {
    if (IsContainBlock(block)) {
        UnorderedHashSet txHash;
		mapBlockTxHashSet[block.GetHash()] = txHash;

        return true;
    }

    LogPrint("ERROR", "failed to delete transactions in block: %s", block.GetHash().GetHex());
    return false;
}

bool CTransactionCache::HaveTx(const uint256 &txHash) {
    for (auto &item : mapBlockTxHashSet) {
        if (item.second.count(txHash)) {
            return true;
        }
    }

    return pBase ? pBase->HaveTx(txHash) : false;
}

void CTransactionCache::BatchWrite(const map<uint256, UnorderedHashSet> &mapBlockTxHashSetIn) {
    // If the value is empty, delete it from cache.
    for (auto &item : mapBlockTxHashSetIn) {
        if (item.second.empty()) {
            mapBlockTxHashSet.erase(item.first);
        } else {
            mapBlockTxHashSet[item.first] = item.second;
        }
    }
}

void CTransactionCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapBlockTxHashSet);
    mapBlockTxHashSet.clear();
}

void CTransactionCache::Flush(CTransactionCache *pBaseIn) {
    pBaseIn->BatchWrite(mapBlockTxHashSet);
    mapBlockTxHashSet.clear();
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
void CConsecutiveBlockPrice::AddUserPrice(const int height, const CUserID &txUid, const uint64_t price) {
    mapBlockUserPrices[ height ][ txUid.ToString() ] = price;
}

uint64_t CConsecutiveBlockPrice::ComputeBlockMedianPrice(const int blockHeight) {
    if (mapBlockUserPrices.count(blockHeight) == 0 ||
        mapBlockUserPrices[ blockHeight ].size() == 0) {
        currBlockMediaPrice = lastBlockMediaPrice;
        return currBlockMediaPrice;
    }

    vector<uint64_t> prices;
    if (mapBlockUserPrices.count(blockHeight - 1) != 0) {
        for (auto userPrice : mapBlockUserPrices[ blockHeight - 1 ]) {
            prices.push_back(userPrice.second);
        }
    }
    if (mapBlockUserPrices.count(blockHeight) != 0) {
        for (auto userPrice : mapBlockUserPrices[ blockHeight ]) {
                prices.push_back(userPrice.second);
        }
    }
    return ComputeMedianNumber(prices);
}

bool CConsecutiveBlockPrice::ExistBlockUserPrice(const int height, const CUserID &txUid) {
    if (mapBlockUserPrices.count(height) == 0)
        return false;

    return mapBlockUserPrices[ height ].count( txUid.ToString() );
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

        cbp.AddUserPrice(blockHeight, txUid, pp.GetPrice());
    }

    return true;
}

uint64_t CPricePointCache::ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType) {
    CConsecutiveBlockPrice cbp = mapCoinPricePointCache[ coinPriceType.ToString() ];
    uint64_t medianPrice = cbp.ComputeBlockMedianPrice(blockHeight);
    return medianPrice;
}

bool CDelegateCache::LoadTopDelegates() {
    delegateRegIds.clear();
    // TODO:
    return true;
}

bool CDelegateCache::ExistDelegate(const CRegID &delegateRegId) {
    if (delegateRegIds.empty()) {
        LoadTopDelegates();
    }

    return delegateRegIds.count(delegateRegId);
}

bool CDelegateCache::SetDelegateData(const CAccount &delegateAcct, CDbOpLog &operLog) {
    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - delegateAcct.receivedVotes);
    const CRegID &regId       = delegateAcct.regID;

    auto key             = std::make_pair(strVotes, regId);
    static uint8_t value = 1;

    operLog = CDbOpLog(voteRegIdCache.GetPrefixType(), key, value);

    return voteRegIdCache.SetData(key, value);
}

bool CDelegateCache::EraseDelegateData(const CAccountLog &delegateAcct, CDbOpLog &operLog) {
    static uint64_t maxNumber = 0xFFFFFFFFFFFFFFFF;
    string strVotes           = strprintf("%016x", maxNumber - delegateAcct.receivedVotes);
    const CRegID &regId       = delegateAcct.regID;

    auto oldKey             = std::make_pair(strVotes, regId);
    static uint8_t oldValue = 1;

    operLog = CDbOpLog(voteRegIdCache.GetPrefixType(), oldKey, oldValue);

    if (!voteRegIdCache.EraseData(oldKey)) {
        return false;
    }

    return true;
}

bool CDelegateCache::SetDelegateData(const CDbOpLog &operLog) {
    std::pair<string, CRegID> key;
    uint8_t value;
    operLog.Get(key, value);

    if (db_util::IsEmpty(key)) {
        return true;
    }

    if (!voteRegIdCache.SetData(key, value)) {
        return false;
    }

    return true;
}

bool CDelegateCache::EraseDelegateData(const CDbOpLog &operLog) {
    std::pair<string, CRegID> key;
    uint8_t value;
    operLog.Get(key, value);

    if (!voteRegIdCache.EraseData(key)) {
        return false;
    }

    return true;
}

string CTxUndo::ToString() const {
    string str;
    string strTxHash("txid:");
    strTxHash += txHash.GetHex();

    str += strTxHash + "\n";

    string strAccountLog("list account log:");
    for (auto iterLog : accountLogs) {
        strAccountLog += iterLog.ToString();
        strAccountLog += ";";
    }

    str += strAccountLog + "\n";

    string strDBOperLog("list LDB Oplog:");
    for (auto itemOpLogs : mapDbOpLogs) {
        strDBOperLog += strprintf("type:%d {", itemOpLogs.first);
        for (auto iterDbLog : itemOpLogs.second) {
            strDBOperLog += iterDbLog.ToString();
            strDBOperLog += ";";
        }
        strDBOperLog += "}";
    }

    str += strDBOperLog;

    return str;
}

bool CTxUndo::GetAccountOperLog(const CKeyID &keyId, CAccountLog &accountLog) {
    for (auto iterLog : accountLogs) {
        if (iterLog.keyID == keyId) {
            accountLog = iterLog;
            return true;
        }
    }
    return false;
}