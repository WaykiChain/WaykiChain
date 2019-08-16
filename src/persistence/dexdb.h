// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DEX_H
#define PERSIST_DEX_H

#include <string>
#include <set>
#include <vector>

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
#include "entities/account.h"
#include "entities/dexorder.h"

using namespace std;

/*       type               prefixType                   key                            value                type             */
/*  ----------------   -------------------------  ---------------------------       ------------------   ------------------------ */
    /////////// DexDB
    // block orders: height generate_type txid -> active order
typedef CCompositeKVCache<dbk::DEX_BLOCK_ORDERS,  tuple<uint32_t, uint8_t, uint256>, CDEXOrderDetail>     DEXBlockOrdersCache;

// DEX_DB
namespace DEX_DB {
    //block order key: height generate_type txid
    typedef pair<DEXBlockOrdersCache::KeyType, DEXBlockOrdersCache::ValueType> BlockOrdersItem;
    typedef vector<BlockOrdersItem> BlockOrders;

    inline uint32_t GetHeight(const DEXBlockOrdersCache::KeyType &key) {
        return std::get<0>(key);
    }

    inline OrderGenerateType GetGenerateType(const DEXBlockOrdersCache::KeyType &key) {
        return (OrderGenerateType)std::get<1>(key);
    }

    inline const uint256& GetOrderId(const DEXBlockOrdersCache::KeyType &key) {
        return std::get<2>(key);
    }

    void OrderToJson(const uint256 &orderId, const CDEXOrderDetail &order, Object &obj);

    void BlockOrdersToJson(const BlockOrders &orderList, Object &obj);
};

class CDEXOrderListGetter {
public:
    string last_pos_info; // exec result
    bool    has_more;     // exec result
    uint32_t last_height;
    DEX_DB::BlockOrders orders; // exec result
private:
    CDBAccess &db_access;
public:
    CDEXOrderListGetter(CDBAccess &dbAccess): db_access(dbAccess) {
    }
    bool Execute(uint32_t fromHeight, uint32_t toHeight, const string &lastPosInfo, uint32_t maxCount);
};


class CDEXSysOrderListGetter {
public:
    DEX_DB::BlockOrders orders; // exec result
private:
    DEXBlockOrdersCache &db_cache;
    CDBAccess &db_access;
public:
    CDEXSysOrderListGetter(DEXBlockOrdersCache &dbCache)
        : db_cache(dbCache), db_access(*dbCache.GetDbAccessPtr()) {
    }    
    bool Execute(uint32_t height);

    void ToJson(Object &obj);
};

class CDexDBCache {
public:
    CDexDBCache() {}
    CDexDBCache(CDBAccess *pDbAccess) : activeOrderCache(pDbAccess), blockOrdersCache(pDbAccess) {};

public:
    bool GetActiveOrder(const uint256 &orderTxId, CDEXOrderDetail& activeOrder);
    bool HaveActiveOrder(const uint256 &orderTxId);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool UpdateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool EraseActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail &activeOrder);

    bool Flush() {
        activeOrderCache.Flush();
        blockOrdersCache.Flush();
        return true;
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        blockOrdersCache.SetBase(&pBaseIn->blockOrdersCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        activeOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
        blockOrdersCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return activeOrderCache.UndoDatas() &&
               blockOrdersCache.UndoDatas();
    }

    shared_ptr<CDEXSysOrderListGetter> CreateSysOrderListGetter() {
        assert(blockOrdersCache.GetBasePtr() == nullptr && "only support top level cache");
        return make_shared<CDEXSysOrderListGetter>(blockOrdersCache);
    }

    // shared_ptr<CDEXOrderListGetter> CreateOrderListGetter() {
    //     assert(blockOrdersCache.GetBasePtr() == nullptr && "only support top level cache");
    //     return make_shared<CDEXOrderListGetter>(blockOrdersCache);
    // }
private:
    DEXBlockOrdersCache::KeyType MakeBlockOrderKey(const uint256 &orderid, const CDEXOrderDetail &activeOrder) {
        return make_tuple(activeOrder.tx_cord.GetHeight(), (uint8_t)activeOrder.generate_type, orderid);
    }
private:
/*       type               prefixType                      key                        value                variable             */
/*  ----------------   -----------------------------  ---------------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositeKVCache< dbk::DEX_ACTIVE_ORDER,          uint256,                     CDEXOrderDetail >     activeOrderCache;
    DEXBlockOrdersCache    blockOrdersCache;
};

#endif //PERSIST_DEX_H
