// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ACCESS_H
#define PERSIST_DB_ACCESS_H

#include <tuple>

#include "dbconf.h"
#include "leveldbwrapper.h"

/**
 * Empty functions
 */
namespace db_util {
    inline bool IsEmpty(const int val) { return true;}

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
    CDBAccess(const string &name, size_t nCacheSize, bool fMemory, bool fWipe) :
                db( GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe ) {}

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
    bool GetTopNElements(const int32_t maxNum, const dbk::PrefixType prefixType, set<KeyType> &expiredKeys,
                         set<KeyType> &keys) {
        leveldb::Iterator *pCursor = db.NewIterator();
        leveldb::Slice slKey = pCursor->key();
        KeyType key;
        uint32_t count = 0;
        pCursor->Seek(dbk::GetKeyPrefix(prefixType));

        for (; (count < maxNum) && pCursor->Valid(); pCursor->Next()) {
            dbk::ParseDbKey(slKey, prefixType, key);

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

private:
    mutable CLevelDBWrapper db; // // TODO: remove the mutable declare
};

template<typename KeyType, typename ValueType>
class CDBCache {

public:
    typedef typename map<KeyType, ValueType>::iterator Iterator;

public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CDBCache(): pBase(nullptr), pDbAccess(nullptr), prefixType(dbk::EMPTY) {};

    CDBCache(CDBCache<KeyType, ValueType> *pBaseIn): pBase(pBaseIn),
        pDbAccess(nullptr), prefixType(pBaseIn->prefixType) {
        assert(pBaseIn != nullptr);
    };

    CDBCache(CDBAccess *pDbAccessIn, dbk::PrefixType prefixTypeIn): pBase(nullptr),
        pDbAccess(pDbAccessIn), prefixType(prefixTypeIn) {
        assert(pDbAccessIn != nullptr);
    };

    void SetBase(CDBCache<KeyType, ValueType> *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(mapData.empty());
        pBase = pBaseIn;
        prefixType = pBaseIn->prefixType;
    };

    bool GetTopNElements(const int32_t maxNum, set<KeyType> &keys) {
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
            keys.insert(iter.first);
        }

        return keys.size() == maxNum;
    }

    bool GetData(const KeyType &key, ValueType &value) const {
        auto it = GetDataIt(key);
        if (it != mapData.end()) {
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


    bool HaveData(const KeyType &key) const {
        auto it = GetDataIt(key);
        return it != mapData.end();
    }

    bool EraseData(const KeyType &key) {
        Iterator it = GetDataIt(key);
        if (it != mapData.end()) {
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
            pDbAccess->BatchWrite<KeyType, ValueType>(prefixType, mapData);
        }

        mapData.clear();
    }

    bool UndoData(const CDbOpLog &dbOpLog) {
        assert(dbOpLog.GetPrefixType() == prefixType);
        KeyType key;
        ValueType value;
        dbOpLog.Get(key, value);
        mapData[key] = value;
        return true;
    }

    dbk::PrefixType GetPrefixType() const { return prefixType; }

private:
    Iterator GetDataIt(const KeyType &key) const {
        // key should not be empty
        if (db_util::IsEmpty(key)) {
            return mapData.end();
        }

        Iterator it = mapData.find(key);
        if (it != mapData.end()) {
            if (!db_util::IsEmpty(it->second)) {
                return it;
            }
        } else if (pBase != nullptr){
            auto it = pBase->GetDataIt(key);
            if (it != mapData.end()) {
                auto newIt = mapData.emplace(make_pair(key, it->second));
                assert(newIt.second); // TODO: throw error
                return newIt.first;
            }
        } else if (pDbAccess != NULL) {
            auto ptrValue = std::make_shared<ValueType>();

            if (pDbAccess->GetData(prefixType, key, *ptrValue)) {
                auto newIt = mapData.emplace(make_pair(key, *ptrValue));
                assert(newIt.second); // TODO: throw error
                return newIt.first;
            }
        }

        return mapData.end();
    };

    bool GetTopNElements(const int32_t maxNum, set<KeyType> &expiredKeys, set<KeyType> &keys) {
        if (!mapData.empty()) {
            uint32_t count = 0;
            auto iter      = mapData.begin();

            for (; (count < maxNum) && iter != mapData.end(); ++iter) {
                if (db_util::IsEmpty(iter.second)) {
                    expiredKeys.insert(iter.first);
                } else if (expiredKeys.count(iter.first) || keys.count(iter.first)) {
                    // TODO: log
                    continue;
                } else {
                    // Got a valid element.
                    keys.insert(iter.first);

                    ++count;
                }
            }
        }

        if (pBase != nullptr) {
            pBase->GetTopNElements(maxNum, expiredKeys, keys);
        } else if (pDbAccess != nullptr) {
            pDbAccess->GetTopNElements(maxNum, prefixType, expiredKeys, keys);
        }

        return true;
    }

private:
    mutable CDBCache<KeyType, ValueType> *pBase;
    CDBAccess *pDbAccess;
    dbk::PrefixType prefixType;
    mutable map<KeyType, ValueType> mapData;
};


template<typename ValueType>
class CDBSingleCache {
public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CDBSingleCache(): pBase(nullptr), pDbAccess(nullptr), prefixType(dbk::EMPTY) {};

    CDBSingleCache(CDBSingleCache<ValueType> *pBaseIn): pBase(pBaseIn),
        pDbAccess(nullptr), prefixType(pBaseIn->prefixType) {
        assert(pBaseIn != nullptr);
    };

    CDBSingleCache(CDBAccess *pDbAccessIn, dbk::PrefixType prefixTypeIn): pBase(nullptr),
        pDbAccess(pDbAccessIn), prefixType(prefixTypeIn) {
        assert(pDbAccessIn != nullptr);
    };

    void SetBase(CDBSingleCache<ValueType> *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(!ptrData);
        pBase = pBaseIn;
        prefixType = pBaseIn->prefixType;
    };

    bool GetData(ValueType &value) const {
        auto ptr = GetDataPtr();
        if (ptr) {
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


    bool HaveData() const {
        auto ptr = GetDataPtr();
        return ptr;
    }

    bool EraseData() {
        auto ptr = GetDataPtr();
        if (ptr) {
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
            pDbAccess->BatchWrite(prefixType, *ptrData);
        }

        ptrData = nullptr;
    }

    void UndoData(const CDbOpLog &dbOpLog) {
        assert(dbOpLog.GetPrefixType() == prefixType);
        if (!ptrData) {
            ptrData = make_shared<ValueType>();
        }
        dbOpLog.Get(*ptrData);
    }

    dbk::PrefixType GetPrefixType() const { return prefixType; }

private:
    std::shared_ptr<ValueType> GetDataPtr() const {

        assert(prefixType != dbk::EMPTY);
        if (ptrData) {
            if (!db_util::IsEmpty(*ptrData)) {
                return ptrData;
            }
        } else if (pBase != nullptr){
            auto ptr = pBase->GetDataPtr();
            if (ptr) {
                assert(!db_util::IsEmpty(*ptr));
                ptrData = std::make_shared<ValueType>(*ptr);
                return ptrData;
            }
        } else if (pDbAccess != NULL) {
            auto ptrDbData = std::make_shared<ValueType>();

            if (pDbAccess->GetData(prefixType, *ptrDbData)) {
                assert(!db_util::IsEmpty(*ptrDbData));
                ptrData = ptrDbData;
                return ptrData;
            }
        }
        return nullptr;
    };
private:
    mutable CDBSingleCache<ValueType> *pBase;
    CDBAccess *pDbAccess;
    dbk::PrefixType prefixType;
    mutable std::shared_ptr<ValueType> ptrData;
};

#endif//PERSIST_DB_ACCESS_H
