// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2020 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_CACHE_H
#define PERSIST_DB_CACHE_H

#include "dbconf.h"
#include "dbaccess.h"

#include <map>
#include <memory>

typedef void(UndoDataFunc)(const CDbOpLogs &pDbOpLogs);
typedef std::map<dbk::PrefixType, std::function<UndoDataFunc>> UndoDataFuncMap;

template<typename ValueType>
struct __CacheValue {
    std::shared_ptr<ValueType> value = std::make_shared<ValueType>();
    bool is_modified = false;

    __CacheValue() {}
    __CacheValue(const ValueType &val, bool isModified)
        : value(std::make_shared<ValueType>(val)), is_modified(isModified) {}
    __CacheValue(std::shared_ptr<ValueType> val, bool isModified)
        : value(val), is_modified(isModified) {}

    inline void Set(const __CacheValue &other) {
        ASSERT(other.value);
        Set(*other.value, other.is_modified);
    }

    inline void Set(const ValueType &val, bool isModified) {
        ASSERT(value);
        *value = val;
        is_modified = isModified;
    }

    inline bool IsValueEmpty() const {
        return db_util::IsEmpty(*value);
    }

    inline void SetValueEmpty(bool isModified) {
        db_util::SetEmpty(*value);
        is_modified = isModified;
    }
};

template<int32_t PREFIX_TYPE_VALUE, typename __KeyType, typename __ValueType>
class CCompositeKVCache {
public:
    static const dbk::PrefixType PREFIX_TYPE = (dbk::PrefixType)PREFIX_TYPE_VALUE;
public:
    typedef __KeyType   KeyType;
    typedef __ValueType ValueType;

    using CacheValue = __CacheValue<ValueType>;

    typedef typename std::map<KeyType, CacheValue> Map;
    typedef typename std::map<KeyType, CacheValue>::iterator Iterator;
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
            mapData[otherItem.first].Set(otherItem.second);
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
        ASSERT(!db_util::IsEmpty(key));
        auto it = GetDataIt(key);
        if (!ValueIsEmpty(it)) {
            value = GetValueBy(it);
            return true;
        }
        return false;
    }

    bool GetData(const KeyType &key, const ValueType **value) const {
        ASSERT(value != nullptr && "the value pointer is NULL");
        ASSERT(!db_util::IsEmpty(key));
        auto it = GetDataIt(key);
        if (!ValueIsEmpty(it)) {
            *value = &(GetValueBy(it));
            return true;
        }
        return false;
    }

    bool SetData(const KeyType &key, const ValueType &value) {
        ASSERT(!db_util::IsEmpty(key));

        auto it = GetDataIt(key);
        if (it == mapData.end()) {
            AddOpLog(key, ValueType(), &value);

            AddDataToMap(key, value, true);
        } else {
            auto &valueRef = *it->second.value;
            AddOpLog(key, valueRef, &value);
            UpdateDataSize(valueRef, value);
            it->second.Set(value, true);
        }
        return true;
    }

    bool HasData(const KeyType &key) const {
        ASSERT(!db_util::IsEmpty(key));

        auto it = GetDataIt(key);
        return !ValueIsEmpty(it);
    }

    bool EraseData(const KeyType &key) {
        ASSERT(!db_util::IsEmpty(key));

        Iterator it = GetDataIt(key);
        if (!ValueIsEmpty(it)) {
            auto &valueRef = GetValueBy(it);
            DecDataSize(valueRef);
            AddOpLog(key, valueRef, nullptr);
            it->second.SetValueEmpty(true);
            IncDataSize(valueRef);
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
                if (item.second.is_modified) {
                    // TODO: move value to base for performance
                    pBase->SetDataToCache(item.first, *item.second.value);
                }
            }
        } else if (pDbAccess != nullptr) {
            assert(pBase == nullptr);
            CLevelDBBatch batch; // TODO: use only one batch for a block
            for (auto item : mapData) {
                if (item.second.is_modified) {
                    string key = dbk::GenDbKey(PREFIX_TYPE, item.first);
                    if (item.second.IsValueEmpty()) {
                        batch.Erase(key);
                    } else {
                        batch.Write(key, *item.second.value);
                    }
                }
            }
            pDbAccess->WriteBatch(batch);
        }

        Clear(); // TODO: use lru cache
    }

    void UndoData(const CDbOpLog &dbOpLog) {
        KeyType key;
        ValueType value;
        dbOpLog.Get(key, value);
        SetDataToCache(key, value);
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

    Map& GetMapData() { return mapData; };
private:
    Iterator GetDataIt(const KeyType &key) const {
        Iterator it = mapData.find(key);
        if (it != mapData.end()) {
            return it;
        } else if (pBase != nullptr) {
            // find key-value at base cache
            auto baseIt = pBase->GetDataIt(key);
            if (baseIt != pBase->mapData.end()) {
                // add the found key-value to current mapData, igore the is_modified of Base
                return AddDataToMap(key, GetValueBy(baseIt), false);
            }
        } else if (pDbAccess != NULL) {
            // TODO: need to save the empty value to mapData for search performance?
            auto pDbValue = std::make_shared<ValueType>();
            CacheValue cacheValue;
            if (!pDbAccess->GetData(PREFIX_TYPE, key, *cacheValue.value)) {
                cacheValue.SetValueEmpty(false);
            }
            return AddDataToMap(key, cacheValue);
        }

        return mapData.end();
    }

    ValueType &GetValueBy(Iterator it) const {
        return *it->second.value;
    }

    inline bool ValueIsEmpty(Iterator it) const {
        return it == mapData.end() || it->second.IsValueEmpty();
    }

    // set data to cache only
    void SetDataToCache(const KeyType &key, const ValueType &value) {
        auto it = mapData.find(key);
        if (it != mapData.end()) {
            UpdateDataSize(GetValueBy(it), value);
            it->second.Set(value, true);
        } else {
            AddDataToMap(key, value, true);
        }
    }

    inline Iterator AddDataToMap(const KeyType &key, const ValueType &value, bool isModified) const {
        CacheValue cacheValue(value, isModified);
        return AddDataToMap(key, cacheValue);
    }

    inline Iterator AddDataToMap(const KeyType &key, CacheValue &cacheValue) const {

        ASSERT(!mapData.count(key));
        auto newRet = mapData.emplace(key, cacheValue);
        if (!newRet.second)
            throw runtime_error(strprintf("%s :  %s, alloc new cache item failed", __FUNCTION__, __LINE__));
        auto it = newRet.first;
        IncDataSize(key, GetValueBy(it));
        return it;
    }


    inline void IncDataSize(const KeyType &key, const ValueType &valueIn) const {
        if (is_calc_size) {
            size += CalcDataSize(key);
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
    mutable Map mapData;
    CDBOpLogMap *pDbOpLogMap = nullptr;
    bool is_calc_size = false;
    mutable uint32_t size = 0;
};


template<int32_t PREFIX_TYPE_VALUE, typename __ValueType>
class CSimpleKVCache {
public:
    typedef __ValueType ValueType;
    using CacheValue = __CacheValue<ValueType>;
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
        if (other.cache_value == nullptr) {
            cache_value = nullptr;
        } else {
            if (cache_value == nullptr)
                cache_value = make_shared<CacheValue>();
            cache_value->Set(*other.cache_value);
        }
        pDbOpLogMap = other.pDbOpLogMap;
        return *this;
    }

    void SetBase(CSimpleKVCache *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(!cache_value && "Must SetBase before have any data");
        pBase = pBaseIn;
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        pDbOpLogMap = pDbOpLogMapIn;
    }

    uint32_t GetCacheSize() const {
        if (!cache_value) {
            return 0;
        }

        return ::GetSerializeSize(*cache_value->value, SER_DISK, CLIENT_VERSION);
    }

    bool GetData(ValueType &value) const {
        FetchData();
        if (!IsDataEmpty(cache_value)) {
            value = *cache_value->value;
            return true;
        }
        return false;
    }

    bool GetData(const ValueType **value) const {
        ASSERT(value != nullptr && "the value pointer is NULL");
        FetchData();
        if (!IsDataEmpty(cache_value)) {
            *value = cache_value->value.get();
            return true;
        }
        return false;
    }

    bool SetData(const ValueType &value) {
        FetchData();
        if (!cache_value) {
            cache_value = std::make_shared<CacheValue>();
        }
        AddOpLog(*cache_value->value, &value);
        cache_value->Set(value, true);
        return true;
    }

    bool HasData() const {
        FetchData();
        return !IsDataEmpty(cache_value);
    }

    bool EraseData() {
        FetchData();
        if (!IsDataEmpty(cache_value)) {
            AddOpLog(*cache_value->value, nullptr);
            cache_value->SetValueEmpty(true);
        }
        return true;
    }

    void Clear() {
        cache_value = nullptr;
    }

    void Flush() {
        ASSERT(pBase != nullptr || pDbAccess != nullptr);
        if (cache_value && cache_value->is_modified) {
            if (pBase != nullptr) {
                ASSERT(pDbAccess == nullptr);
                pBase->cache_value = cache_value; // move the data pointer to base cache
            } else if (pDbAccess != nullptr) {
                ASSERT(pBase == nullptr);
                pDbAccess->WriteBatch(PREFIX_TYPE, *cache_value->value);
            }
            cache_value = nullptr;
        }
    }

    void UndoData(const CDbOpLog &dbOpLog) {
        if (!cache_value) {
            cache_value = std::make_shared<CacheValue>();
        }
        dbOpLog.Get(*cache_value->value);
        cache_value->is_modified = true;
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

    std::shared_ptr<ValueType> GetDataPtr() {
        FetchData();
        return cache_value ? cache_value->value : nullptr;
    }
private:
    // fetch data from BaseCache or DB
    void FetchData() const {

        if (!cache_value) {
            if (pBase != nullptr){
                pBase->FetchData();
                if (pBase->cache_value) {
                    auto &value = *pBase->cache_value->value;
                    cache_value = std::make_shared<CacheValue>(value, false);
                }
            } else if (pDbAccess != NULL) {
                auto ptrDbData = std::make_shared<ValueType>();
                if (!pDbAccess->GetData(PREFIX_TYPE, *ptrDbData)) {
                    ptrDbData = nullptr;
                }
                cache_value = std::make_shared<CacheValue>(ptrDbData, false);
            }
        }
    }

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

    inline bool IsDataEmpty(const std::shared_ptr<CacheValue> &ptr) const {
        return ptr == nullptr || ptr->IsValueEmpty();
    }
private:
    mutable CSimpleKVCache              *pBase;
    CDBAccess                           *pDbAccess;
    mutable std::shared_ptr<CacheValue> cache_value     = nullptr;
    CDBOpLogMap                         *pDbOpLogMap    = nullptr;
};

#endif  // PERSIST_DB_CACHE_H
