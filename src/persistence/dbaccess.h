// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ACCESS_H
#define PERSIST_DB_ACCESS_H


/**
 * Empty functions
 */
namespace db_util {
    template<typename C>
    bool IsEmpty(const basic_string<C> &val) {
        return val.empty();
    }

    template<typename C>
    bool SetEmpty(basic_string<C> &val) {
        return val.clear();
    }

    template<typename T, typename A> 
    bool IsEmpty(const vector<T, A>& val) {
        return val.empty();
    }

    template<typename T, typename A> 
    bool SetEmpty( vector<T, A>& val) {
        return val.clear();
    }

    template<typename T>
    bool IsEmpty(const T& val) {
        return val.IsEmpty();
    }

    template<typename T>
    bool SetEmpty(const T& val) {
        return val.SetEmpty();
    }
}

class CDBAccess {
public:
    CDBAccess(const string &name, size_t nCacheSize, bool fMemory, bool fWipe) :
                db( GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe ) {}

    template<typename KeyType, typename ValueType>
    virtual bool GetData(PrefixType prefixType, const KeyType &key, ValueType &value) {
        string keyStr = GenDbKey(prefixType, keyId);
        return db.Read(keyStr, account);
    }

    template<typename KeyType, typename ValueType>
    virtual bool HaveData(PrefixType prefixType, const KeyType &key) {
        string keyStr = GenDbKey(prefixType, keyId);
        return db.Exists(keyStr);
    }


    template<typename KeyType, typename ValueType>
    void BatchWrite(PrefixType prefixType, const map<KeyType, ValueType> &mapData) {
        CLevelDBBatch batch;
        auto iterAccount = mapAccounts.begin();
        for (auto it : mapData) {
            if (db_util::IsEmpty(it->second)) {
                batch.Erase(dbk::GenDbKey(prefixType, it->first));
            } else {
                batch.Write(dbk::GenDbKey(prefixType, it->first);
            }
        }
        return db.WriteBatch(batch, true);
    }

private:
    CLevelDBWrapper db;
}

template<typename KeyType, typename ValueType>
class CDBCache {
public:
    typedef map<KeyType, ValueType>::Iterator Iterator;
public:
    CDBCache(IDBView<KeyType, ValueType> *pBaseIn): pBase(pBaseIn), pDbAccess(null) { }

    CDBCache(CDBAccess *dbAccess): pBase(nullptr), pDbAccess(nullptr) { }

    bool GetData(const KeyType &key, ValueType &value) {
        auto it = GetDataIt(key);
        if (it != mapData.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    void SetData(const KeyType &key, const ValueType &value) {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        mapData[key] = value;
        return true;
    };

    bool EraseData(const KeyType &key) {
        auto it = GetDataIt();
        if (it != mapData.end()) {
            db_util::SetEmpty(*value);
        }
        return true;
    }

    void Flush() {
        assert(pBase != nullptr || pDbAccess != nullptr);
        if (pBase != nullptr) {
            assert(pDbAccess == nullptr);
            for (auto it : this.mapdata) {
                pBase->mapData[it.first] = it.second;
            }
            this.mapData.clear();
        } else if (pDbAccess != nullptr) {
            assert(pBase == nullptr);
            pDbAccess.BatchWrite(mapData);
        }

        this.mapData.clear();
    }

private:
    Iterator GetDataIt(const KeyType &key) {
        // key should not be empty
        if db_util::IsEmpty(key) return nullptr;
        Iterator it = mapData.find(key);
        if (it != mapData.end()) {
            ValueType *value = &it->second;
            if (!db_util::IsEmpty(it->second)) {
                return value;
            }
        } else if (pBase != nullptr){
            auto it = pBase->GetDataIt(key);
            if (it != mapData.end()) {
                auto newIt = mapData.insert(make_pair(key, *value));
                assert(newIt.second); // TODO: throw error
                return newIt->first;
            }
        } else if (pDbAccess != NULL) {
            shared_ptr<ValueType> value(new ValueType());
            if (pDbAccess->GetData(key, value)) {
                auto newIt = mapData.insert(make_pair(key, value));
                assert(newIt.second); // TODO: throw error
                return newIt->first;
            }
        }
        return mapData.end();
    };

private:
    CDBCache *pBase;
    CDBAccess *pDbAccess;

    map<KeyType, ValueType> mapData;
}


#endif//PERSIST_DB_ACCESS_H