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
Object CDBDexBlockList::ToJson() {
    Object obj;
    for (auto &item : list) {
        Object objItem = item.second.ToJson();
        objItem.insert(objItem.begin(), Pair("txid", std::get<2>(item.first).ToString()));
    }
    return obj;
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
bool CDEXSysOrderListGetter::Execute(uint32_t height) {

    string prefix = dbk::GenDbKey(CDBDexBlockList::PREFIX_TYPE, make_pair(height, (uint8_t)SYSTEM_GEN_ORDER));
    auto pCursor = db_access.NewIterator();
    pCursor->Seek(prefix);
    for (; pCursor->Valid(); pCursor->Next()) {
        const leveldb::Slice &slKey = pCursor->key();
        const leveldb::Slice &slValue = pCursor->value();
        if (!slKey.starts_with(prefix))
            break; // finish
        
        CDBDexBlockList::KeyType curKey;
        if (!ParseDbKey(slKey, CDBDexBlockList::PREFIX_TYPE, curKey)) {
            return ERRORMSG("CDBSysOrderListGetter::Execute Parse db key error! key=%s", HexStr(slKey.ToString()));
        }

        CDEXOrderDetail value;
        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            return ERRORMSG("CDBSysOrderListGetter::Execute unserialize value error! %s", e.what());
        }

        data_list.list.push_back(make_pair(curKey, value));
    }

    return true;
}

Object CDEXSysOrderListGetter::ToJson() {
    return data_list.ToJson();
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
