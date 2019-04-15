// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_LEVELDBWRAPPER_H
#define COIN_LEVELDBWRAPPER_H

#include "serialize.h"
#include "util.h"
#include "version.h"

#include <boost/filesystem/path.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include "json/json_spirit_value.h"
using namespace json_spirit;
class leveldb_error : public runtime_error
{
public:
    leveldb_error(const string &msg) : runtime_error(msg) {}
};

void HandleError(const leveldb::Status &status);

// Batch of changes queued to be written to a CLevelDBWrapper
class CLevelDBBatch
{
    friend class CLevelDBWrapper;

private:
    leveldb::WriteBatch batch;

public:
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

class CLevelDBWrapper
{
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
            HandleError(status);
        }
        try {
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch(std::exception &e) {
            return false;
        }
        return true;
    }

    template<typename K, typename V> bool Write(const K& key, const V& value, bool fSync = false) {
        CLevelDBBatch batch;
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
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
            HandleError(status);
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

#endif // COIN_LEVELDBWRAPPER_H
