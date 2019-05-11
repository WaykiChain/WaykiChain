// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain Developers
// Copyright (c) 2019- The WaykiChain CoreDev
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef _TXDB_H_
#define _TXDB_H_

#include "vm/appaccount.h"
#include "commons/serialize.h"

#include <map>
#include <vector>
using namespace std;

class CAccount;
class CKeyID;
class uint256;
struct CDiskTxPos;
class CVmOperate;


class CTransactionDBCache : public CTransactionDBView {
protected:
    CTransactionDBView *pBase;

private:
    // CTransactionDBCache(CTransactionDBCache &transactionView);
    map<uint256, UnorderedHashSet> mapTxHashByBlockHash;  // key:block hash  value:tx hash
    bool IsInMap(const map<uint256, UnorderedHashSet> &mMap, const uint256 &hash) const;

public:
    CTransactionDBCache(CTransactionDBView &pBaseIn) : pBase(&pBaseIn) {};

    bool HaveTx(const uint256 &txHash);
    bool IsContainBlock(const CBlock &block);
    bool AddBlockToCache(const CBlock &block);
    bool DeleteBlockFromCache(const CBlock &block);
    map<uint256, UnorderedHashSet> GetTxHashCache();
    bool BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHashIn);
    void AddTxHashCache(const uint256 &blockHash, const UnorderedHashSet &vTxHash);
    bool Flush();
    void Clear();
    Object ToJsonObj() const;
    int GetSize();
    void SetBaseView(CTransactionDBView *pBaseIn) { pBase = pBaseIn; };
    const map<uint256, UnorderedHashSet> &GetCacheMap();
    void SetCacheMap(const map<uint256, UnorderedHashSet> &mapCache);
};


#endif