// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ACCESS_H
#define PERSIST_DB_ACCESS_H

#include "commons/types.h"
#include "dbconf.h"
#include "leveldbwrapper.h"

#include <string>
#include <tuple>
#include <vector>
#include <optional>

using namespace std;

/**
 * Empty functions
 */
namespace db_util {

#define DEFINE_NUMERIC_DB_FUNC(type) \
    inline bool IsEmpty(const type val) { return val == 0; } \
    inline void SetEmpty(type &val) { val = 0; } \
    inline string ToString(const type &val) { return std::to_string(val); }

    // bool
    inline bool IsEmpty(const bool val) { return val == false; }
    inline void SetEmpty(bool &val) { val = false; }
    inline string ToString(const bool &val) { return val ? "true" : "false"; }

    DEFINE_NUMERIC_DB_FUNC(int32_t)
    DEFINE_NUMERIC_DB_FUNC(uint8_t)
    DEFINE_NUMERIC_DB_FUNC(uint16_t)
    DEFINE_NUMERIC_DB_FUNC(uint32_t)
    DEFINE_NUMERIC_DB_FUNC(uint64_t)

    // string
    template<typename C> bool IsEmpty(const basic_string<C> &val);
    template<typename C> void SetEmpty(basic_string<C> &val);
    template<typename C> string ToString(const basic_string<C> &val);

    // vector
    template<typename T, typename A> bool IsEmpty(const vector<T, A>& val);
    template<typename T, typename A> void SetEmpty(vector<T, A>& val);
    template<typename T, typename A> string ToString(const vector<T, A>& val);
    //shared_ptr
    template <typename T> bool  IsEmpty(const std::shared_ptr<T>& val) {
        return val == nullptr || (*val).IsEmpty();
    }
    template <typename T> void SetEmpty(std::shared_ptr<T>& val) {
        if(val == nullptr){
            val = make_shared<T>();
        }
        (*val).SetEmpty();
    }

    //optional
    template<typename T> bool IsEmpty(const std::optional<T> val);
    template<typename T> void SetEmpty(std::optional<T>& val);
    template<typename T> string ToString(const std::optional<T>& val);

    // set
    template<typename K, typename Pred, typename A> bool IsEmpty(const set<K, Pred, A>& val);
    template<typename K, typename Pred, typename A> void SetEmpty(set<K, Pred, A>& val);
    template<typename K, typename Pred, typename A> string ToString(const set<K, Pred, A>& val);

    // map
    template<typename K, typename T, typename Pred, typename A> bool IsEmpty(const map<K, T, Pred, A>& val);
    template<typename K, typename T, typename Pred, typename A> void SetEmpty(map<K, T, Pred, A>& val);
    template<typename K, typename T, typename Pred, typename A> string ToString(const map<K, T, Pred, A>& val);

    // 2 pair
    template<typename K, typename T> bool IsEmpty(const std::pair<K, T>& val);
    template<typename K, typename T> void SetEmpty(std::pair<K, T>& val);
    template<typename K, typename T> string ToString(const std::pair<K, T>& val);


    // 2 tuple
    template<typename T0, typename T1> bool IsEmpty(const std::tuple<T0, T1>& val);
    template<typename T0, typename T1> void SetEmpty(std::tuple<T0, T1>& val);
    template<typename T0, typename T1> string ToString(const std::tuple<T0, T1>& val);

    // 3 tuple
    template<typename T0, typename T1, typename T2> bool IsEmpty(const std::tuple<T0, T1, T2>& val);
    template<typename T0, typename T1, typename T2> void SetEmpty(std::tuple<T0, T1, T2>& val);
    template<typename T0, typename T1, typename T2> string ToString(const std::tuple<T0, T1, T2>& val);

    // 4 tuple
    template<typename T0, typename T1, typename T2, typename T3> bool IsEmpty(const std::tuple<T0, T1, T2, T3>& val);
    template<typename T0, typename T1, typename T2, typename T3> void SetEmpty(std::tuple<T0, T1, T2, T3>& val);
    template<typename T0, typename T1, typename T2, typename T3> string ToString(const std::tuple<T0, T1, T2, T3>& val);

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T> bool IsEmpty(const T& val);
    template<typename T> void SetEmpty(T& val);
    template<typename T> string ToString(const T& val);

    //optional
    template<typename T> bool IsEmpty(const std::optional<T> val) { return val == std::nullopt; }
    template<typename T> void SetEmpty(std::optional<T>& val) { val = std::nullopt; }
    template<typename T> string ToString(const std::optional<T>& val) { return val ? ToString(val.value()) : ""; }

    // string
    template<typename C> bool IsEmpty(const basic_string<C> &val) {
        return val.empty();
    }
    template<typename C> void SetEmpty(basic_string<C> &val) {
        val.clear();
    }
    template<typename C> string ToString(const basic_string<C> &val) {
        return val;
    }

    // vector
    template<typename T, typename A> bool IsEmpty(const vector<T, A>& val) {
        return val.empty();
    }
    template<typename T, typename A> void SetEmpty(vector<T, A>& val) {
        val.clear();
    }
    template<typename T, typename A> string ToString(const vector<T, A>& val) {
        string ret;
        for (const auto &item : val) {
            if (!ret.empty()) ret += ",";
            ret += "{" + ToString(item) + "}";
        }
        return "[" + ret + "]";
    }

    // set
    template<typename K, typename Pred, typename A> bool IsEmpty(const set<K, Pred, A>& val) {
        return val.empty();
    }
    template<typename K, typename Pred, typename A> void SetEmpty(set<K, Pred, A>& val) {
        val.clear();
    }
    template<typename K, typename Pred, typename A> string ToString(const set<K, Pred, A>& val) {
        string ret;
        for (const auto &item : val) {
            if (!ret.empty()) ret += ",";
            ret += "{" + ToString(item) + "}";
        }
        return "[" + ret + "]";
    }

    // map
    template<typename K, typename T, typename Pred, typename A> bool IsEmpty(const map<K, T, Pred, A>& val) {
        return val.empty();
    }
    template<typename K, typename T, typename Pred, typename A> void SetEmpty(map<K, T, Pred, A>& val) {
        val.clear();
    }
    template<typename K, typename T, typename Pred, typename A> string ToString(const map<K, T, Pred, A>& val) {
        string ret;
        for (const auto &item : val) {
            if (!ret.empty()) ret += ",";
            ret += "{" + ToString(item.first) + "=" + ToString(item.second) + "}";
        }
        return "[" + ret + "]";
    }

    // 2 pair
    template<typename K, typename T> bool IsEmpty(const std::pair<K, T>& val) {
        return IsEmpty(val.first) && IsEmpty(val.second);
    }
    template<typename K, typename T> void SetEmpty(std::pair<K, T>& val) {
        SetEmpty(val.first);
        SetEmpty(val.second);
    }
    template<typename K, typename T> string ToString(const std::pair<K, T>& val) {
        string ret = "{first=" + ToString(val.first) + "}";
        ret += "{second=" + ToString(val.second) + "}";
        return "{" + ret + "}";
    }


    // 2 tuple
    template<typename T0, typename T1> bool IsEmpty(const std::tuple<T0, T1>& val) {
        return IsEmpty(std::get<0>(val)) &&
               IsEmpty(std::get<1>(val));
    }
    template<typename T0, typename T1> void SetEmpty(std::tuple<T0, T1>& val) {
        SetEmpty(std::get<0>(val));
        SetEmpty(std::get<1>(val));

    }
    template<typename T0, typename T1> string ToString(const std::tuple<T0, T1>& val) {
        string ret = "{0=" + ToString(std::get<0>(val)) + "}";
        ret += "{1=" + ToString(std::get<1>(val)) + "}";
        return "{" + ret + "}";
    }

    // 3 tuple
    template<typename T0, typename T1, typename T2> bool IsEmpty(const std::tuple<T0, T1, T2>& val) {
        return IsEmpty(std::get<0>(val)) &&
               IsEmpty(std::get<1>(val)) &&
               IsEmpty(std::get<2>(val));
    }
    template<typename T0, typename T1, typename T2> void SetEmpty(std::tuple<T0, T1, T2>& val) {
        SetEmpty(std::get<0>(val));
        SetEmpty(std::get<1>(val));
        SetEmpty(std::get<2>(val));
    }
    template<typename T0, typename T1, typename T2> string ToString(const std::tuple<T0, T1, T2>& val) {
        string ret = "{0=" + ToString(std::get<0>(val)) + "}";
        ret += "{1=" + ToString(std::get<1>(val)) + "}";
        ret += "{2=" + ToString(std::get<2>(val)) + "}";
        return "{" + ret + "}";
    }

    // 4 tuple
    template<typename T0, typename T1, typename T2, typename T3>
    bool IsEmpty(const std::tuple<T0, T1, T2, T3>& val) {
        return IsEmpty(std::get<0>(val)) &&
               IsEmpty(std::get<1>(val)) &&
               IsEmpty(std::get<2>(val)) &&
               IsEmpty(std::get<3>(val));
    }
    template<typename T0, typename T1, typename T2, typename T3>
    void SetEmpty(std::tuple<T0, T1, T2, T3>& val) {
        SetEmpty(std::get<0>(val));
        SetEmpty(std::get<1>(val));
        SetEmpty(std::get<2>(val));
        SetEmpty(std::get<3>(val));
    }
    template<typename T0, typename T1, typename T2, typename T3>
    string ToString(const std::tuple<T0, T1, T2, T3>& val) {
        string ret = "{0=" + ToString(std::get<0>(val)) + "}";
        ret += "{1=" + ToString(std::get<1>(val)) + "}";
        ret += "{2=" + ToString(std::get<2>(val)) + "}";
        ret += "{3=" + ToString(std::get<3>(val)) + "}";
        return "{" + ret + "}";
    }

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T> bool IsEmpty(const T& val) {
        return val.IsEmpty();
    }
    template<typename T> void SetEmpty(T& val) {
        val.SetEmpty();
    }
    template<typename T> string ToString(const T& val) {
        return val.ToString();
    }

    template <typename ValueType>
    std::shared_ptr<ValueType> MakeEmptyValue() {
        auto value = std::make_shared<ValueType>();
        SetEmpty(*value);
        return value;
    }

    template<typename T>
    T MakeEmpty() {
        T value; SetEmpty(value);
        return value;
    }

    template<typename V>
    inline string to_kv_string(const string &k, const V &v) {
        return k + "=" + db_util::ToString(v) + ",";
    }
    template<typename V>
    inline string to_kv_string_end(const string &k, const V &v) {
        return k + "=" + db_util::ToString(v);
    }

    #define TO_KV_STRING1(v) db_util::to_kv_string(#v, v)
    #define TO_KV_STRING2(k, v) db_util::to_kv_string(k, v)
    #define TO_KV_STRING_END1(v) db_util::to_kv_string(#v, v)
    #define TO_KV_STRING_END2(k, v) db_util::to_kv_string(k, v)
};

class CDBAccess {
public:
    CDBAccess(DBNameType dbNameTypeIn, const boost::filesystem::path &path, size_t cacheSize,
              bool memory, bool wipe)
        : dbNameType(dbNameTypeIn), db(path, cacheSize, memory, wipe) {}

    int64_t GetDbCount() const { return db.GetDbCount(); }
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

    template<typename KeyType, typename ValueType>
    bool HasData(const dbk::PrefixType prefixType, const KeyType &key) const {
        string keyStr = dbk::GenDbKey(prefixType, key);
        return db.Exists(keyStr);
    }

    inline void WriteBatch(CLevelDBBatch &batch) {
        db.WriteBatch(batch, true);
    }

    template<typename ValueType>
    void WriteBatch(const dbk::PrefixType prefixType, ValueType &value) {
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

    std::shared_ptr<leveldb::Iterator> NewIterator() {
        return std::shared_ptr<leveldb::Iterator>(db.NewIterator());
    }
private:
    DBNameType dbNameType;
    mutable CLevelDBWrapper db; // // TODO: remove the mutable declare
};

#endif  // PERSIST_DB_ACCESS_H
