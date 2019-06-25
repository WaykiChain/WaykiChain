// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_TXDB_H
#define PERSIST_TXDB_H

#include "accounts/account.h"
#include "accounts/id.h"
#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"
#include "json/json_spirit_value.h"

#include <map>
#include <vector>

using namespace std;
using namespace json_spirit;

class CBlock;
class CPricePoint;
class CCoinPriceType;
class CRegID;

// TODO: initialize pBase by constructor instead of SetBaseView.
class CTxMemCache {
private:
    map<uint256, UnorderedHashSet> mapBlockTxHashSet;  // map: BlockHash -> TxHashSet
    CTxMemCache *pBase;

public:
    CTxMemCache() : pBase(nullptr) {}
    CTxMemCache(CTxMemCache *pBaseIn) : pBase(pBaseIn) {}

public:
    bool HaveTx(const uint256 &txHash);
    bool IsContainBlock(const CBlock &block);

    bool AddBlockToCache(const CBlock &block);
    bool DeleteBlockFromCache(const CBlock &block);

    void Clear();
    void SetBaseView(CTxMemCache *pBaseIn) { pBase = pBaseIn; }
    void BatchWrite(const map<uint256, UnorderedHashSet> &mapBlockTxHashSetIn);
    void Flush(CTxMemCache *pBaseIn);
    void Flush();

    Object ToJsonObj() const;
    uint64_t GetSize();

    const map<uint256, UnorderedHashSet> &GetTxHashCache();
    void SetTxHashCache(const map<uint256, UnorderedHashSet> &mapCache);
};

// Price Points in 11 consecutive blocks
class CConsecutiveBlockPrice {
private:
    map<int, map<string, uint64_t>> mapBlockUserPrices;  // height -> { regId -> price }

public:
    void AddUserPrice(const int blockHeight, const CRegID &regId, const uint64_t price);
    // delete user price by specific block height.
    void DeleteUserPrice(const int blockHeight);
    bool ExistBlockUserPrice(const int blockHeight, const CRegID &regId);
    uint64_t ComputeBlockMedianPrice(const int blockHeight);
    uint64_t GetLastBlockMedianPrice();

public:
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);
};

class CPricePointCache {
private:
    uint64_t bcoinMedianPrice;  // against scoin
    uint64_t fcoinMedianPrice;  // against scoin

    map<string, CConsecutiveBlockPrice> mapCoinPricePointCache; // coinPriceType -> consecutiveBlockPrice

public:
    bool AddBlockPricePointInBatch(const int blockHeight, const CRegID &regId, const vector<CPricePoint> &pps);
    // delete block price point by specific block height.
    bool DeleteBlockPricePoint(const int blockHeight);

    void ComputeBlockMedianPrice(const int blockHeight);
    uint64_t GetBcoinMedianPrice() { return bcoinMedianPrice; }
    uint64_t GetFcoinMedianPrice() { return fcoinMedianPrice; }

private:
    uint64_t ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType);
};

class CTxUndo {
public:
    uint256 txHash;
    vector<CAccountLog> accountLogs;
    CDBOpLogMap dbOpLogMap; // dbName -> dbOpLogs

    IMPLEMENT_SERIALIZE(
        READWRITE(txHash);
        READWRITE(accountLogs);
        READWRITE(dbOpLogMap);
	)

public:
    bool GetAccountLog(const CKeyID &keyId, CAccountLog &accountLog);

    void Clear() {
        txHash = uint256();
        accountLogs.clear();
        dbOpLogMap.Clear();
    }

    string ToString() const;
};

#endif // PERSIST_TXDB_H