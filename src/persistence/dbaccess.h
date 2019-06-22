// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ACCESS_H
#define PERSIST_DB_ACCESS_H

#include "dbconf.h"
#include "leveldbwrapper.h"

#include <tuple>

/**
 * Empty functions
 */
namespace db_util {
    // uint8_t
    inline bool IsEmpty(const uint8_t val) { return val != 0; }
    inline void SetEmpty(uint8_t &val) { val = 0; }

    // uint16_t
    inline bool IsEmpty(const uint16_t val) { return val != 0; }
    inline void SetEmpty(uint16_t &val) { val = 0; }

    // uint32_t
    inline bool IsEmpty(const uint32_t val) { return val != 0; }
    inline void SetEmpty(uint32_t &val) { val = 0; }

    // uint64_t
    inline bool IsEmpty(const uint64_t val) { return val != 0; }
    inline void SetEmpty(uint64_t &val) { val = 0; }

    // string
    template<typename C> bool IsEmpty(const basic_string<C> &val);
    template<typename C> void SetEmpty(basic_string<C> &val);

    // vector
    template<typename T, typename A> bool IsEmpty(const vector<T, A>& val);
    template<typename T, typename A> void SetEmpty(vector<T, A>& val);

    // set
    template<typename K, typename Pred, typename A> bool IsEmpty(const set<K, Pred, A>& val);
    template<typename K, typename Pred, typename A> void SetEmpty(set<K, Pred, A>& val);

    // 2 pair
    template<typename K, typename T> bool IsEmpty(const std::pair<K, T>& val);
    template<typename K, typename T> void SetEmpty(std::pair<K, T>& val);

    // 3 tuple
    template<typename T0, typename T1, typename T2> bool IsEmpty(const std::tuple<T0, T1, T2>& val);
    template<typename T0, typename T1, typename T2> void SetEmpty(std::tuple<T0, T1, T2>& val);

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T> bool IsEmpty(const T& val);
    template<typename T> void SetEmpty(const T& val);

    // string
    template<typename C>
    bool IsEmpty(const basic_string<C> &val) {
        return val.empty();
    }

    template<typename C>
    void SetEmpty(basic_string<C> &val) {
        val.clear();
    }

    // vector
    template<typename T, typename A>
    bool IsEmpty(const vector<T, A>& val) {
        return val.empty();
    }
    template<typename T, typename A>
    void SetEmpty(vector<T, A>& val) {
        val.clear();
    }

    // set
    template<typename K, typename Pred, typename A> bool IsEmpty(const set<K, Pred, A>& val) {
        return val.empty();
    }
    template<typename K, typename Pred, typename A> void SetEmpty(set<K, Pred, A>& val) {
        val.clear();
    }

    // 2 pair
    template<typename K, typename T>
    bool IsEmpty(const std::pair<K, T>& val) {
        return IsEmpty(val.first) && IsEmpty(val.second);
    }
    template<typename K, typename T>
    void SetEmpty(std::pair<K, T>& val) {
        SetEmpty(val.first);
        SetEmpty(val.second);
    }

    // 3 tuple
    template<typename T0, typename T1, typename T2>
    bool IsEmpty(const std::tuple<T0, T1, T2>& val) {
        return IsEmpty(std::get<0>(val)) &&
               IsEmpty(std::get<1>(val)) &&
               IsEmpty(std::get<2>(val));
    }
    template<typename T0, typename T1, typename T2>
    void SetEmpty(std::tuple<T0, T1, T2>& val) {
        SetEmpty(std::get<0>(val));
        SetEmpty(std::get<1>(val));
        SetEmpty(std::get<2>(val));
    }

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T>
    bool IsEmpty(const T& val) {
        return val.IsEmpty();
    }

    template<typename T>
    void SetEmpty(T& val) {
        val.SetEmpty();
    }
};

class CDBCountValue {
public:
    int64_t value;
public:
    CDBCountValue(): value(0) {}

    bool IsEmpty() const {
        return value == 0;
    }

    void SetEmpty() {
        value = 0;
    }

    inline bool IsValid() { return value >= 0; };

    IMPLEMENT_SERIALIZE( READWRITE(value); )
};

class CDBAccess {
public:
    CDBAccess(DBNameType dbNameTypeIn, size_t nCacheSize, bool fMemory, bool fWipe) :
              dbNameType(dbNameTypeIn),
              db( GetDataDir() / "blocks" / ::GetDbName(dbNameTypeIn), nCacheSize, fMemory, fWipe ) {}

    template<typename KeyType, typename ValueType>
    bool GetData(const dbk::PrefixType prefixType, const KeyType &key, ValueType &value) const {
        string keyStr = dbk::GenDbKey(prefixType, key);
        return db.Read(keyStr, value);
    }

    template<typename ValueType>
    bool GetData(const dbk::PrefixType prefixType, ValueType &value) const {
        const string prefix = dbk::GetKeyPrefix(prefixType);
        return db.Read(prefix, value);
    }

    template <typename KeyType>
    bool GetTopNElements(const uint32_t maxNum, const dbk::PrefixType prefixType, set<KeyType> &expiredKeys,
                         set<KeyType> &keys) {
        KeyType key;
        uint32_t count             = 0;
        leveldb::Iterator *pCursor = db.NewIterator();
        leveldb::Slice slKey       = pCursor->key();
        pCursor->Seek(dbk::GetKeyPrefix(prefixType));

        for (; (count < maxNum) && pCursor->Valid(); pCursor->Next()) {
            if (!dbk::ParseDbKey(slKey, prefixType, key)) {
                break;
            }

            if (expiredKeys.count(key)) {
                continue;
            } else {
                // Got an valid element.
                auto ret = keys.emplace(key);
                assert(ret.second);  // TODO: throw error

                ++count;
            }
        }

        return true;
    }

    template <typename KeyType, typename ValueType>
    bool GetAllElements(const dbk::PrefixType prefixType, map<KeyType, ValueType> &elements) {
        KeyType key;
        ValueType value;
        leveldb::Iterator *pCursor = db.NewIterator();
        leveldb::Slice slKey       = pCursor->key();
        pCursor->Seek(dbk::GetKeyPrefix(prefixType));

        for (; pCursor->Valid(); pCursor->Next()) {
            if (!dbk::ParseDbKey(slKey, prefixType, key)) {
                break;
            }

            // Got an valid element.
            leveldb::Slice slValue = pCursor->value();
            CDataStream ds(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ds >> value;
            auto ret = elements.emplace(key, value);
            assert(ret.second);  // TODO: throw error
        }

        return true;
    }

    template <typename KeyType, typename ValueType>
    bool GetAllElements(const dbk::PrefixType prefixType, set<KeyType> &expiredKeys,
                        map<KeyType, ValueType> &elements) {
        KeyType key;
        ValueType value;
        leveldb::Iterator *pCursor = db.NewIterator();
        leveldb::Slice slKey       = pCursor->key();
        pCursor->Seek(dbk::GetKeyPrefix(prefixType));

        for (; pCursor->Valid(); pCursor->Next()) {
            if (!dbk::ParseDbKey(slKey, prefixType, key)) {
                break;
            }

            if (expiredKeys.count(key)) {
                continue;
            } else {
                // Got an valid element.
                leveldb::Slice slValue = pCursor->value();
                CDataStream ds(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                ds >> value;
                auto ret = elements.emplace(key, value);
                assert(ret.second);  // TODO: throw error
            }
        }

        return true;
    }

    template<typename KeyType, typename ValueType>
    bool HaveData(const dbk::PrefixType prefixType, const KeyType &key) const {
        string keyStr = dbk::GenDbKey(prefixType, key);
        return db.Exists(keyStr);
    }

    template<typename KeyType, typename ValueType>
    void BatchWrite(const dbk::PrefixType prefixType, const map<KeyType, ValueType> &mapData) {
        CLevelDBBatch batch;
        for (auto item : mapData) {
            string key = dbk::GenDbKey(prefixType, item.first);
            if (db_util::IsEmpty(item.second)) {
                batch.Erase(key);
            } else {
                batch.Write(key, item.second);
            }
        }
        db.WriteBatch(batch, true);
    }

    template<typename ValueType>
    void BatchWrite(const dbk::PrefixType prefixType, ValueType &value) {
        CLevelDBBatch batch;
        const string prefix = dbk::GetKeyPrefix(prefixType);

        if (db_util::IsEmpty(value)) {
            batch.Erase(prefix);
        } else {
            batch.Write(prefix, value);
        }
        db.WriteBatch(batch, true);
    }

    DBNameType GetDbNameType() const { return dbNameType; }
private:
    DBNameType dbNameType;
    mutable CLevelDBWrapper db; // // TODO: remove the mutable declare
};

template<int PREFIX_TYPE_VALUE, typename KeyType, typename ValueType>
class CDBMultiValueCache {
public:
    static const dbk::PrefixType PREFIX_TYPE = (dbk::PrefixType)PREFIX_TYPE_VALUE;
public:
    typedef typename map<KeyType, ValueType>::iterator Iterator;

public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CDBMultiValueCache(): pBase(nullptr), pDbAccess(nullptr) {};

    CDBMultiValueCache(CDBMultiValueCache<PREFIX_TYPE, KeyType, ValueType> *pBaseIn): pBase(pBaseIn),
        pDbAccess(nullptr) {
        assert(pBaseIn != nullptr);
    };

    CDBMultiValueCache(CDBAccess *pDbAccessIn): pBase(nullptr),
        pDbAccess(pDbAccessIn) {
        assert(pDbAccessIn != nullptr);
        assert(pDbAccess->GetDbNameType() == GetDbNameEnumByPrefix(PREFIX_TYPE));
    };

    void SetBase(CDBMultiValueCache<PREFIX_TYPE, KeyType, ValueType> *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(mapData.empty());
        pBase = pBaseIn;
    };

    bool GetTopNElements(const uint32_t maxNum, set<KeyType> &keys) {
        // 1. Get all candidate elements.
        set<KeyType> expiredKeys;
        set<KeyType> candidateKeys;
        if (!GetTopNElements(maxNum, expiredKeys, candidateKeys)) {
            // TODO: log
            return false;
        }

        // 2. Get the top N elements.
        uint32_t count = 0;
        auto iter      = candidateKeys.begin();
        for (; (count < maxNum) && iter != candidateKeys.end(); ++iter) {
            keys.insert(*iter);
        }

        return keys.size() == maxNum;
    }

    bool GetAllElements(map<KeyType, ValueType> &elements) {
        set<KeyType> expiredKeys;
        if (!GetAllElements(expiredKeys, elements)) {
            // TODO: log
            return false;
        }

        return true;
    }

    bool GetData(const KeyType &key, ValueType &value) const {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        if (it != mapData.end() && !db_util::IsEmpty(it->second)) {
            value = it->second;
            return true;
        }
        return false;
    }

    bool SetData(const KeyType &key, const ValueType &value) {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        mapData[key] = value;
        return true;
    };

    bool SetData(const KeyType &key, const ValueType &value, CDbOpLog &dbOpLog) {

        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        if (it != mapData.end()) {
            ValueType newEmptyValue; db_util::SetEmpty(newEmptyValue);
            auto newRet = mapData.emplace(std::make_pair(key, newEmptyValue)); // create new empty value
            assert(newRet.second); // TODO: if false then throw error
            it = newRet.first;
        }        
        dbOpLog.Set(key, value);
        it->second = value;
        return true;
    };


    bool HaveData(const KeyType &key) const {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        auto it = GetDataIt(key);
        return it != mapData.end() && !db_util::IsEmpty(it->second);
    }

    bool EraseData(const KeyType &key) {
        if (db_util::IsEmpty(key)) {
            return false;
        }        
        Iterator it = GetDataIt(key);
        if (it != mapData.end() && !db_util::IsEmpty(it->second)) {
            db_util::SetEmpty(it->second);
        }
        return true;
    }

    bool EraseData(const KeyType &key, CDbOpLog &dbOpLog) {
        if (db_util::IsEmpty(key)) {
            return false;
        }        
        Iterator it = GetDataIt(key);
        if (it != mapData.end()) {
            dbOpLog.Set(key, it->second);
        } else {
            ValueType emptyValue; db_util::SetEmpty(emptyValue);
            dbOpLog.Set(key, emptyValue);
        }
        if (it != mapData.end() && !db_util::IsEmpty(it->second)) {
            db_util::SetEmpty(it->second);
        }
        return true;
    }

    void Flush() {
        assert(pBase != nullptr || pDbAccess != nullptr);
        if (pBase != nullptr) {
            assert(pDbAccess == nullptr);
            for (auto it : mapData) {
                pBase->mapData[it.first] = it.second;
            }
        } else if (pDbAccess != nullptr) {
            assert(pBase == nullptr);
            pDbAccess->BatchWrite<KeyType, ValueType>(PREFIX_TYPE, mapData);
        }

        mapData.clear();
    }

    bool UndoData(const CDbOpLog &dbOpLog) {
        assert(dbOpLog.GetPrefixType() == PREFIX_TYPE);
        KeyType key;
        ValueType value;
        dbOpLog.Get(key, value);
        mapData[key] = value;
        return true;
    }

    void ParseUndoData(const CDbOpLog &dbOpLog, KeyType &key, ValueType &value) {
        assert(dbOpLog.GetPrefixType() == PREFIX_TYPE);
        dbOpLog.Get(key, value);
    }

    dbk::PrefixType GetPrefixType() const { return PREFIX_TYPE; }

private:
    Iterator GetDataIt(const KeyType &key) const {
        // key should not be empty
        if (db_util::IsEmpty(key)) {
            return mapData.end();
        }

        Iterator it = mapData.find(key);
        if (it != mapData.end()) {
            return it;
        } else if (pBase != nullptr){
            auto it = pBase->GetDataIt(key);
            if (it != mapData.end()) {
                auto newRet = mapData.emplace(std::make_pair(key, it->second));
                assert(newRet.second); // TODO: throw error
                return newRet.first;
            }
        } else if (pDbAccess != NULL) {
            // TODO: need to save the empty value to mapData for search performance?
            auto ptrValue = std::make_shared<ValueType>();

            if (pDbAccess->GetData(PREFIX_TYPE, key, *ptrValue)) {
                auto newRet = mapData.emplace(std::make_pair(key, *ptrValue));
                assert(newRet.second); // TODO: throw error
                return newRet.first;
            }
        }

        return mapData.end();
    };

    bool GetTopNElements(const uint32_t maxNum, set<KeyType> &expiredKeys, set<KeyType> &keys) {
        if (!mapData.empty()) {
            uint32_t count = 0;
            auto iter      = mapData.begin();

            for (; (count < maxNum) && iter != mapData.end(); ++iter) {
                if (db_util::IsEmpty(iter->second)) {
                    expiredKeys.insert(iter->first);
                } else if (expiredKeys.count(iter->first) || keys.count(iter->first)) {
                    // TODO: log
                    continue;
                } else {
                    // Got a valid element.
                    keys.insert(iter->first);

                    ++count;
                }
            }
        }

        if (pBase != nullptr) {
            return pBase->GetTopNElements(maxNum, expiredKeys, keys);
        } else if (pDbAccess != nullptr) {
            return pDbAccess->GetTopNElements(maxNum, PREFIX_TYPE, expiredKeys, keys);
        }

        return true;
    }

    bool GetAllElements(set<KeyType> &expiredKeys, map<KeyType, ValueType> &elements) {
        if (!mapData.empty()) {
            for (auto iter : mapData) {
                if (db_util::IsEmpty(iter.second)) {
                    expiredKeys.insert(iter.first);
                } else if (expiredKeys.count(iter.first) || elements.count(iter.first)) {
                    // TODO: log
                    continue;
                } else {
                    // Got a valid element.
                    elements.insert(iter);
                }
            }
        }

        if (pBase != nullptr) {
            return pBase->GetAllElements(expiredKeys, elements);
        } else if (pDbAccess != nullptr) {
            return pDbAccess->GetAllElements(PREFIX_TYPE, expiredKeys, elements);
        }

        return true;
    }

private:
    mutable CDBMultiValueCache<PREFIX_TYPE, KeyType, ValueType> *pBase;
    CDBAccess *pDbAccess;
    mutable map<KeyType, ValueType> mapData;
};


template<int PREFIX_TYPE_VALUE, typename ValueType>
class CDBScalarValueCache {
public:
    static const dbk::PrefixType PREFIX_TYPE = (dbk::PrefixType)PREFIX_TYPE_VALUE;
public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CDBScalarValueCache(): pBase(nullptr), pDbAccess(nullptr) {};

    CDBScalarValueCache(CDBScalarValueCache<PREFIX_TYPE, ValueType> *pBaseIn): pBase(pBaseIn),
        pDbAccess(nullptr) {
        assert(pBaseIn != nullptr);
    };

    CDBScalarValueCache(CDBAccess *pDbAccessIn): pBase(nullptr),
        pDbAccess(pDbAccessIn) {
        assert(pDbAccessIn != nullptr);
    };

    void SetBase(CDBScalarValueCache<PREFIX_TYPE, ValueType> *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(!ptrData);
        pBase = pBaseIn;
    };

    bool GetData(ValueType &value) const {
        auto ptr = GetDataPtr();
        if (ptr && !db_util::IsEmpty(*ptr)) {
            value = *ptr;
            return true;
        }
        return false;
    }

    bool SetData(const ValueType &value) {
        if (ptrData) {
            *ptrData = value;
        } else {
            ptrData = std::make_shared<ValueType>(value);
        }
        return true;
    };

    bool SetData(const ValueType &value, CDbOpLog &dbOpLog) {
        if (ptrData) {
            dbOpLog.Set(*ptrData);
            *ptrData = value;
        } else {
            ValueType emptyValue; db_util::SetEmpty(emptyValue);
            dbOpLog.Set(emptyValue);
            ptrData = std::make_shared<ValueType>(value);
        }
        return true;
    };

    bool HaveData() const {
        auto ptr = GetDataPtr();
        return ptr && !db_util::IsEmpty(*ptr);
    }

    bool EraseData() {
        auto ptr = GetDataPtr();
        if (ptr && !db_util::IsEmpty(*ptr)) {
            db_util::SetEmpty(*ptr);
        }
        return true;
    }

    bool EraseData(CDbOpLog &dbOpLog) {
        auto ptr = GetDataPtr();
        if (ptr) {
            dbOpLog.Set(*ptrData);
        } else {
            ValueType emptyValue; db_util::SetEmpty(emptyValue);
            dbOpLog.Set(emptyValue);
        }
        if (ptr && !db_util::IsEmpty(*ptr)) {
            db_util::SetEmpty(*ptr);
        }
        return true;
    }

    void Flush() {
        assert(pBase != nullptr || pDbAccess != nullptr);
        if (pBase != nullptr) {
            assert(pDbAccess == nullptr);
            pBase->ptrData = ptrData;
        } else if (pDbAccess != nullptr) {
            assert(pBase == nullptr);
            pDbAccess->BatchWrite(PREFIX_TYPE, *ptrData);
        }

        ptrData = nullptr;
    }

    void UndoData(const CDbOpLog &dbOpLog) {
        assert(dbOpLog.GetPrefixType() == PREFIX_TYPE);
        if (!ptrData) {
            ptrData = make_shared<ValueType>();
        }
        dbOpLog.Get(*ptrData);
    }

    dbk::PrefixType GetPrefixType() const { return PREFIX_TYPE; }

private:
    std::shared_ptr<ValueType> GetDataPtr() const {

        if (ptrData) {
            return ptrData;
        } else if (pBase != nullptr){
            auto ptr = pBase->GetDataPtr();
            if (ptr) {
                assert(!db_util::IsEmpty(*ptr));
                ptrData = std::make_shared<ValueType>(*ptr);
                return ptrData;
            }
        } else if (pDbAccess != NULL) {
            auto ptrDbData = std::make_shared<ValueType>();

            if (pDbAccess->GetData(PREFIX_TYPE, *ptrDbData)) {
                assert(!db_util::IsEmpty(*ptrDbData));
                ptrData = ptrDbData;
                return ptrData;
            }
        }
        return nullptr;
    };
private:
    mutable CDBScalarValueCache<PREFIX_TYPE, ValueType> *pBase;
    CDBAccess *pDbAccess;
    mutable std::shared_ptr<ValueType> ptrData;
};

#endif//PERSIST_DB_ACCESS_H
