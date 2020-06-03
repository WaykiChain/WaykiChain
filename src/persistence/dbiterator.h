// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ITERATOR_H
#define PERSIST_DB_ITERATOR_H

#include "dbaccess.h"

template<typename CacheType>
class CDBBaseIterator {
public:
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
public:
    CacheType &db_cache;
    shared_ptr<KeyType> sp_key = nullptr;
    shared_ptr<ValueType> sp_value = nullptr;
    bool is_valid = false;

public:
    CDBBaseIterator(CacheType &dbCache)
        : db_cache(dbCache),
          sp_key(make_shared<KeyType>()),
          sp_value(make_shared<ValueType>()),
          is_valid(false) {}

    virtual bool First() = 0;

    // seek to the first element not less than the given key
    virtual bool Seek(const KeyType *pKey) = 0;
    // seek to the first element greater than the given key
    virtual bool SeekUpper(const KeyType *pKey) = 0;

    virtual bool Next() = 0;

    virtual bool IsValid() const {
        return is_valid;
    }

    const KeyType& GetKey() const {
        assert(IsValid());
        return *sp_key;
    }

    const ValueType& GetValue() const {
        assert(IsValid());
        return *sp_value;
    }
};

template<typename CacheType>
class CDBAccessIterator: public CDBBaseIterator<CacheType> {
public:
    typedef CDBBaseIterator<CacheType> Base;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
private:
    shared_ptr<leveldb::Iterator> p_db_it;
public:
    CDBAccessIterator(CacheType &dbCache)
        : Base(dbCache), p_db_it(nullptr) {
        p_db_it = this->db_cache.GetDbAccessPtr()->NewIterator();
    }

    bool First() {
        const string &prefix = dbk::GetKeyPrefix(CacheType::PREFIX_TYPE);
        p_db_it->Seek(prefix);
        return ProcessData();
    }

    bool Seek(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        string lastKeyStr = dbk::GenDbKey(CacheType::PREFIX_TYPE, *pKey);
        p_db_it->Seek(lastKeyStr);

        return ProcessData();
    }

    bool SeekUpper(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        string lastKeyStr = dbk::GenDbKey(CacheType::PREFIX_TYPE, *pKey);
        p_db_it->Seek(lastKeyStr);
        if (p_db_it->Valid() && p_db_it->key() == Slice(lastKeyStr)) {
            p_db_it->Next(); // skip the last key
        }

        return ProcessData();
    }

    bool Next() {
        p_db_it->Next();
        return ProcessData();
    }
private:
    inline bool ProcessData() {
        const string& prefixStr = dbk::GetKeyPrefix(CacheType::PREFIX_TYPE);
        this->is_valid = false;
        if (!p_db_it->Valid() || !p_db_it->key().starts_with(prefixStr)) return false;

        const leveldb::Slice &slKey = p_db_it->key();
        const leveldb::Slice &slValue = p_db_it->value();
        if (!ParseDbKey(slKey, CacheType::PREFIX_TYPE, *this->sp_key)) {
            throw runtime_error(strprintf("CDBAccessIterator::ProcessData db key error! key=%s", HexStr(slKey.ToString())));
        }

        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> *this->sp_value;
        } catch(std::exception &e) {
            throw runtime_error(strprintf("CDBAccessIterator::ProcessData db value error! %s", HexStr(slValue.ToString())));
        }
        this->is_valid = true;
        return true;
    }
};

//CMapPrefixIterator
template<typename CacheType>
class CCacheMapIterator: public CDBBaseIterator<CacheType> {
public:
    typedef CDBBaseIterator<CacheType> Base;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
private:
    typename CacheType::Iterator map_it;
public:
    CCacheMapIterator(CacheType &dbCache) : Base(dbCache), map_it(dbCache.GetMapData().end()) {}

    virtual bool First() {
        map_it = this->db_cache.GetMapData().begin();
        return ProcessData();
    }

    bool Seek(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        map_it = this->db_cache.GetMapData().lower_bound(*pKey);
        return ProcessData();
    }

    bool SeekUpper(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        map_it = this->db_cache.GetMapData().upper_bound(*pKey);
        return ProcessData();
    }

    bool Next() {
        assert(this->IsValid());
        map_it++;
        return ProcessData();
    }

private:
    inline bool ProcessData() {
        this->is_valid = false;
        if (map_it == this->db_cache.GetMapData().end())  return false;
        *this->sp_key = map_it->first;
        *this->sp_value = *map_it->second;
        this->is_valid = true;
        return true;
    }
};

template<typename CacheType>
class CDBCacheIteratorImpl: public CDBBaseIterator<CacheType> {
public:
    typedef CDBBaseIterator<CacheType> Base;
    typedef CCacheMapIterator<CacheType> CacheMapIt;
    typedef CDBAccessIterator<CacheType> DbAccessIt;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;

public:
    static shared_ptr<CDBCacheIteratorImpl> Create(CacheType &cache) {
        assert(cache.GetBasePtr() != nullptr || cache.GetDbAccessPtr() != nullptr);
        shared_ptr<CDBCacheIteratorImpl> spIt;
        if (cache.GetBasePtr() != nullptr) {
            spIt = make_shared<CDBCacheIteratorImpl>(cache, Create(*cache.GetBasePtr()));
        } else {
            spIt = make_shared<CDBCacheIteratorImpl>(cache, make_shared<DbAccessIt>(cache));
        }
        return spIt;
    }
public:
    CDBCacheIteratorImpl(CacheType &dbCacheIn, shared_ptr<Base> spBaseItIn)
        : Base(dbCacheIn), sp_map_it(make_shared<CacheMapIt>(dbCacheIn)), sp_base_it(spBaseItIn) {}

    bool First() {
        sp_map_it->First();
        sp_base_it->First();
        return ProcessData();
    }

    bool Seek(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        sp_map_it->Seek(pKey);
        sp_base_it->Seek(pKey);
        return ProcessData();
    }

    bool SeekUpper(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        sp_map_it->SeekUpper(pKey);
        sp_base_it->SeekUpper(pKey);
        return ProcessData();
    }

    const KeyType& GetKey() {
        assert(this->is_valid);
        return *this->sp_key;
    }


    bool Next() {
        InternalNext();
        return ProcessData();
    }

    // got count
    int32_t GotCount() const {
        return count;
    }

private:
    shared_ptr<CacheMapIt> sp_map_it = nullptr;
    shared_ptr<Base> sp_base_it = nullptr;
    bool is_map_data = false;
    bool is_same_key = false;
    int32_t count = 0;

    virtual void InternalNext() {
        if (is_map_data) {
            sp_map_it->Next();
            if (is_same_key) {
                assert(sp_base_it->IsValid());
                // if same key and map has no more valid data, must use db next data
                sp_base_it->Next();
            }
        } else { // is base data
            sp_base_it->Next();
        }
    }

    virtual bool ProcessData() {
        this->is_valid = sp_map_it->IsValid() || sp_base_it->IsValid();
        if (!this->is_valid)
            return false;

        while(this->is_valid) {
            ProcessGetData();
            if (!db_util::IsEmpty(*this->sp_value)) {
                break;
            }
            InternalNext();
            this->is_valid = sp_map_it->IsValid() || sp_base_it->IsValid();
        }
        if (this->is_valid)
            count++;
        return this->is_valid;
    }

    virtual void ProcessGetData() {
        is_map_data = true;
        is_same_key = false;
        if (sp_map_it->IsValid() && sp_base_it->IsValid()) {
            if (*sp_base_it->sp_key < *sp_map_it->sp_key) {
                is_map_data = false;
            } else if (*sp_map_it->sp_key < *sp_base_it->sp_key) { // dbIt.key >= sp_map_it->key
                is_map_data = true;
            } else {// dbIt.key == sp_map_it->key
                is_map_data = true;
                is_same_key = true;
            }
        } else if (sp_map_it->IsValid()) {
            is_map_data = true;
        } else { // dbIt.IsValid())
            is_map_data = false;
        }

        if (is_map_data) {
            this->sp_key = sp_map_it->sp_key;
            this->sp_value = sp_map_it->sp_value;
        } else { // is db data
            this->sp_key = sp_base_it->sp_key;
            this->sp_value = sp_base_it->sp_value;
        }
    }
};

template<typename CacheType>
class CDbIterator {
public:
    typedef CDBCacheIteratorImpl<CacheType> IteratorImpl;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;

    CDbIterator(CacheType &dbCacheIn): sp_it_Impl(IteratorImpl::Create(dbCacheIn)){

    }
    virtual bool First() {
        return sp_it_Impl->First();
    }

    virtual bool Seek(const KeyType *pKey) {
        return sp_it_Impl->Seek(pKey);
    }

    virtual bool SeekUpper(const KeyType *pKey) {
        return sp_it_Impl->SeekUpper(pKey);
    }

    virtual bool Next() {
        return sp_it_Impl->Next();
    }

    virtual bool IsValid() const {
        return sp_it_Impl->IsValid();
    }

    const KeyType& GetKey() const {
        return sp_it_Impl->GetKey();
    }

    const ValueType& GetValue() const {
        return sp_it_Impl->GetValue();
    }

    int32_t GotCount() const {
        return sp_it_Impl->GotCount();
    }
protected:
    shared_ptr<IteratorImpl> sp_it_Impl;
};


template<typename CacheType>
std::shared_ptr<CDbIterator<CacheType>> MakeDbIterator(CacheType &cache) {
    return std::make_shared<CDbIterator<CacheType>>(cache);
}

struct CommonPrefixMatcher {
    // empty prefix, will match all keys
    template<typename KeyType>
    static void MakeKeyByPrefix(const CNullObject &prefix, KeyType &keyObj) {
        db_util::SetEmpty(keyObj);
    }

    // 1/2: make 2 pair key object by 1 prefix
    template<typename T0, typename T1>
    static void MakeKeyByPrefix(const T0 &prefix, std::pair<T0, T1> &keyObj) {
        keyObj = pair<T0, T1>(prefix, db_util::MakeEmpty<T1>());
    }

    // 2/2: make 2 pair key object by 2 prefix, the 2nd prefix must support partial match
    template<typename T0, typename T1>
    static void MakeKeyByPrefix(const std::pair<T0, T1> &prefix, std::pair<T0, T1> &keyObj) {
        keyObj = prefix;
    }

    // 1/3: make 3 tuple key object by 1 prefix
    template<typename T0, typename T1, typename T2>
    static void MakeKeyByPrefix(const T0 &prefix, std::tuple<T0, T1, T2> &keyObj) {
        keyObj = tuple<T0, T1, T2>(prefix, db_util::MakeEmpty<T1>(), db_util::MakeEmpty<T2>());
    }

    // 2/3: make 3 tuple key object by 2 pair prefix
    template<typename T0, typename T1, typename T2>
    static void MakeKeyByPrefix(const std::pair<T0, T1> &prefix, std::tuple<T0, T1, T2> &keyObj) {
        keyObj = tuple<T0, T1, T2>(prefix.first, prefix.second, db_util::MakeEmpty<T2>());
    }

    // 1/4: make 4 tuple key object by 1 pair prefix
    template<typename T0, typename T1, typename T2, typename T3>
    static void MakeKeyByPrefix(const T0 &prefix, std::tuple<T0, T1, T2, T3> &keyObj) {
        keyObj = tuple<T0, T1, T2, T3>(prefix, db_util::MakeEmpty<T1>(),
            db_util::MakeEmpty<T2>(), db_util::MakeEmpty<T3>());
    }

    // empty prefix, will match all keys
    template<typename KeyType>
    static bool MatchPrefix(KeyType &key, const CNullObject &prefix) {
        return true;
    }


    // 1/2: check 2 pair key match 1 prefix
    template<typename T0, typename T1>
    static bool MatchPrefix(const std::pair<T0, T1> &key, const T0 &prefix) {
        return MatchPrefix(key.first, prefix);
    }

    // 2/2: check 2 pair key match 2 prefix, the 2nd prefix must support partial match
    template<typename T0, typename T1>
    static bool MatchPrefix(const std::pair<T0, T1> &key, const std::pair<T0, T1> &prefix) {
        return key.first == prefix.first && MatchPrefix(key.second, prefix.second);
    }

    // 1/3: check 3 tuple key match 1 prefix
    template<typename T0, typename T1, typename T2>
    static bool MatchPrefix(const std::tuple<T0, T1, T2> &key, const T0 &prefix) {
        return MatchPrefix(std::get<0>(key), prefix);
    }

    // 2/3: check 3 tuple key match 2 pair prefix
    template<typename T0, typename T1, typename T2>
    static bool MatchPrefix(const std::tuple<T0, T1, T2> &key, const std::pair<T0, T1> &prefix) {
        return std::get<0>(key) == prefix.first && MatchPrefix(std::get<1>(key), prefix.second);
    }

    // 1/4: check 4 tuple key match 1 prefix
    template<typename T0, typename T1, typename T2, typename T3>
    static bool MatchPrefix(const std::tuple<T0, T1, T2, T3> &key, const T0 &prefix) {
        return MatchPrefix(std::get<0>(key), prefix);
    }

    // match string, support part string match
    template<uint32_t MAX_KEY_SIZE>
    static bool MatchPrefix(const dbk::CDBTailKey<MAX_KEY_SIZE> &key, const dbk::CDBTailKey<MAX_KEY_SIZE> &prefix) {
        return key.StartWith(prefix);
    }

    template<typename T>
    static bool MatchPrefix(const T &key, const T &prefix) {
        return key == prefix;
    }
};
template<typename CacheType, typename PrefixElement, typename PrefixMatcher = CommonPrefixMatcher>
class CDBPrefixIterator: public CDbIterator<CacheType> {
private:
    typedef CDbIterator<CacheType> Base;
    typedef typename CacheType::KeyType KeyType;
    typedef typename CacheType::ValueType ValueType;
protected:
    PrefixElement prefix_element;
public:
    CDBPrefixIterator(CacheType &dbCache, const PrefixElement &prefixElementIn)
        : Base(dbCache), prefix_element(prefixElementIn) {}

    virtual bool First() {
        KeyType lastKey;
        PrefixMatcher::MakeKeyByPrefix(prefix_element, lastKey);
        return Base::Seek(&lastKey);
    }

    virtual bool Seek(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        return this->sp_it_Impl->Seek(pKey);
    }

    virtual bool SeekUpper(const KeyType *pKey) {
        if (pKey == nullptr || db_util::IsEmpty(*pKey))
            return First();
        return this->sp_it_Impl->SeekUpper(pKey);
    }

    virtual bool IsValid() const {
        return Base::IsValid() && PrefixMatcher::MatchPrefix(this->GetKey(), prefix_element);
    }

    const PrefixElement& GetPrefixElement() const {
        return prefix_element;
    }
};

template<typename CacheType, typename PrefixElement>
shared_ptr<CDBPrefixIterator<CacheType, PrefixElement, CommonPrefixMatcher>> MakeDbPrefixIterator(CacheType &cache, const PrefixElement &prefixElement){
    return make_shared<CDBPrefixIterator<CacheType, PrefixElement, CommonPrefixMatcher>>(cache, prefixElement);
}

#endif //PERSIST_DB_ITERATOR_H