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
#include "commons/leb128.h"
#include "persistence/dbaccess.h"
#include "entities/account.h"
#include "entities/dexorder.h"

using namespace std;

/*       type               prefixType                   key                            value                type             */
/*  ----------------   -------------------------  ---------------------------       ------------------   ------------------------ */
    /////////// DexDB
    // block orders: height generate_type txid -> active order
typedef CCompositeKVCache<dbk::DEX_BLOCK_ORDERS,  tuple<CFixedUInt32, uint8_t, uint256>, CDEXOrderDetail>     DEXBlockOrdersCache;

typedef uint32_t DexOperatorID;

// dex operator
struct DexOperatorDetail {
    CUserID owner_uid;
    CUserID matcher_uid;
    string portal_url;
    string memo;

    IMPLEMENT_SERIALIZE(
        READWRITE(owner_uid);
        READWRITE(matcher_uid);
        READWRITE(portal_url);
        READWRITE(memo);
    )

    bool IsEmpty() const {
        return owner_uid.IsEmpty() && matcher_uid.IsEmpty() && portal_url.empty() && memo.empty();
    }

    void SetEmpty() {
        owner_uid.SetEmpty(); matcher_uid.SetEmpty(); portal_url = ""; memo = "";
    }
};

// DEX_DB
namespace DEX_DB {
    //block order key: height generate_type txid
    typedef pair<DEXBlockOrdersCache::KeyType, DEXBlockOrdersCache::ValueType> BlockOrdersItem;
    typedef vector<BlockOrdersItem> BlockOrders;

    inline uint32_t GetHeight(const DEXBlockOrdersCache::KeyType &key) {
        return std::get<0>(key).value;
    }

    inline OrderGenerateType GetGenerateType(const DEXBlockOrdersCache::KeyType &key) {
        return (OrderGenerateType)std::get<1>(key);
    }

    inline const uint256& GetOrderId(const DEXBlockOrdersCache::KeyType &key) {
        return std::get<2>(key);
    }

    // return err str if err happens
    shared_ptr<string> ParseLastPos(const string &lastPosInfo, DEXBlockOrdersCache::KeyType &lastKey);

    shared_ptr<string> MakeLastPos(const DEXBlockOrdersCache::KeyType &lastKey, string &lastPosInfo);

    void OrderToJson(const uint256 &orderId, const CDEXOrderDetail &order, Object &obj);

    void BlockOrdersToJson(const BlockOrders &orderList, Object &obj);
};

class CDEXOrdersGetter {
public:
    uint32_t    begin_height    = 0;        // the begin block height of returned orders
    uint32_t    end_height      = 0;        // the end block height of returned orders
    bool        has_more        = false;    // has more orders in db
    DEXBlockOrdersCache::KeyType  last_key;       // the key of last position to get more orders
    DEX_DB::BlockOrders orders;             // the returned orders
private:
    DEXBlockOrdersCache &db_cache;
    CDBAccess &db_access;
public:
    CDEXOrdersGetter(DEXBlockOrdersCache &dbCache)
        : db_cache(dbCache), db_access(*dbCache.GetDbAccessPtr()) {
    }

    bool Execute(uint32_t fromHeight, uint32_t toHeight, uint32_t maxCount, const DEXBlockOrdersCache::KeyType &lastPosInfo);
    void ToJson(Object &obj);
};


class CDEXSysOrdersGetter {
public:
    DEX_DB::BlockOrders orders; // exec result
private:
    DEXBlockOrdersCache &db_cache;
    CDBAccess &db_access;
public:
    CDEXSysOrdersGetter(DEXBlockOrdersCache &dbCache)
        : db_cache(dbCache), db_access(*dbCache.GetDbAccessPtr()) {
    }
    bool Execute(uint32_t height);

    void ToJson(Object &obj);
};

class CDexDBCache {
public:
    CDexDBCache() {}
    CDexDBCache(CDBAccess *pDbAccess)
        : activeOrderCache(pDbAccess),
          blockOrdersCache(pDbAccess),
          operator_detail_cache(pDbAccess),
          operator_last_id_cache(pDbAccess){};

public:
    bool GetActiveOrder(const uint256 &orderTxId, CDEXOrderDetail& activeOrder);
    bool HaveActiveOrder(const uint256 &orderTxId);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool UpdateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool EraseActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail &activeOrder);

    bool IncDexOperatorId(DexOperatorID &id);
    bool GetDexOperator(const DexOperatorID &id, DexOperatorDetail& detail);
    bool HaveDexOperator(const DexOperatorID &id);
    bool CreateDexOperator(const DexOperatorID &id, const DexOperatorDetail& detail);
    bool UpdateDexOperator(const DexOperatorID &id, const DexOperatorDetail& detail);

    bool Flush() {
        activeOrderCache.Flush();
        blockOrdersCache.Flush();
        operator_detail_cache.Flush(),
        operator_last_id_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return activeOrderCache.GetCacheSize() +
            blockOrdersCache.GetCacheSize();
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        blockOrdersCache.SetBase(&pBaseIn->blockOrdersCache);
        operator_detail_cache.SetBase(&pBaseIn->operator_detail_cache);
        operator_last_id_cache.SetBase(&pBaseIn->operator_last_id_cache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        activeOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
        blockOrdersCache.SetDbOpLogMap(pDbOpLogMapIn);
        operator_detail_cache.SetDbOpLogMap(pDbOpLogMapIn);
        operator_last_id_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        activeOrderCache.RegisterUndoFunc(undoDataFuncMap);
        blockOrdersCache.RegisterUndoFunc(undoDataFuncMap);
        operator_detail_cache.RegisterUndoFunc(undoDataFuncMap);
        operator_last_id_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CDEXOrdersGetter> CreateOrdersGetter() {
        assert(blockOrdersCache.GetBasePtr() == nullptr && "only support top level cache");
        return make_shared<CDEXOrdersGetter>(blockOrdersCache);
    }

    shared_ptr<CDEXSysOrdersGetter> CreateSysOrdersGetter() {
        assert(blockOrdersCache.GetBasePtr() == nullptr && "only support top level cache");
        return make_shared<CDEXSysOrdersGetter>(blockOrdersCache);
    }
private:
    DEXBlockOrdersCache::KeyType MakeBlockOrderKey(const uint256 &orderid, const CDEXOrderDetail &activeOrder) {
        return make_tuple(CFixedUInt32(activeOrder.tx_cord.GetHeight()), (uint8_t)activeOrder.generate_type, orderid);
    }
private:
/*       type               prefixType                      key                        value                variable             */
/*  ----------------   -----------------------------  ---------------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositeKVCache< dbk::DEX_ACTIVE_ORDER,          uint256,                     CDEXOrderDetail >     activeOrderCache;
    DEXBlockOrdersCache    blockOrdersCache;
    CCompositeKVCache< dbk::DEX_OPERATOR_DETAIL,       CFixedLeb128<DexOperatorID>, DexOperatorDetail >         operator_detail_cache;

    CSimpleKVCache<dbk::DEX_OPERATOR_LAST_ID, CVarIntValue<DexOperatorID>> operator_last_id_cache;
};

#endif //PERSIST_DEX_H
