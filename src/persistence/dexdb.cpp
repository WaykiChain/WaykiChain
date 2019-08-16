// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// class CDBDexBlockList 
void CDBDexBlockList::ToJson(Object &obj) {
    Array array;
    for (auto &item : list) {
        Object objItem = item.second.ToJson();
        objItem.insert(objItem.begin(), Pair("txid", std::get<2>(item.first).ToString()));
        array.push_back(objItem);
    }
    obj.push_back(Pair("orders", array));
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXOrderListGetter 
bool CDEXOrderListGetter::Execute(uint32_t fromHeight, uint32_t toHeight, const string &lastPosInfo, uint32_t maxCount) {
    has_more = false;
    uint32_t count = 0;
    string prestartKey;
    if (lastPosInfo.empty()) {
        prestartKey = dbk::GenDbKey(CDBDexBlockList::PREFIX_TYPE, fromHeight);
    } else {
        prestartKey = lastPosInfo;
    }
    const string prefix = dbk::GetKeyPrefix(CDBDexBlockList::PREFIX_TYPE);
    auto pCursor = db_access.NewIterator();
    pCursor->Seek(prestartKey);
    for (; pCursor->Valid(); pCursor->Next()) {
        const leveldb::Slice &slKey = pCursor->key();
        const leveldb::Slice &slValue = pCursor->value();
        if (!slKey.starts_with(prefix))
            break; // finish
        
        if (!lastPosInfo.empty()) {
            if (slKey == lastPosInfo) {
                continue; // ignore last pos
            }
        }
        CDBDexBlockList::KeyType curKey;
        if (!ParseDbKey(slKey, CDBDexBlockList::PREFIX_TYPE, curKey)) {
            return ERRORMSG("CDBOrderListGetter::Execute Parse db key error! key=%s", HexStr(slKey.ToString()));
        }
        assert(std::get<0>(curKey) < fromHeight);
        if(std::get<0>(curKey) > toHeight) {
            break;// finish
        }

        CDEXOrderDetail value;
        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            return ERRORMSG("CDBOrderListGetter::Execute unserialize value error! %s", e.what());
        }

        data_list.list.push_back(make_pair(curKey, value));
        if (maxCount > 0 && count >= maxCount) {
            // finish, but has more
            has_more = true;
            break;
        }
        count ++;
    }

    if (has_more) {
        pCursor->Next();
        if (!pCursor->Valid() || !pCursor->key().starts_with(prefix)) has_more = false;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// class CDEXSysOrderListGetter

class CDBDexOrderIt {
public:    
    CDBDexBlockList::KeyType key;
    CDEXOrderDetail value;
private:
    shared_ptr<leveldb::Iterator> p_db_it;
    uint32_t height;
    bool is_valid;
    string prefix;    
public:
    CDBDexOrderIt(CDBAccess &dbAccess, uint32_t heightIn)
        : key(), value(), height(heightIn), is_valid(false) {

        p_db_it = dbAccess.NewIterator();
        prefix = dbk::GenDbKey(CDBDexBlockList::PREFIX_TYPE, make_pair(height, (uint8_t)SYSTEM_GEN_ORDER));
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
        CDBDexBlockList::KeyType cur_key;
        if (!ParseDbKey(slKey, CDBDexBlockList::PREFIX_TYPE, key)) {
            throw runtime_error(strprintf("CDBDexOrderIt::Parse db key error! key=%s", HexStr(slKey.ToString())));
        }
        assert(std::get<0>(key) == height || std::get<1>(key) == (uint8_t)SYSTEM_GEN_ORDER);

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

class CMapDexOrderIt {
public:
    CDBDexBlockList::KeyType key;
    CDEXOrderDetail value;
private:        
    CDEXSysOrderListGetter::DBBlockOrderCache::Map &data_map;
    CDEXSysOrderListGetter::DBBlockOrderCache::Iterator map_it;
    uint32_t height;
    bool is_valid;
public:
    CMapDexOrderIt(CDEXSysOrderListGetter::DBBlockOrderCache &dbCache, uint32_t heightIn)
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
        if (std::get<0>(key) != height || std::get<1>(key) != (uint8_t)SYSTEM_GEN_ORDER)
            return false;
        value = map_it->second;
        return true;
    }
};

bool CDEXSysOrderListGetter::Execute(uint32_t height) {

    CMapDexOrderIt mapIt(db_cache, height);
    CDBDexOrderIt dbIt(db_access, height);
    mapIt.First();
    dbIt.First();
    //auto mapIt = mapRangePair.first;
    while(mapIt.IsValid() || dbIt.IsValid()) {
        bool useMap = true, isSameKey = false;
        if (mapIt.IsValid() && !dbIt.IsValid()) {
            if (dbIt.key < mapIt.key) {
                useMap = false;
            } else if (dbIt.key > mapIt.key) { // dbIt.key >= mapIt.key
                useMap = true;
            } else {// dbIt.key == mapIt.key
                useMap = true;
                isSameKey = true;
            }
        } else if (mapIt.IsValid()) {
            useMap = true;
        } else { // dbIt.IsValid())
            useMap = false;
        }
            
        if (useMap) {
            if (!mapIt.value.IsEmpty()) {
                data_list.list.push_back(make_pair(mapIt.key, mapIt.value));
            } // else ignore
            mapIt.Next();
            if (isSameKey && !mapIt.IsValid() && dbIt.IsValid()) {
                // if the same key and map has no more valid data, must use db next data
                dbIt.Next();
            }
        } else { // use db
            data_list.list.push_back(make_pair(dbIt.key, dbIt.value));
            dbIt.Next();
            if (isSameKey && !dbIt.IsValid() && mapIt.IsValid()) {
                // if the same key and db has no more valid data, must use map next data
                mapIt.Next();
            }
        }
    }

    return true;
}

void CDEXSysOrderListGetter::ToJson(Object &obj) {
    data_list.ToJson(obj);
}

///////////////////////////////////////////////////////////////////////////////
// class CDexDBCache

bool CDexDBCache::GetActiveOrder(const uint256 &orderId, CDEXOrderDetail &activeOrder) {
    return activeOrderCache.GetData(orderId, activeOrder);
};

bool CDexDBCache::HaveActiveOrder(const uint256 &orderId) {
    return activeOrderCache.HaveData(orderId);
};

bool CDexDBCache::CreateActiveOrder(const uint256 &orderId, const CDEXOrderDetail &activeOrder) {
    assert(!activeOrderCache.HaveData(orderId));
    return activeOrderCache.SetData(orderId, activeOrder)
        && blockOrderCache.SetData(CDBDexBlockList::MakeKey(orderId, activeOrder), activeOrder);
}

bool CDexDBCache::UpdateActiveOrder(const uint256 &orderId, const CDEXOrderDetail &activeOrder) {
    return activeOrderCache.SetData(orderId, activeOrder)
        && blockOrderCache.SetData(CDBDexBlockList::MakeKey(orderId, activeOrder), activeOrder);
};

bool CDexDBCache::EraseActiveOrder(const uint256 &orderId, const CDEXOrderDetail &activeOrder) {
    return activeOrderCache.EraseData(orderId)
        && blockOrderCache.EraseData(CDBDexBlockList::MakeKey(orderId, activeOrder));
};
