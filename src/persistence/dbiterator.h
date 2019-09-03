// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ITERATOR_H
#define PERSIST_DB_ITERATOR_H

#include "dbaccess.h"

class CCommonPrefixMatcher{
public:
    // empty prefix, will match all keys
    template<typename KeyType>
    static void MakeKeyByPrefix(const CNullObject &prefix, KeyType &keyObj) {
        db_util::SetEmpty(keyObj);
    }

    // 1/2: make 2 pair key object by 1 prefix
    template<typename T1, typename T2>
    static void MakeKeyByPrefix(const T1 &prefix, std::pair<T1, T2> &keyObj) {
        keyObj = make_pair<T1, T2>(prefix, db_util::MakeEmpty<T2>());
    }

    // 2/2: make 2 pair key object by 2 prefix, the 2nd prefix must support partial match
    template<typename T1, typename T2>
    static void MakeKeyByPrefix(const std::pair<T1, T2> &prefix, std::pair<T1, T2> &keyObj) {
        keyObj = prefix;
    }

    // 1/3: make 3 tuple key object by 1 prefix
    template<typename T1, typename T2, typename T3>
    static void MakeKeyByPrefix(const T1 &prefix, std::tuple<T1, T2, T3> &keyObj) {
        keyObj = make_tuple<T1, T2, T3>(prefix, db_util::MakeEmpty<T2>(), db_util::MakeEmpty<T3>());
    }

    // 2/3: make 3 tuple key object by 2 pair prefix
    template<typename T1, typename T2, typename T3>
    static void MakeKeyByPrefix(const std::pair<T1, T2> &prefix, std::tuple<T1, T2, T3> &keyObj) {
        keyObj = make_tuple<T1, T2, T3>(prefix.first, prefix.second, db_util::MakeEmpty<T3>());
    }

    // empty prefix, will match all keys
    template<typename KeyType>
    static bool MatchPrefix(KeyType &key, const CNullObject &prefix) {
        return true;
    }


    // 1/2: check 2 pair key match 1 prefix
    template<typename T1, typename T2>
    static bool MatchPrefix(const std::pair<T1, T2> &key, const T1 &prefix) {
        return MatchPrefix(key.first, prefix);
    }

    // 2/2: check 2 pair key match 2 prefix, the 2nd prefix must support partial match
    template<typename T1, typename T2>
    static bool MatchPrefix(const std::pair<T1, T2> &key, const std::pair<T1, T2> &prefix) {
        return key.first == prefix.first && MatchPrefix(key.second, prefix.second);
    }

    // 1/3: check 3 tuple key match 1 prefix
    template<typename T1, typename T2, typename T3>
    static bool MatchPrefix(const std::tuple<T1, T2, T3> &key, const T1 &prefix) {
        return MatchPrefix(std::get<0>(key), prefix);
    }

    // 2/3: check 3 tuple key match 2 pair prefix
    template<typename T1, typename T2, typename T3>
    static bool MatchPrefix(const std::tuple<T1, T2, T3> &key, const std::pair<T1, T2> &prefix) {
        return std::get<0>(key) == prefix.first && MatchPrefix(std::get<1>(key), prefix.second);
    }


    // match string, support part string match
    template<typename C>
    static bool MatchPrefix(const basic_string<C> &key, const basic_string<C> &prefix) {
        return key.compare(0, prefix.size(), prefix) == 0;
    }

    template<typename T1>
    static bool MatchPrefix(const T1 &key, const T1 &prefix) {
        return key == prefix;
    }
};

template<typename CacheType, typename PrefixElement>
class CBasePrefixIterator {
public:
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
    KeyType key;
    ValueType value;
    bool is_valid;
protected:
    CacheType &db_cache;
    PrefixElement prefix_element;

public:
    CBasePrefixIterator(CacheType &dbCache, const PrefixElement &prefixElement)
        : key(), value(), is_valid(false), db_cache(dbCache), prefix_element(prefixElement) {}

    bool IsValid() {return is_valid;}
};

//CDBPrefixIterator
template<typename CacheType, typename PrefixElement, typename PrefixMatcher>
class CDBPrefixIterator: public CBasePrefixIterator<CacheType, PrefixElement> {
public:
    typedef CBasePrefixIterator<CacheType, PrefixElement> Base;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
private:
    shared_ptr<leveldb::Iterator> p_db_it;
    string prefix;
public:
    CDBPrefixIterator(CacheType &dbCache, const PrefixElement &prefixElement)
        : Base(dbCache, prefixElement), p_db_it(nullptr) {}

    bool First() {
        return First(db_util::MakeEmpty<KeyType>());
    }

    bool First(const KeyType &lastKey) {
        p_db_it = Base::db_cache.GetDbAccessPtr()->NewIterator();
        prefix = dbk::GenDbKey(CacheType::PREFIX_TYPE, Base::prefix_element);
        if (!db_util::IsEmpty(lastKey)) {
            string lastKeyStr = dbk::GenDbKey(CacheType::PREFIX_TYPE, lastKey);
            p_db_it->Seek(lastKeyStr);
            if (p_db_it->Valid() && p_db_it->key() == Slice(lastKeyStr)) {
                p_db_it->Next(); // skip the last key
            }
        } else {
            p_db_it->Seek(prefix);
        }
        return Parse();
    }

    bool Next() {
        p_db_it->Next();
        return Parse();
    }
private:
    inline bool Parse() {
        Base::is_valid = false;
        if (!p_db_it->Valid() || !p_db_it->key().starts_with(prefix)) return false;

        const leveldb::Slice &slKey = p_db_it->key();
        const leveldb::Slice &slValue = p_db_it->value();
        if (!ParseDbKey(slKey, CacheType::PREFIX_TYPE, Base::key)) {
            throw runtime_error(strprintf("CDBPrefixIterator::Parse db key error! key=%s", HexStr(slKey.ToString())));
        }
        assert(PrefixMatcher::MatchPrefix(Base::key, Base::prefix_element));

        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> Base::value;
        } catch(std::exception &e) {
            throw runtime_error(strprintf("CDBPrefixIterator::Parse db value error! %s", HexStr(slValue.ToString())));
        }
        Base::is_valid = true;
        return true;
    }
};

//CMapPrefixIterator
template<typename CacheType, typename PrefixElement, typename PrefixMatcher>
class CMapPrefixIterator: public CBasePrefixIterator<CacheType, PrefixElement> {
public:
    typedef CBasePrefixIterator<CacheType, PrefixElement> Base;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
private:
    typename CacheType::Iterator map_it;
    string prefix;
public:
    CMapPrefixIterator(CacheType &dbCache, const PrefixElement &prefixElement)
        : Base(dbCache, prefixElement), map_it(dbCache.GetMapData().end()) {}

    bool First() {
        return First(db_util::MakeEmpty<KeyType>());
    }

    bool First(const KeyType &lastKey) {
        KeyType keyObj;
        if (db_util::IsEmpty(lastKey)) {
            PrefixMatcher::MakeKeyByPrefix(Base::prefix_element, keyObj);
        } else {
            keyObj = lastKey;
        }
        map_it = Base::db_cache.GetMapData().upper_bound(keyObj);
        return Parse();
    }

    bool Next() {
        map_it++;
        return Parse();
    }

private:
    inline bool Parse() {
        Base::is_valid = false;
        if (map_it == Base::db_cache.GetMapData().end())  return false;
        Base::key = map_it->first;
        if (!PrefixMatcher::MatchPrefix(Base::key, Base::prefix_element))
            return false;
        Base::value = map_it->second;
        Base::is_valid = true;
        return true;
    }
};

template<typename CacheType, typename PrefixElement = CNullObject, typename PrefixMatcher = CCommonPrefixMatcher>
class CDBListGetter {
public:
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
    typedef pair<KeyType, ValueType> DataListItem;
    typedef std::vector<DataListItem> DataList;
public:
    DataList data_list; // exec result
    bool has_more;
    bool have_next;

private:
    CacheType &db_cache;
    PrefixElement prefix_element;
    KeyType last_key;
    uint32_t max_count;
public:
    CDBListGetter(CacheType &dbCache)
        : CDBListGetter(dbCache, db_util::MakeEmpty<PrefixElement>()) {}

    CDBListGetter(CacheType &dbCache, const PrefixElement &prefixElement)
        : CDBListGetter(dbCache, prefixElement, 0) {}

    CDBListGetter(CacheType &dbCache, const PrefixElement &prefixElement, uint32_t maxCount)
        : CDBListGetter(dbCache, prefixElement, maxCount, db_util::MakeEmpty<KeyType>()) {}

    CDBListGetter(CacheType &dbCache, const PrefixElement &prefixElement, uint32_t maxCount,
                  const KeyType &lastKey)
        : data_list(),
          has_more(false),
          have_next(false),
          db_cache(dbCache),
          prefix_element(prefixElement),
          last_key(lastKey),
          max_count(maxCount){}

    bool Execute() {
        CMapPrefixIterator<CacheType, PrefixElement, PrefixMatcher> mapIt(db_cache, prefix_element);
        CDBPrefixIterator<CacheType, PrefixElement, PrefixMatcher> dbIt(db_cache, prefix_element);
        mapIt.First(last_key);
        dbIt.First(last_key);
        db_util::SetEmpty(last_key);

        while(mapIt.IsValid() || dbIt.IsValid()) {
            bool isMapData = true, isSameKey = false;
            if (mapIt.IsValid() && dbIt.IsValid()) {
                if (dbIt.key < mapIt.key) {
                    isMapData = false;
                } else if (mapIt.key < dbIt.key) { // dbIt.key >= mapIt.key
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

            shared_ptr<DataListItem> pItem = nullptr;
            if (isMapData) {
                if (!db_util::IsEmpty(mapIt.value)) {
                    pItem = make_shared<DataListItem>(mapIt.key, mapIt.value);
                } // else ignore
                mapIt.Next();
                if (isSameKey) {
                    assert(dbIt.IsValid());
                    // if same key and map has no more valid data, must use db next data
                    dbIt.Next();
                }
            } else { // is db data
                pItem = make_shared<DataListItem>(dbIt.key, dbIt.value);
                dbIt.Next();
            }
            if (pItem != nullptr) {
                if (max_count != 0 && data_list.size() > max_count) {
                    have_next = true;
                    break;
                }
                data_list.push_back(*pItem);
            }
        }
        return true;
    }
};

#endif //PERSIST_DB_ITERATOR_H