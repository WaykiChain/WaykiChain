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

class CDBDexBlockList {
public:
    //block order key: height generate_type txid
    typedef std::tuple<uint32_t, uint8_t, uint256>  KeyType;
    typedef pair<KeyType, CDEXOrderDetail>          OrderListItem;
    typedef vector<OrderListItem>                   OrderList;
    static const dbk::PrefixType PREFIX_TYPE = dbk::DEX_BLOCK_ORDER;

    static KeyType MakeKey(const uint256 &orderid, const CDEXOrderDetail &activeOrder) {
        return make_tuple(activeOrder.tx_cord.GetHeight(), (uint8_t)activeOrder.generate_type, orderid);
    }    
public:
    OrderList list;

    const CDEXOrderDetail& GetOrder(const OrderListItem& item) {
        return item.second;
    }

    Object ToJson();
};


class CDEXOrderListGetter {
public:
    string last_pos_info; // exec result
    bool    has_more;     // exec result
    uint32_t last_height;
    CDBDexBlockList data_list; // exec result
private:
    CDBAccess &db_access;
public:
    CDEXOrderListGetter(CDBAccess &dbAccess): db_access(dbAccess) {
        assert(db_access.GetDbNameType() == dbk::GetDbNameEnumByPrefix(CDBDexBlockList::PREFIX_TYPE));
    }
    bool Execute(uint32_t fromHeight, uint32_t toHeight, const string &lastPosInfo, uint32_t maxCount);
};


class CDEXSysOrderListGetter {
public:
    CDBDexBlockList data_list; // exec result
private:
    CDBAccess &db_access;
public:
    CDEXSysOrderListGetter(CDBAccess &dbAccess): db_access(dbAccess) {
        assert(db_access.GetDbNameType() == dbk::GetDbNameEnumByPrefix(CDBDexBlockList::PREFIX_TYPE));
    }    
    bool Execute(uint32_t height);

    Object ToJson();
};

class CDexDBCache {
public:
    CDexDBCache() {}
    CDexDBCache(CDBAccess *pDbAccess) : activeOrderCache(pDbAccess), blockOrderCache(pDbAccess) {};

public:
    bool GetActiveOrder(const uint256 &orderTxId, CDEXOrderDetail& activeOrder);
    bool HaveActiveOrder(const uint256 &orderTxId);
    bool CreateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool UpdateActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail& activeOrder);
    bool EraseActiveOrder(const uint256 &orderTxId, const CDEXOrderDetail &activeOrder);

    bool Flush() {
        activeOrderCache.Flush();
        blockOrderCache.Flush();
        return true;
    }
    void SetBaseViewPtr(CDexDBCache *pBaseIn) {
        activeOrderCache.SetBase(&pBaseIn->activeOrderCache);
        blockOrderCache.SetBase(&pBaseIn->blockOrderCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        activeOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
        blockOrderCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return activeOrderCache.UndoDatas() &&
               blockOrderCache.UndoDatas();
    }
private:
/*       type               prefixType                      key                        value                variable             */
/*  ----------------   -----------------------------  ---------------------------  ------------------   ------------------------ */
    /////////// DexDB
    // order tx id -> active order
    CCompositeKVCache< dbk::DEX_ACTIVE_ORDER,          uint256,                     CDEXOrderDetail >     activeOrderCache;
    CCompositeKVCache< CDBDexBlockList::PREFIX_TYPE,   CDBDexBlockList::KeyType,    CDEXOrderDetail >     blockOrderCache;
};

#endif //PERSIST_DEX_H
