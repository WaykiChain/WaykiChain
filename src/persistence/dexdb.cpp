// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "main.h"
#include <optional>
#include <functional>

using namespace dex;

///////////////////////////////////////////////////////////////////////////////
// class DEX_DB

static uint32_t MAIN_DEX_ID = 0;

void DEX_DB::OrderToJson(const uint256 &orderId, const CDEXOrderDetail &order, Object &obj) {
        obj.push_back(Pair("order_id", orderId.ToString()));
        order.ToJson(obj);
}

void DEX_DB::BlockOrdersToJson(const BlockOrders &orders, Object &obj) {
    obj.push_back(Pair("count", (int64_t)orders.size()));
    Array array;
    for (auto &item : orders) {
        Object objItem;
        OrderToJson(GetOrderId(item.first), item.second, objItem);
        array.push_back(objItem);
    }
    obj.push_back(Pair("orders", array));
}

shared_ptr<string> DEX_DB::ParseLastPos(const string &lastPosInfo, DEXBlockOrdersCache::KeyType &lastKey) {

    CDataStream ds(lastPosInfo, SER_DISK, CLIENT_VERSION);
    uint256 lastBlockHash;
    ds >> lastBlockHash >> lastKey;
    uint32_t lastHeight = DEX_DB::GetHeight(lastKey);
    CBlockIndex *pBlockIndex = chainActive[lastHeight];
    if (pBlockIndex == nullptr)
        return make_shared<string>(strprintf("The last_pos_info is not contained in active chains,"
            " last_height=%d, tip_height=%d", lastHeight, chainActive.Height()));
    if (pBlockIndex->GetBlockHash() != lastBlockHash)
        return make_shared<string>(strprintf("The block of height in last_pos_info does not match with the active block,"
            " height=%d, last_block_hash=%s, cur_height_block_hash=%s",
            lastHeight, lastBlockHash.ToString(), pBlockIndex->GetBlockHash().ToString()));
    return nullptr;
}

shared_ptr<string> DEX_DB::MakeLastPos(const DEXBlockOrdersCache::KeyType &lastKey, string &lastPosInfo) {
    uint32_t lastHeight = DEX_DB::GetHeight(lastKey);
    CBlockIndex *pBlockIndex = chainActive[lastHeight];
    if (pBlockIndex == nullptr)
        return make_shared<string>(strprintf("The block of lastKey is not contained in active chains,"
            " last_height=%d, tip_height=%d", lastHeight, chainActive.Height()));

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << pBlockIndex->GetBlockHash() << lastKey;
    lastPosInfo = ds.str();
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// class CDexDBCache

bool CDexDBCache::GetActiveOrder(const uint256 &orderId, CDEXOrderDetail &activeOrder) {
    return activeOrderCache.GetData(orderId, activeOrder);
}

bool CDexDBCache::HaveActiveOrder(const uint256 &orderId) { return activeOrderCache.HasData(orderId); }

bool CDexDBCache::CreateActiveOrder(const uint256 &orderId, const CDEXOrderDetail &activeOrder) {
    if (activeOrderCache.HasData(orderId)) {
        return ERRORMSG("CreateActiveOrder, the order is existed! order_id=%s, order=%s\n", orderId.GetHex(),
                        activeOrder.ToString());
    }

    return activeOrderCache.SetData(orderId, activeOrder)
        && blockOrdersCache.SetData(MakeBlockOrderKey(orderId, activeOrder), activeOrder);
}

bool CDexDBCache::UpdateActiveOrder(const uint256 &orderId, const CDEXOrderDetail &activeOrder) {
    return activeOrderCache.SetData(orderId, activeOrder)
        && blockOrdersCache.SetData(MakeBlockOrderKey(orderId, activeOrder), activeOrder);
}

bool CDexDBCache::EraseActiveOrder(const uint256 &orderId, const CDEXOrderDetail &activeOrder) {
    return activeOrderCache.EraseData(orderId)
        && blockOrdersCache.EraseData(MakeBlockOrderKey(orderId, activeOrder));
}

bool CDexDBCache::IncDexID(DexID &id) {
    decltype(operator_last_id_cache)::ValueType idVariant;
    operator_last_id_cache.GetData(idVariant);
    DexID &newId = idVariant.get();
    if (newId == ULONG_MAX)
        return ERRORMSG("dex operator id is inc to max! last_id=%ul\n", newId);
    newId++;

    if (operator_last_id_cache.SetData(idVariant)) {
        id = newId;
        return true;
    }
    return false;
}

bool Dex0(DexOperatorDetail& detail){

    CRegID regid = SysCfg().GetDex0OwnerRegId();
    detail.owner_regid =  regid;
    detail.fee_receiver_regid = regid;
    detail.name = "wayki-dex";
    detail.portal_url = "https://dex.waykichain.com";
    detail.order_open_mode = OpenMode::PUBLIC;
    detail.taker_fee_ratio = 40000;
    detail.maker_fee_ratio = 40000;
    detail.order_open_dexop_set = {};
    detail.activated = true;

    return true;
}

bool CDexDBCache::GetDexOperator(const DexID &id, DexOperatorDetail& detail) {
    decltype(operator_detail_cache)::KeyType idKey(id);
    bool result =  operator_detail_cache.GetData(idKey, detail);
    if (result)
        return result;
    if (id == MAIN_DEX_ID )
        return Dex0(detail);
    return result;

}

bool CDexDBCache::GetDexOperatorByOwner(const CRegID &regid, DexID &idOut, DexOperatorDetail& detail) {
    decltype(operator_owner_map_cache)::ValueType dexID;
    if (operator_owner_map_cache.GetData(regid, dexID)) {
        idOut = dexID.value().get();
        return GetDexOperator(dexID.value().get(), detail);
    }else {
        CRegID sysRegId = SysCfg().GetDex0OwnerRegId();
        if (sysRegId == regid) {
            idOut = MAIN_DEX_ID;
            bool result = GetDexOperator(idOut ,detail);
            if (result && detail.owner_regid == regid)
                return result;
        }
    }
    return false;
}

bool CDexDBCache::HaveDexOperator(const DexID &id) {
    if (id == MAIN_DEX_ID )
        return true;
    decltype(operator_detail_cache)::KeyType idKey(id);
    return operator_detail_cache.HasData(idKey);
}

bool CDexDBCache::HasDexOperatorByOwner(const CRegID &regid) {
     bool dbHave = operator_owner_map_cache.HasData(regid);

     if (!dbHave){
         CRegID sysRegId = SysCfg().GetDex0OwnerRegId();
         if (sysRegId == regid){
             DexOperatorDetail detail;
             bool b = GetDexOperator(MAIN_DEX_ID , detail);
             if (b && detail.owner_regid == regid)
                 return true;
         }
     }

     return dbHave;

}

bool CDexDBCache::CreateDexOperator(const DexID &id, const DexOperatorDetail& detail) {
    decltype(operator_detail_cache)::KeyType idKey(id);
    if (operator_detail_cache.HasData(idKey)) {
        return ERRORMSG("the dex operator is existed! id=%s\n", id);
    }

    if (HasDexOperatorByOwner(detail.owner_regid)) {
        return ERRORMSG("the owner already has a dex operator! owner=%s\n", detail.owner_regid.ToString());
    }

    return  operator_detail_cache.SetData(idKey, detail) &&
            operator_owner_map_cache.SetData(detail.owner_regid, id);
}

bool CDexDBCache::UpdateDexOperator(const DexID &id, const DexOperatorDetail& old_detail,
    const DexOperatorDetail& detail) {
    decltype(operator_detail_cache)::KeyType idKey(id);
    std::optional idValue{CVarIntValue<DexID>(id)};
    if (old_detail.owner_regid != detail.owner_regid) {
        if (!operator_owner_map_cache.EraseData(old_detail.owner_regid) ||
            !operator_owner_map_cache.SetData(detail.owner_regid, idValue)){
            return false;
        }

    }
    return operator_detail_cache.SetData(idKey, detail);
}
