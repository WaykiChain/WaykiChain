// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_LEVELDBWRAPPER_H
#define PERSIST_LEVELDBWRAPPER_H

#include "commons/serialize.h"
#include "commons/util.h"
#include "config/version.h"
#include "dbconf.h"

#include "json/json_spirit_value.h"
#include <boost/filesystem/path.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace json_spirit;

class CDbOpLog {
private:
    string key;
    string value;
public:
    CDbOpLog() {}

    // for key-value
    template<typename K, typename V>
    void Set(const K& keyIn, const V& valueIn){

        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey << keyIn;
        key = ssKey.str();

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue << valueIn;
        value = ssValue.str();
    }

    // for single value
    template<typename V>
    void Set(const V& valueIn){
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue << valueIn;
        value = ssValue.str();
    }

    // for key-value
    template<typename K, typename V>
    void Get(K& keyOut, V& valueOut) const {
        CDataStream ssKey(key, SER_DISK, CLIENT_VERSION);
        ssKey >> keyOut;

        CDataStream ssValue(value, SER_DISK, CLIENT_VERSION);
        ssValue >> valueOut;
    }

    // for single value
    template<typename V>
    void Get(V& valueOut) const {
        CDataStream ssValue(value, SER_DISK, CLIENT_VERSION);
        ssValue >> valueOut;
    }

    inline Slice GetValue() { return value; }

    IMPLEMENT_SERIALIZE(
        READWRITE(key);
        READWRITE(value);
    )

    string ToString() const {
        string str;
        str += strprintf("key: %s, value: %s", HexStr(key), HexStr(value));
        return str;
    }

    friend bool operator<(const CDbOpLog &log1, const CDbOpLog &log2) {
        return log1.key < log2.key;
    }
};

typedef vector<CDbOpLog> CDbOpLogs;

class CDBOpLogMap {
public:
    const CDbOpLogs* GetDbOpLogsPtr(dbk::PrefixType prefixType) const {
        assert(prefixType != dbk::EMPTY);
        const string& prefix = dbk::GetKeyPrefix(prefixType);
        auto it = mapDbOpLogs.find(prefix);
        if (it != mapDbOpLogs.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void AddOpLog(dbk::PrefixType prefixType, const CDbOpLog& dbOpLogIn) {
        assert(prefixType != dbk::EMPTY);
        const string& prefix = dbk::GetKeyPrefix(prefixType);
        mapDbOpLogs[prefix].push_back(dbOpLogIn);
    }

    void Clear() { mapDbOpLogs.clear(); }

    std::string ToString() const;
public:
    IMPLEMENT_SERIALIZE(
        READWRITE(mapDbOpLogs);
	)
private:
    mutable map<string, CDbOpLogs> mapDbOpLogs; // dbName -> dbOpLogs
};

class leveldb_error : public runtime_error
{
public:
    leveldb_error(const string &msg) : runtime_error(msg) {}
};

void ThrowError(const leveldb::Status &status);

// Batch of changes queued to be written to a CLevelDBWrapper
class CLevelDBBatch {
    friend class CLevelDBWrapper;

private:
    leveldb::WriteBatch batch;

public:
    template<typename V>
    void Write(const std::string &key, const V& value) {
    	leveldb::Slice slKey(key);
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(ssValue.GetSerializeSize(value));
        ssValue << value;
        leveldb::Slice slValue(&ssValue[0], ssValue.size());
        batch.Put(slKey, slValue);
    }

    void Erase(const std::string &key) {
        batch.Delete(key);
    }

 };

class CLevelDBWrapper {
private:
    // custom environment this database is using (may be NULL in case of default environment)
    leveldb::Env *penv;

    // database options used
    leveldb::Options options;

    // options used when reading from the database
    leveldb::ReadOptions readoptions;

    // options used when iterating over values of the database
    leveldb::ReadOptions iteroptions;

    // options used when writing to the database
    leveldb::WriteOptions writeoptions;

    // options used when sync writing to the database
    leveldb::WriteOptions syncoptions;

    // the database itself
    leveldb::DB *pdb;

public:
    CLevelDBWrapper(const boost::filesystem::path &path, size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    ~CLevelDBWrapper();

    template<typename V>
    bool Read(std::string key, V &value) {
    	leveldb::Slice slKey(key);

        string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrint("INFO","LevelDB read failure: %s\n", status.ToString().c_str());
            ThrowError(status);
        }
        try {
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            return false;
        }
        return true;
    }

    template<typename V>
    bool Write(const std::string &key, const V &value, bool fSync = false) {
        CLevelDBBatch batch;
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
    }

    bool Exists(const std::string &key) {
    	leveldb::Slice slKey(key);
        string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrint("INFO","LevelDB read failure: %s\n", status.ToString().c_str());
            ThrowError(status);
        }
        return true;
    }

    bool Erase(const string& key, bool fSync = false) {
        CLevelDBBatch batch;
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }

    bool WriteBatch(CLevelDBBatch &batch, bool fSync = false);

    // not available for LevelDB; provide for compatibility with BDB
    bool Flush() {
        return true;
    }

    bool Sync() {
        CLevelDBBatch batch;
        return WriteBatch(batch, true);
    }

    // not exactly clean encapsulation, but it's easiest for now
    leveldb::Iterator *NewIterator() {
        return pdb->NewIterator(iteroptions);
    }
    int64_t GetDbCount();
   // Object ToJsonObj();
};

#endif // PERSIST_LEVELDBWRAPPER_H
