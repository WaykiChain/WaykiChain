// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ITERATOR_H
#define PERSIST_DB_ITERATOR_H

#include "dbaccess.h"

namespace prefix_util {

    // empty prefix, will match all keys
    template<typename KeyType>
    void MakeKeyByPrefix(const CNullObject &prefix, KeyType &keyObj) {
        db_util::SetEmpty(keyObj);
    }

    // 1/2: make 2 pair key object by 1 prefix
    template<typename T1, typename T2>
    void MakeKeyByPrefix(const T1 &prefix, std::pair<T1, T2> &keyObj) {
        keyObj = make_pair<T1, T2>(prefix, db_util::MakeEmpty<T2>());
    }

    // 1/3: make 3 tuple key object by 1 prefix
    template<typename T1, typename T2, typename T3>
    void MakeKeyByPrefix(const T1 &prefix, std::tuple<T1, T2, T3> &keyObj) {
        keyObj = make_tuple<T1, T2, T3>(prefix, db_util::MakeEmpty<T2>(), db_util::MakeEmpty<T3>());
    }

    // 2/3: make 3 tuple key object by 2 pair prefix
    template<typename T1, typename T2, typename T3>
    void MakeKeyByPrefix(const std::pair<T1, T2> &prefix, std::tuple<T1, T2, T3> &keyObj) {
        keyObj = make_tuple<T1, T2, T3>(prefix.first, prefix.second, db_util::MakeEmpty<T3>());
    }

    // empty prefix, will match all keys
    template<typename KeyType>
    bool MatchPrefix(KeyType &key, const CNullObject &prefix) {
        return true;
    }

    // 1/2: check 2 pair key match 1 prefix
    template<typename T1, typename T2>
    bool MatchPrefix(std::pair<T1, T2> &key, const T1 &prefix) {
        return key.first == prefix;
    }

    // 1/3: check 3 tuple key match 1 prefix
    template<typename T1, typename T2, typename T3>
    bool MatchPrefix(std::tuple<T1, T2, T3> &key, const T1 &prefix) {
        return std::get<0>(key) == prefix;
    }

    // 2/3: check 3 tuple key match 2 pair prefix
    template<typename T1, typename T2, typename T3>
    bool MatchPrefix(std::tuple<T1, T2, T3> &key, const std::pair<T1, T2> &prefix) {
        return std::get<0>(key) == prefix.first && std::get<1>(key) == prefix.second;
    }
}

//CDBPrefixIterator
template<typename CacheType, typename PrefixElement>
class CDBPrefixIterator {
public:
    typename CacheType::KeyType key;
    typename CacheType::ValueType value;
    bool is_valid;
private:
    CDBAccess &db_access;
    PrefixElement prefix_element;
    shared_ptr<leveldb::Iterator> p_db_it;
    string prefix;
public:
    CDBPrefixIterator(CDBAccess &dbAccess, const PrefixElement &prefixElement)
        : key(), value(), is_valid(false), db_access(dbAccess), prefix_element(prefixElement), p_db_it(nullptr) {

    }

    bool First() {
        p_db_it = db_access.NewIterator();
        prefix = dbk::GenDbKey(CacheType::PREFIX_TYPE, prefix_element);
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
        if (!ParseDbKey(slKey, CacheType::PREFIX_TYPE, key)) {
            throw runtime_error(strprintf("CDBPrefixIterator::Parse db key error! key=%s", HexStr(slKey.ToString())));
        }
        assert(prefix_util::MatchPrefix(key, prefix_element));

        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            throw runtime_error(strprintf("CDBPrefixIterator::Parse db value error! %s", HexStr(slValue.ToString())));
        }
        is_valid = true;
        return true;
    }
};

template<typename CacheType, typename PrefixElement>
class CMapPrefixIterator {
public:
    typename CacheType::KeyType key;
    typename CacheType::ValueType value;
    bool is_valid;
private:
    typename CacheType::Map &data_map;
    typename CacheType::Iterator map_it;
    PrefixElement prefix_element;
public:
    CMapPrefixIterator(CacheType &dbCache, PrefixElement prefixElement)
        : key(), value(), is_valid(false), data_map(dbCache.GetMapData()), map_it(data_map.end()), prefix_element(prefixElement) {}

    bool First() {
        typename CacheType::KeyType keyObj;
        prefix_util::MakeKeyByPrefix(prefix_element, keyObj);
        map_it = data_map.upper_bound(keyObj);
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
        if (!prefix_util::MatchPrefix(key, prefix_element))
            return false;
        value = map_it->second;
        return true;
    }
};

template<typename CacheType, typename PrefixElement = CNullObject>
class CDBListGetter {
public:
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
    typedef pair<KeyType, ValueType> DataListItem;
    typedef std::vector<DataListItem> DataList;
public:
    DataList data_list; // exec result
private:
    CacheType &db_cache;
    CDBAccess &db_access;
    PrefixElement prefix_element;
public:
    CDBListGetter(CacheType &dbCache, const PrefixElement &prefixElement)
        : db_cache(dbCache), db_access(*dbCache.GetDbAccessPtr()), prefix_element(prefixElement) {
    }

    CDBListGetter(CacheType &dbCache)
        : CDBListGetter(dbCache, PrefixElement()) {
    }

    bool Execute() {
        CMapPrefixIterator<CacheType, PrefixElement> mapIt(db_cache, prefix_element);
        CDBPrefixIterator<CacheType, PrefixElement> dbIt(db_access, prefix_element);
        mapIt.First();
        dbIt.First();
        //auto mapIt = mapRangePair.first;
        while(mapIt.IsValid() || dbIt.IsValid()) {
            bool useMap = true, isSameKey = false;
            if (mapIt.IsValid() && dbIt.IsValid()) {
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
                    data_list.push_back(make_pair(mapIt.key, mapIt.value));
                } // else ignore
                mapIt.Next();
                if (isSameKey && !mapIt.IsValid() && dbIt.IsValid()) {
                    // if the same key and map has no more valid data, must use db next data
                    dbIt.Next();
                }
            } else { // use db
                data_list.push_back(make_pair(dbIt.key, dbIt.value));
                dbIt.Next();
            }
        }
        return true;
    }
};

#endif //PERSIST_DB_ITERATOR_H