// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_TXDB_H
#define PERSIST_TXDB_H

#include "accounts/account.h"
#include "Accounts/id.h"
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
class CUserID;

// TODO: initialize pBase by constructor instead of SetBaseView.
class CTransactionCache {
private:
    map<uint256, UnorderedHashSet> mapBlockTxHashSet;  // map: BlockHash ->TxHashSet
    CTransactionCache *pBase;

public:
    CTransactionCache() : pBase(nullptr) {}
    CTransactionCache(CTransactionCache *pBaseIn) : pBase(pBaseIn) {}

public:
    bool HaveTx(const uint256 &txHash);
    bool IsContainBlock(const CBlock &block);

    bool AddBlockToCache(const CBlock &block);
    bool DeleteBlockFromCache(const CBlock &block);

    void Clear();
    void SetBaseView(CTransactionCache *pBaseIn) { pBase = pBaseIn; }
    void BatchWrite(const map<uint256, UnorderedHashSet> &mapBlockTxHashSetIn);
    void Flush(CTransactionCache *pBaseIn);
    void Flush();

    Object ToJsonObj() const;
    int GetSize();

    const map<uint256, UnorderedHashSet> &GetTxHashCache();
    void SetTxHashCache(const map<uint256, UnorderedHashSet> &mapCache);
};

// Price Points in 11 consecutive blocks
class CConsecutiveBlockPrice {
private:
    map<int, map<string, uint64_t>>mapBlockUserPrices;    // height -> { strUid -> price }
    int lastBlockHeight;
    int currBlockHeight;
    uint64_t lastBlockMediaPrice;
    uint64_t currBlockMediaPrice;

public:
    void AddUserPrice(const int height, const CUserID &txUid, const uint64_t price);
    bool ExistBlockUserPrice(const int height, const CUserID &txUid);
    uint64_t ComputeBlockMedianPrice(const int blockHeight);
    uint64_t GetLastBlockMedianPrice();

public:
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);
};

class CPricePointCache {
private:
    uint64_t bcoinMedianPrice; //against scoin
    uint64_t fcoinMedianPrice; //against scoin

    map<string, CConsecutiveBlockPrice> mapCoinPricePointCache; // coinPriceType -> consecutiveBlockPrice

public:
    bool AddBlockPricePointInBatch(const int blockHeight, const CUserID &txUid, const vector<CPricePoint> &pps);
    uint64_t ComputeBlockMedianPrice(const int blockHeight, CCoinPriceType coinPriceType);

    uint64_t GetBcoinMedianPrice() { return bcoinMedianPrice; }
    uint64_t GetFcoinMedianPrice() { return fcoinMedianPrice; }
};

/* Top 11 delegates */
class CDelegateCache {
public:
    CDBMultiValueCache<std::tuple<uint64_t /* votes */, CRegID>, uint8_t> voteRegIdCache;

public:
    CDelegateCache(){};
    CDelegateCache(CDBAccess *pDbAccess) : voteRegIdCache(pDbAccess, dbk::VOTE){};
    CDelegateCache(CDelegateCache *pBaseIn) : voteRegIdCache(pBaseIn->voteRegIdCache){};

    bool LoadTopDelegates();
    bool ExistDelegate(const CRegID &regId);

private:
    set<CRegID> delegateRegIds;
};

class CTxUndo {
public:
    uint256 txHash;
    vector<CAccountLog> accountLogs;
    map<DbOpLogType, CDbOpLogs> mapDbOpLogs;

    IMPLEMENT_SERIALIZE(
        READWRITE(txHash);
        READWRITE(accountLogs);
		/*TODO:
        //READWRITE(mapDbOpLogs);*/
	)

public:
    bool GetAccountOperLog(const CKeyID &keyId, CAccountLog &accountLog);

    void Clear() {
        txHash = uint256();
        accountLogs.clear();
        mapDbOpLogs.clear();
    }

    string ToString() const;
};

#endif // PERSIST_TXDB_H