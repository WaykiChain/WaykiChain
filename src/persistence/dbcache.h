// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2020 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_CACHE_H
#define PERSIST_DB_CACHE_H

#include "commons/uint256.h"
#include "dbconf.h"
#include "dbaccess.h"

#include <map>
#include <memory>

typedef void(UndoDataFunc)(const CDbOpLogs &pDbOpLogs);
typedef std::map<dbk::PrefixType, std::function<UndoDataFunc>> UndoDataFuncMap;

template<int32_t PREFIX_TYPE_VALUE, typename __KeyType, typename __ValueType>
class CCompositeKVCache {
public:
    static const dbk::PrefixType PREFIX_TYPE = (dbk::PrefixType)PREFIX_TYPE_VALUE;
public:
    typedef __KeyType   KeyType;
    typedef __ValueType ValueType;
    typedef std::shared_ptr<__ValueType> ValueSPtr;
    typedef typename std::map<KeyType, ValueSPtr> Map;
    typedef typename std::map<KeyType, ValueSPtr>::iterator Iterator;

public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CCompositeKVCache(): pBase(nullptr), pDbAccess(nullptr) {};

    CCompositeKVCache(CCompositeKVCache *pBaseIn): pBase(pBaseIn),
        pDbAccess(nullptr) {
        assert(pBaseIn != nullptr);
    };

    CCompositeKVCache(CDBAccess *pDbAccessIn): pBase(nullptr),
        pDbAccess(pDbAccessIn), is_calc_size(true) {
        assert(pDbAccessIn != nullptr);
        assert(pDbAccess->GetDbNameType() == GetDbNameEnumByPrefix(PREFIX_TYPE));
    };

    CCompositeKVCache(const CCompositeKVCache &other) {
        operator=(other);
    }

    CCompositeKVCache& operator=(const CCompositeKVCache& other) {
        pBase = other.pBase;
        pDbAccess = other.pDbAccess;
        // deep copy for map
        mapData.clear();
        for (auto otherItem : other.mapData) {
            mapData[otherItem.first] = make_shared<ValueType>(*otherItem.second);
        }
        pDbOpLogMap = other.pDbOpLogMap;
        is_calc_size = other.is_calc_size;
        size = other.size;

        return *this;
    }

    void SetBase(CCompositeKVCache *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(mapData.empty());
        pBase = pBaseIn;
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        pDbOpLogMap = pDbOpLogMapIn;
    }

    bool IsCalcSize() const { return is_calc_size; }

    uint32_t GetCacheSize() const {
        return size;
    }

    bool GetData(const KeyType &key, ValueType &value) const {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        if (it != mapData.end() && !db_util::IsEmpty(*it->second)) {
            value = *it->second;
            return true;
        }
        return false;
    }

    bool GetData(const KeyType &key, const ValueType **value) const {
        assert(value != nullptr && "the value pointer is NULL");
        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        if (it != mapData.end() && !db_util::IsEmpty(*it->second)) {
            *value = it->second.get();
            return true;
        }
        return false;
    }

    bool SetData(const KeyType &key, const ValueType &value) {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        if (it == mapData.end()) {
            auto pEmptyValue = db_util::MakeEmptyValue<ValueType>();
            AddOpLog(key, *pEmptyValue, &value);
            AddDataToMap(key, value);
        } else {
            AddOpLog(key, *it->second, &value);
            UpdateDataSize(*it->second, value);
            *it->second = value;
        }
        return true;
    }

    bool HasData(const KeyType &key) const {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        return it != mapData.end() && !db_util::IsEmpty(*it->second);
    }

    bool EraseData(const KeyType &key) {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        Iterator it = GetDataIt(key);
        if (it != mapData.end() && !db_util::IsEmpty(*it->second)) {
            DecDataSize(*it->second);
            AddOpLog(key, *it->second, nullptr);
            db_util::SetEmpty(*it->second);
            IncDataSize(*it->second);
        }
        return true;
    }

    void Clear() {
        mapData.clear();
        size = 0;
    }

    void Flush() {
        assert(pBase != nullptr || pDbAccess != nullptr);
        if (pBase != nullptr) {
            assert(pDbAccess == nullptr);
            for (auto item : mapData) {
                pBase->SetDataToSelf(item.first, *item.second);
            }
        } else if (pDbAccess != nullptr) {
            assert(pBase == nullptr);
            CLevelDBBatch batch;
            for (auto item : mapData) {
                string key = dbk::GenDbKey(PREFIX_TYPE, item.first);
                if (db_util::IsEmpty(*item.second)) {
                    batch.Erase(key);
                } else {
                    batch.Write(key, *item.second);
                }
            }
            pDbAccess->WriteBatch(batch);
        }

        Clear();
    }

    void UndoData(const CDbOpLog &dbOpLog) {
        KeyType key;
        ValueType value;
        dbOpLog.Get(key, value);
        SetDataToSelf(key, value);
    }

    void UndoDataList(const CDbOpLogs &dbOpLogs) {
        for (auto it = dbOpLogs.rbegin(); it != dbOpLogs.rend(); it++) {
            UndoData(*it);
        }
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        undoDataFuncMap[GetPrefixType()] = std::bind(&CCompositeKVCache::UndoDataList, this, std::placeholders::_1);
    }

    dbk::PrefixType GetPrefixType() const { return PREFIX_TYPE; }

    CDBAccess* GetDbAccessPtr() {
        CDBAccess* pRet = pDbAccess;
        if (pRet == nullptr && pBase != nullptr) {
            pRet = pBase->GetDbAccessPtr();
        }
        return pRet;
    }

    CCompositeKVCache<PREFIX_TYPE, KeyType, ValueType>* GetBasePtr() { return pBase; }

    map<KeyType, ValueSPtr>& GetMapData() { return mapData; };
private:
    Iterator GetDataIt(const KeyType &key) const {
        Iterator it = mapData.find(key);
        if (it != mapData.end()) {
            return it;
        } else if (pBase != nullptr) {
            // find key-value at base cache
            auto baseIt = pBase->GetDataIt(key);
            if (baseIt != pBase->mapData.end()) {
                // the found key-value add to current mapData
                return AddDataToMap(key, *baseIt->second);
            }
        } else if (pDbAccess != NULL) {
            // TODO: need to save the empty value to mapData for search performance?
            auto pDbValue = db_util::MakeEmptyValue<ValueType>();
            if (pDbAccess->GetData(PREFIX_TYPE, key, *pDbValue)) {
                return AddDataToMap(key, pDbValue);
            }
        }

        return mapData.end();
    }

    // set data to self only
    void SetDataToSelf(const KeyType &key, const ValueType &value) {
        auto it = mapData.find(key);
        if (it != mapData.end()) {
            UpdateDataSize(*it->second, value);
            *it->second = value;
        } else {
            AddDataToMap(key, value);
        }
    }

    inline Iterator AddDataToMap(const KeyType &keyIn, const ValueType &valueIn) const {
        auto spNewValue = make_shared<ValueType>(valueIn);
        return AddDataToMap(keyIn, spNewValue);
    }

    inline Iterator AddDataToMap(const KeyType &keyIn, ValueSPtr &spNewValue) const {
        auto newRet = mapData.emplace(keyIn, spNewValue);
        if (!newRet.second)
            throw runtime_error(strprintf("%s :  %s, alloc new cache item failed", __FUNCTION__, __LINE__));
        IncDataSize(keyIn, *spNewValue);
        return newRet.first;
    }

    inline void IncDataSize(const KeyType &keyIn, const ValueType &valueIn) const {
        if (is_calc_size) {
            size += CalcDataSize(keyIn);
            size += CalcDataSize(valueIn);
        }
    }

    inline void IncDataSize(const ValueType &valueIn) const {
        if (is_calc_size)
            size += CalcDataSize(valueIn);
    }

    inline void DecDataSize(const ValueType &valueIn) const {
        if (is_calc_size) {
            uint32_t sz = CalcDataSize(valueIn);
            size = size > sz ? size - sz : 0;
        }
    }

    inline void UpdateDataSize(const ValueType &oldValue, const ValueType &newVvalue) const {
        if (is_calc_size) {
            size += CalcDataSize(newVvalue);
            uint32_t oldSz = CalcDataSize(oldValue);
            size = size > oldSz ? size - oldSz : 0;
        }
    }

    template <typename Data>
    inline uint32_t CalcDataSize(const Data &d) const {
        return ::GetSerializeSize(d, SER_DISK, CLIENT_VERSION);
    }

    inline void AddOpLog(const KeyType &key, const ValueType& oldValue, const ValueType *pNewValue) {
        if (pDbOpLogMap != nullptr) {
            CDbOpLog dbOpLog;
            #ifdef DB_OP_LOG_NEW_VALUE
                if (pNewValue != nullptr)
                    dbOpLog.Set(key, make_pair(oldValue, *pNewValue));
                else
                    dbOpLog.Set(key, make_pair(oldValue, ValueType()));
            #else
                dbOpLog.Set(key, oldValue);
            #endif
            pDbOpLogMap->AddOpLog(PREFIX_TYPE, dbOpLog);
        }

    }
private:
    mutable CCompositeKVCache<PREFIX_TYPE, KeyType, ValueType> *pBase = nullptr;
    CDBAccess *pDbAccess = nullptr;
    mutable map<KeyType, ValueSPtr> mapData;
    CDBOpLogMap *pDbOpLogMap = nullptr;
    bool is_calc_size = false;
    mutable uint32_t size = 0;
};


template<int32_t PREFIX_TYPE_VALUE, typename __ValueType>
class CSimpleKVCache {
public:
    typedef __ValueType ValueType;
    static const dbk::PrefixType PREFIX_TYPE = (dbk::PrefixType)PREFIX_TYPE_VALUE;
public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CSimpleKVCache(): pBase(nullptr), pDbAccess(nullptr) {};

    CSimpleKVCache(CSimpleKVCache *pBaseIn): pBase(pBaseIn),
        pDbAccess(nullptr) {
        assert(pBaseIn != nullptr);
    }

    CSimpleKVCache(CDBAccess *pDbAccessIn): pBase(nullptr),
        pDbAccess(pDbAccessIn) {
        assert(pDbAccessIn != nullptr);
    }

    CSimpleKVCache(const CSimpleKVCache &other) {
        operator=(other);
    }

    CSimpleKVCache& operator=(const CSimpleKVCache& other) {
        pBase = other.pBase;
        pDbAccess = other.pDbAccess;
        // deep copy for shared_ptr
        if (other.ptrData == nullptr) {
            ptrData = nullptr;
        } else {
            ptrData = make_shared<ValueType>(*other.ptrData);
        }
        pDbOpLogMap = other.pDbOpLogMap;
        return *this;
    }

    void SetBase(CSimpleKVCache *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(!ptrData && "Must SetBase before have any data");
        pBase = pBaseIn;
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        pDbOpLogMap = pDbOpLogMapIn;
    }

    uint32_t GetCacheSize() const {
        if (!ptrData) {
            return 0;
        }

        return ::GetSerializeSize(*ptrData, SER_DISK, CLIENT_VERSION);
    }

    bool GetData(ValueType &value) const {
        auto ptr = GetDataPtr();
        if (ptr && !db_util::IsEmpty(*ptr)) {
            value = *ptr;
            return true;
        }
        return false;
    }

    bool GetData(const ValueType **value) const {
        assert(value != nullptr && "the value pointer is NULL");
        auto ptr = GetDataPtr();
        if (ptr && !db_util::IsEmpty(*ptr)) {
            *value = ptr.get();
            return true;
        }
        return false;
    }

    bool SetData(const ValueType &value) {
        if (!ptrData) {
            ptrData = db_util::MakeEmptyValue<ValueType>();
        }
        AddOpLog(*ptrData, &value);
        *ptrData = value;
        return true;
    }

    bool HasData() const {
        auto ptr = GetDataPtr();
        return ptr && !db_util::IsEmpty(*ptr);
    }

    bool EraseData() {
        auto ptr = GetDataPtr();
        if (ptr && !db_util::IsEmpty(*ptr)) {
            AddOpLog(*ptr, nullptr);
            db_util::SetEmpty(*ptr);
        }
        return true;
    }

    void Clear() {
        ptrData = nullptr;
    }

    void Flush() {
        assert(pBase != nullptr || pDbAccess != nullptr);
        if (ptrData) {
            if (pBase != nullptr) {
                assert(pDbAccess == nullptr);
                pBase->ptrData = ptrData;
            } else if (pDbAccess != nullptr) {
                assert(pBase == nullptr);
                pDbAccess->WriteBatch(PREFIX_TYPE, *ptrData);
            }
            ptrData = nullptr;
        }
    }

    void UndoData(const CDbOpLog &dbOpLog) {
        if (!ptrData) {
            ptrData = db_util::MakeEmptyValue<ValueType>();
        }
        dbOpLog.Get(*ptrData);
    }

    void UndoDataList(const CDbOpLogs &dbOpLogs) {
        for (auto it = dbOpLogs.rbegin(); it != dbOpLogs.rend(); it++) {
            UndoData(*it);
        }
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        undoDataFuncMap[GetPrefixType()] = std::bind(&CSimpleKVCache::UndoDataList, this, std::placeholders::_1);
    }

    dbk::PrefixType GetPrefixType() const { return PREFIX_TYPE; }

    std::shared_ptr<ValueType> GetDataPtr() const {

        if (ptrData) {
            return ptrData;
        } else if (pBase != nullptr){
            auto ptr = pBase->GetDataPtr();
            if (ptr) {
                ptrData = std::make_shared<ValueType>(*ptr);
                return ptrData;
            }
        } else if (pDbAccess != NULL) {
            auto ptrDbData = db_util::MakeEmptyValue<ValueType>();

            if (pDbAccess->GetData(PREFIX_TYPE, *ptrDbData)) {
                assert(!db_util::IsEmpty(*ptrDbData));
                ptrData = ptrDbData;
                return ptrData;
            }
        }
        return nullptr;
    }

private:
    inline void AddOpLog(const ValueType &oldValue, const ValueType *pNewValue) {
        if (pDbOpLogMap != nullptr) {
            CDbOpLog dbOpLog;
            #ifdef DB_OP_LOG_NEW_VALUE
                if (pNewValue != nullptr)
                    dbOpLog.Set(make_pair(oldValue, *pNewValue));
                else
                    dbOpLog.Set(make_pair(oldValue, ValueType()));
            #else
                dbOpLog.Set(oldValue);
            #endif
            pDbOpLogMap->AddOpLog(PREFIX_TYPE, dbOpLog);
        }

    }
private:
    mutable CSimpleKVCache<PREFIX_TYPE, ValueType> *pBase;
    CDBAccess *pDbAccess;
    mutable std::shared_ptr<ValueType> ptrData = nullptr;
    CDBOpLogMap *pDbOpLogMap                   = nullptr;
};

#endif  // PERSIST_DB_CACHE_H
