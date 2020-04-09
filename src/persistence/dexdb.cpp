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
// class CDEXOrdersGetter

class CDexOrderIt {
public:
    DEXBlockOrdersCache::KeyType key;
    DEXBlockOrdersCache::ValueType value;
    bool is_valid;
protected:
    DEXBlockOrdersCache &db_cache;
    CFixedUInt32 begin_height;
    CFixedUInt32 end_height;
public:
    CDexOrderIt(DEXBlockOrdersCache &dbCache, const CFixedUInt32 &beginHeight, const CFixedUInt32 &endHeight)
        : key(),
          value(),
          is_valid(false),
          db_cache(dbCache),
          begin_height(beginHeight),
          end_height(endHeight){}
};

class CDBDexOrderIt: public CDexOrderIt {
private:
    shared_ptr<leveldb::Iterator> p_db_it;
    string last_pos_key;
    string prefix;
public:
    using CDexOrderIt::CDexOrderIt;

    bool First(DEXBlockOrdersCache::KeyType lastPosKey) {
        p_db_it = db_cache.GetDbAccessPtr()->NewIterator();
        prefix = dbk::GetKeyPrefix(DEXBlockOrdersCache::PREFIX_TYPE);
        last_pos_key = dbk::GenDbKey(DEXBlockOrdersCache::PREFIX_TYPE, lastPosKey);
        p_db_it->Seek(last_pos_key);
        if (p_db_it->Valid() && p_db_it->key() == last_pos_key) {
            p_db_it->Next();
        }
        return Parse();
    }

    bool First() {
        return First(make_tuple(begin_height, 0, uint256()));
    }

    bool Next() {
        p_db_it->Next();
        return Parse();
    }

    bool IsValid() {return is_valid;}
private:
    inline bool Parse() {
        is_valid = false;
        if (!p_db_it->Valid() || !p_db_it->key().starts_with(prefix)) return false;

        const leveldb::Slice &slKey = p_db_it->key();
        const leveldb::Slice &slValue = p_db_it->value();
        if (!ParseDbKey(slKey, DEXBlockOrdersCache::PREFIX_TYPE, key)) {
            throw runtime_error(strprintf("CDBDexOrderIt::Parse db key error! key=%s", HexStr(slKey.ToString())));
        }

        const CFixedUInt32 &curHeight = std::get<0>(key);
        if ( curHeight < begin_height || curHeight > end_height)
            return false;

        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            throw runtime_error(strprintf("CDBDexOrderIt::Parse db value error! %s", HexStr(slValue.ToString())));
        }
        is_valid = true;
        return true;
    }
};

class CMapDexOrderIt: public CDexOrderIt {
private:
    DEXBlockOrdersCache::Iterator map_it;
public:
    using CDexOrderIt::CDexOrderIt;

    bool First(DEXBlockOrdersCache::KeyType lastPosKey) {
        map_it = db_cache.GetMapData().upper_bound(lastPosKey);
        return Parse();
    }

    bool First() {
        return First(make_tuple(begin_height, 0, uint256()));
    }

    bool Next() {
        map_it++;
        return Parse();
    }

    bool IsValid() {return is_valid;}
private:
    inline bool Parse() {
        is_valid = false;
        if (map_it == db_cache.GetMapData().end())  return false;
        key = map_it->first;
        const CFixedUInt32 &curHeight = std::get<0>(key);
        if ( curHeight < begin_height || curHeight > end_height)
            return false;
        value = map_it->second;
        return true;
    }
};

bool CDEXOrdersGetter::Execute(uint32_t beginHeightIn, uint32_t endHeightIn, uint32_t maxCount, const DEXBlockOrdersCache::KeyType &lastKey) {

    assert(orders.size() == 0 && "Can only execute 1 times");
    CFixedUInt32 beginHeight(beginHeightIn);
    CFixedUInt32 endHeight(endHeightIn);
    CMapDexOrderIt mapIt(db_cache, beginHeight, endHeight);
    CDBDexOrderIt dbIt(db_cache, beginHeight, endHeight);

    if (!db_util::IsEmpty(lastKey)) {
        mapIt.First(lastKey);
        dbIt.First(lastKey);
    } else {
        mapIt.First();
        dbIt.First();
    }
    //auto mapIt = mapRangePair.first;
    while(mapIt.IsValid() || dbIt.IsValid()) {
        bool isMapData = true, isSameKey = false;
        if (mapIt.IsValid() && dbIt.IsValid()) {
            if (dbIt.key < mapIt.key) {
                isMapData = false;
            } else if (dbIt.key > mapIt.key) { // dbIt.key >= mapIt.key
                isMapData = true;
            } else {// dbIt.key == mapIt.key
                isMapData = true;
                isSameKey = true;
            }
        } else if (mapIt.IsValid()) {
            isMapData = true;
        } else { // dbIt.IsValid())
            isMapData = false;
        }

        shared_ptr<DEX_DB::BlockOrdersItem> pItem = nullptr;
        if (isMapData) {
            if (!mapIt.value.IsEmpty()) {
                pItem = make_shared<DEX_DB::BlockOrdersItem>(mapIt.key, mapIt.value);
            } // else ignore
            mapIt.Next();
            if (isSameKey) {
                assert(dbIt.IsValid());
                dbIt.Next();
            }
        } else { // use db
            pItem = make_shared<DEX_DB::BlockOrdersItem>(dbIt.key, dbIt.value);
            dbIt.Next();
        }
        if (pItem != nullptr) {
            if (maxCount != 0 && orders.size() >= maxCount) {
                has_more = true;
                break;
            }
            orders.push_back(*pItem);
        }
    }
    if (!orders.empty()) {
        begin_height = DEX_DB::GetHeight(orders.front().first);
        last_key = orders.back().first;
        end_height = DEX_DB::GetHeight(orders.back().first);
    }
    return true;
}

void CDEXOrdersGetter::ToJson(Object &obj) {
    DEX_DB::BlockOrdersToJson(orders, obj);
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSysOrdersGetter

//CDBDexSysOrderIt
class CDBDexSysOrderIt {
public:
    DEXBlockOrdersCache::KeyType key;
    DEXBlockOrdersCache::ValueType value;
private:
    shared_ptr<leveldb::Iterator> p_db_it;
    CFixedUInt32 height;
    bool is_valid;
    string prefix;
public:
    CDBDexSysOrderIt(CDBAccess &dbAccess, const CFixedUInt32 &heightIn)
        : key(), value(), height(heightIn), is_valid(false) {

        p_db_it = dbAccess.NewIterator();
        prefix = dbk::GenDbKey(DEXBlockOrdersCache::PREFIX_TYPE, make_pair(height, (uint8_t)SYSTEM_GEN_ORDER));
    }

    bool First() {
        p_db_it->Seek(prefix);
        return Parse();
    }

    bool Next() {
        p_db_it->Next();
        return Parse();
    }

    bool IsValid() {return is_valid;}
private:
    inline bool Parse() {
        is_valid = false;
        if (!p_db_it->Valid() || !p_db_it->key().starts_with(prefix)) return false;

        const leveldb::Slice &slKey = p_db_it->key();
        const leveldb::Slice &slValue = p_db_it->value();
        if (!ParseDbKey(slKey, DEXBlockOrdersCache::PREFIX_TYPE, key)) {
            throw runtime_error(strprintf("CDBDexSysOrderIt::Parse db key error! key=%s", HexStr(slKey.ToString())));
        }
        assert(std::get<0>(key) == height || DEX_DB::GetGenerateType(key) == (uint8_t)SYSTEM_GEN_ORDER);

        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            throw runtime_error(strprintf("CDBDexSysOrderIt::Parse db value error! %s", HexStr(slValue.ToString())));
        }
        is_valid = true;
        return true;
    }
};

class CMapDexSysOrderIt {
public:
    DEXBlockOrdersCache::KeyType key;
    DEXBlockOrdersCache::ValueType value;
private:
    DEXBlockOrdersCache::Map &data_map;
    DEXBlockOrdersCache::Iterator map_it;
    CFixedUInt32 height;
    bool is_valid;
public:
    CMapDexSysOrderIt(DEXBlockOrdersCache &dbCache, const CFixedUInt32 &heightIn)
        : key(), value(), data_map(dbCache.GetMapData()), map_it(data_map.end()), height(heightIn), is_valid(false) {}

    bool First() {
        map_it = data_map.upper_bound(make_tuple(height, (uint8_t)SYSTEM_GEN_ORDER, uint256()));
        return Parse();
    }
    bool Next() {
        map_it++;
        return Parse();
    }

    bool IsValid() {return is_valid;}
private:
    inline bool Parse() {
        is_valid = false;
        if (map_it == data_map.end())  return false;
        key = map_it->first;
        if (std::get<0>(key) != height || DEX_DB::GetGenerateType(key) != SYSTEM_GEN_ORDER)
            return false;
        value = map_it->second;
        return true;
    }
};

bool CDEXSysOrdersGetter::Execute(uint32_t heightIn) {

    CFixedUInt32 height(heightIn);
    CMapDexSysOrderIt mapIt(db_cache, height);
    CDBDexSysOrderIt dbIt(db_access, height);
    mapIt.First();
    dbIt.First();
    //auto mapIt = mapRangePair.first;
    while(mapIt.IsValid() || dbIt.IsValid()) {
        bool isMapData = true, isSameKey = false;
        if (mapIt.IsValid() && dbIt.IsValid()) {
            if (dbIt.key < mapIt.key) {
                isMapData = false;
            } else if (dbIt.key > mapIt.key) { // dbIt.key >= mapIt.key
                isMapData = true;
            } else {// dbIt.key == mapIt.key
                isMapData = true;
                isSameKey = true;
            }
        } else if (mapIt.IsValid()) {
            isMapData = true;
        } else { // dbIt.IsValid())
            isMapData = false;
        }

        if (isMapData) {
            if (!mapIt.value.IsEmpty()) {
                orders.push_back(make_pair(mapIt.key, mapIt.value));
            } // else ignore
            mapIt.Next();
            if (isSameKey) {
                dbIt.Next();
            }
        } else { // use db
            orders.push_back(make_pair(dbIt.key, dbIt.value));
            dbIt.Next();
        }
    }

    return true;
}

void CDEXSysOrdersGetter::ToJson(Object &obj) {
    DEX_DB::BlockOrdersToJson(orders, obj);
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
    detail.public_mode = PublicMode::PUBLIC;
    detail.taker_fee_ratio = 40000;
    detail.maker_fee_ratio = 40000;
    detail.activated = true;

    return true;
}

bool CDexDBCache::GetDexOperator(const DexID &id, DexOperatorDetail& detail) {
    decltype(operator_detail_cache)::KeyType idKey(id);
    bool result =  operator_detail_cache.GetData(idKey, detail);
    if(result)
        return result;
    if(id == MAIN_DEX_ID )
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
        if(sysRegId == regid) {
            idOut = MAIN_DEX_ID;
            bool result = GetDexOperator(idOut ,detail);
            if(result && detail.owner_regid == regid)
                return result;
        }
    }
    return false;
}

bool CDexDBCache::HaveDexOperator(const DexID &id) {
    if(id == MAIN_DEX_ID )
        return true;
    decltype(operator_detail_cache)::KeyType idKey(id);
    return operator_detail_cache.HasData(idKey);
}

bool CDexDBCache::HasDexOperatorByOwner(const CRegID &regid) {
     bool dbHave = operator_owner_map_cache.HasData(regid);

     if(!dbHave){
         CRegID sysRegId = SysCfg().GetDex0OwnerRegId();
         if(sysRegId == regid){
             DexOperatorDetail detail;
             bool b = GetDexOperator(MAIN_DEX_ID , detail);
             if(b && detail.owner_regid == regid)
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
