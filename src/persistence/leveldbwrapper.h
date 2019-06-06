// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_LEVELDBWRAPPER_H
#define PERSIST_LEVELDBWRAPPER_H

#include "commons/serialize.h"
#include "util.h"
#include "version.h"
#include "dbconf.h"

#include "json/json_spirit_value.h"
#include <boost/filesystem/path.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace json_spirit;

enum DbOpLogType {
    COMMON_OP,
    ADDR_TXHASH,
    TX_FILE_POS,
};

class CDbOpLog {
public:
    dbk::PrefixType prefixType;
    string prefix;
    string key; // TODO: delete
    string keyElement;
    string value; // TODO: private

    CDbOpLog() : key(), value() {}

    CDbOpLog(const string &keyIn, const string& valueIn):
        key(keyIn), value(valueIn) {
    }

    template<typename K, typename V>
    CDbOpLog(dbk::PrefixType prefixTypeIn, const K& keyElementIn, const V& valueIn){
        Set(prefixTypeIn, keyElementIn, valueIn);
    }

    template<typename K, typename V>
    void Set(dbk::PrefixType prefixTypeIn, const K& keyElementIn, const V& valueIn){
        prefixType = prefixTypeIn;
        prefix = dbk::GetKeyPrefix(prefixType);
        keyElement = dbk::GenDbKey(dbk::EMPTY, keyElementIn);
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue << valueIn;
        value = ssValue.str();
    }

    template<typename K, typename V>
    void Get(const K& keyElementOut, const V& valueOut) const {
        prefix = dbk::GetKeyPrefix(prefixType);
        dbk::ParseDbKey(keyElement, dbk::EMPTY, keyElementOut);
        CDataStream ssValue(value, SER_DISK, CLIENT_VERSION);
        ssValue << valueOut;
        value = ssValue.str();
    }

    inline dbk::PrefixType GetPrefixType() const {
        return prefixType;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(prefix);
        if (fRead) {
            //TODO: parse PrefixType
        }
        READWRITE(key);
        READWRITE(value);
    )

    string ToString() const {
        string str;
        str += strprintf("vey: %s, value: %s", HexStr(key), HexStr(value));
        return str;
    }

    friend bool operator<(const CDbOpLog &log1, const CDbOpLog &log2) {
        return log1.key < log2.key;
    }
};

inline std::string GetDbOpLogTypeName(DbOpLogType type){
    switch (type) {
    case COMMON_OP:
        return "COMMON_OP";
    case ADDR_TXHASH:
        return "ADDR_TXHASH";
    case TX_FILE_POS:
        return "TX_FILE_POS";
    }
    return "UNKNOWN";
}

typedef vector<CDbOpLog> CDbOpLogs;

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

    template<typename K, typename V> void Write(const K& key, const V& value) {
    	leveldb::Slice slKey;
    	CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
		if (typeid(key) == typeid(std::vector<unsigned char>)) {
			CDataStream ssKeyTemp(ssKey.begin(), ssKey.end(), SER_DISK, CLIENT_VERSION);
			vector<unsigned char> vKey;
			ssKeyTemp >> vKey;
 			int iStartPos = 0;
			int nVKeySize = vKey.size();
			iStartPos = GetSizeOfCompactSize(nVKeySize);
			slKey = leveldb::Slice(&ssKey[iStartPos], ssKey.size() - iStartPos);
		} else {
			slKey = leveldb::Slice(&ssKey[0], ssKey.size());
		}
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(ssValue.GetSerializeSize(value));
        ssValue << value;
        leveldb::Slice slValue(&ssValue[0], ssValue.size());
        batch.Put(slKey, slValue);
    }

    void Erase(const std::string &key) {
        batch.Delete(key);
    }

    template<typename K> void Erase(const K& key) {
    	leveldb::Slice slKey;
    	CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
		if (typeid(key) == typeid(std::vector<unsigned char>)) {
			CDataStream ssKeyTemp(ssKey.begin(), ssKey.end(), SER_DISK, CLIENT_VERSION);
			vector<unsigned char> vKey;
			ssKeyTemp >> vKey;
			int iStartPos = 0;
			int nVKeySize = vKey.size();
			iStartPos = GetSizeOfCompactSize(nVKeySize);
			slKey = leveldb::Slice(&ssKey[iStartPos], ssKey.size() - iStartPos);
		} else {
			slKey = leveldb::Slice(&ssKey[0], ssKey.size());
		}

        batch.Delete(slKey);
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

    template<typename K, typename V> bool Read(const K& key, V& value) {
    	leveldb::Slice slKey;
    	CDataStream ssKey(SER_DISK, CLIENT_VERSION);

		ssKey.reserve(ssKey.GetSerializeSize(key));
		ssKey << key;
    	if(typeid(key) == typeid(std::vector<unsigned char>)) {
    		CDataStream ssKeyTemp(ssKey.begin(), ssKey.end(), SER_DISK, CLIENT_VERSION);
    		vector<unsigned char> vKey;
    		ssKeyTemp >> vKey;
    		int iStartPos = 0;
    		int nVKeySize = vKey.size();
			iStartPos = GetSizeOfCompactSize(nVKeySize);
    		slKey = leveldb::Slice(&ssKey[iStartPos], ssKey.size()-iStartPos);
        }else{
        	slKey = leveldb::Slice(&ssKey[0], ssKey.size());
        }
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

    template<typename K, typename V> bool Write(const K& key, const V& value, bool fSync = false) {
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

    template<typename K> bool Exists(const K& key) {
    	leveldb::Slice slKey;
    	CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
		if (typeid(key) == typeid(std::vector<unsigned char>)) {
			CDataStream ssKeyTemp(ssKey.begin(), ssKey.end(), SER_DISK, CLIENT_VERSION);
			vector<unsigned char> vKey;
			ssKeyTemp >> vKey;
			int iStartPos = 0;
			int nVKeySize = vKey.size();
			iStartPos = GetSizeOfCompactSize(nVKeySize);
			slKey = leveldb::Slice(&ssKey[iStartPos], ssKey.size() - iStartPos);
		} else {
			slKey = leveldb::Slice(&ssKey[0], ssKey.size());
		}

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

    template<typename K> bool Erase(const K& key, bool fSync = false) {
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
