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
#include <optional>

using namespace std;

/*       type               prefixType                   key                            value                type             */
/*  ----------------   -------------------------  ---------------------------       ------------------   ------------------------ */
    /////////// DexDB
    // block orders: height generate_type txid -> active order
typedef CCompositeKVCache<dbk::DEX_BLOCK_ORDERS,  tuple<CFixedUInt32, uint8_t, uint256>, dex::CDEXOrderDetail>     DEXBlockOrdersCache;

// DEX_DB
namespace DEX_DB {
    //block order key: height generate_type txid
    typedef pair<DEXBlockOrdersCache::KeyType, DEXBlockOrdersCache::ValueType> BlockOrdersItem;
    typedef vector<BlockOrdersItem> BlockOrders;

    inline uint32_t GetHeight(const DEXBlockOrdersCache::KeyType &key) {
        return std::get<0>(key).value;
    }

    inline dex::OrderGenerateType GetGenerateType(const DEXBlockOrdersCache::KeyType &key) {
        return (dex::OrderGenerateType)std::get<1>(key);
    }

    inline const uint256& GetOrderId(const DEXBlockOrdersCache::KeyType &key) {
        return std::get<2>(key);
    }

    // return err str if err happens
    shared_ptr<string> ParseLastPos(const string &lastPosInfo, DEXBlockOrdersCache::KeyType &lastKey);

    shared_ptr<string> MakeLastPos(const DEXBlockOrdersCache::KeyType &lastKey, string &lastPosInfo);

    void OrderToJson(const uint256 &orderId, const dex::CDEXOrderDetail &order, Object &obj);

    void BlockOrdersToJson(const BlockOrders &orderList, Object &obj);
};

class CDexDBCache {
public:
    CDexDBCache() {}
    CDexDBCache(CDBAccess *pDbAccess)
        : activeOrderCache(pDbAccess),
          blockOrdersCache(pDbAccess),
          operator_detail_cache(pDbAccess),
          operator_owner_map_cache(pDbAccess),
          operator_last_id_cache(pDbAccess) {};


public:
    bool GetActiveOrder(const uint256 &orderTxId, dex::CDEXOrderDetail& activeOrder);
    bool HaveActiveOrder(const uint256 &orderTxId);
    bool CreateActiveOrder(const uint256 &orderTxId, const dex::CDEXOrderDetail& activeOrder);
    bool UpdateActiveOrder(const uint256 &orderTxId, const dex::CDEXOrderDetail& activeOrder);
    bool EraseActiveOrder(const uint256 &orderTxId, const dex::CDEXOrderDetail &activeOrder);

    bool IncDexID(DexID &id);
    bool GetDexOperator(const DexID &id, DexOperatorDetail& detail);
    bool GetDexOperatorByOwner(const CRegID &regid, DexID &id, DexOperatorDetail& detail);
    bool HaveDexOperator(const DexID &id);
    bool HasDexOperatorByOwner(const CRegID &regid);
    bool CreateDexOperator(const DexID &id, const DexOperatorDetail& detail);
    bool UpdateDexOperator(const DexID &id, const DexOperatorDetail& old_detail,
        const DexOperatorDetail& detail);

    bool Flush() {
        activeOrderCache.Flush();
        blockOrdersCache.Flush();
        operator_detail_cache.Flush(),
        operator_owner_map_cache.Flush();
        operator_last_id_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return activeOrderCache.GetCacheSize() +
            blockOrdersCache.GetCacheSize() +
            operator_detail_cache.GetCacheSize() +
            operator_owner_map_cache.GetCacheSize() +
            operator_last_id_cache.GetCacheSize();
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        blockOrdersCache.SetBase(&pBaseIn->blockOrdersCache);
        operator_detail_cache.SetBase(&pBaseIn->operator_detail_cache);
        operator_owner_map_cache.SetBase(&pBaseIn->operator_owner_map_cache);
        operator_last_id_cache.SetBase(&pBaseIn->operator_last_id_cache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        activeOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
        blockOrdersCache.SetDbOpLogMap(pDbOpLogMapIn);
        operator_detail_cache.SetDbOpLogMap(pDbOpLogMapIn);
        operator_owner_map_cache.SetDbOpLogMap(pDbOpLogMapIn);
        operator_last_id_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        activeOrderCache.RegisterUndoFunc(undoDataFuncMap);
        blockOrdersCache.RegisterUndoFunc(undoDataFuncMap);
        operator_detail_cache.RegisterUndoFunc(undoDataFuncMap);
        operator_owner_map_cache.RegisterUndoFunc(undoDataFuncMap);
        operator_last_id_cache.RegisterUndoFunc(undoDataFuncMap);
    }

private:
    DEXBlockOrdersCache::KeyType MakeBlockOrderKey(const uint256 &orderid, const dex::CDEXOrderDetail &activeOrder) {
        return make_tuple(CFixedUInt32(activeOrder.tx_cord.GetHeight()), (uint8_t)activeOrder.generate_type, orderid);
    }
public:
/*       type               prefixType                      key                        value                variable             */
/*  ----------------   -----------------------------  ---------------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositeKVCache< dbk::DEX_ACTIVE_ORDER,          uint256,                     dex::CDEXOrderDetail >     activeOrderCache;
    DEXBlockOrdersCache    blockOrdersCache;
    CCompositeKVCache< dbk::DEX_OPERATOR_DETAIL,       std::optional<CVarIntValue<DexID>> , DexOperatorDetail >   operator_detail_cache;
    CCompositeKVCache< dbk::DEX_OPERATOR_OWNER_MAP,    CRegIDKey,               std::optional<CVarIntValue<DexID>>> operator_owner_map_cache;
    CSimpleKVCache<dbk::DEX_OPERATOR_LAST_ID, CVarIntValue<DexID>> operator_last_id_cache;

};

#endif //PERSIST_DEX_H
