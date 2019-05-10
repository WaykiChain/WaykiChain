// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contractdb.h"

#include "core.h"
#include "accounts/key.h"
#include "commons/uint256.h"
#include "util.h"

#include <stdint.h>

using namespace std;

CScriptDB::CScriptDB(const string &name, size_t nCacheSize, bool fMemory, bool fWipe) :
    db(GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe) {}

CScriptDB::CScriptDB(size_t nCacheSize, bool fMemory, bool fWipe) :
    db(GetDataDir() / "blocks" / "script", nCacheSize, fMemory, fWipe) {}

bool CScriptDB::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    return db.Read(vKey, vValue);
}

bool CScriptDB::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) {
    return db.Write(vKey, vValue);
}

bool CScriptDB::BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb) {
    CLevelDBBatch batch;
    for (auto &item : mapContractDb) {
        if (item.second.empty()) {
            batch.Erase(item.first);
        } else {
            batch.Write(item.first, item.second);
        }
    }
    return db.WriteBatch(batch, true);
}

bool CScriptDB::EraseKey(const vector<unsigned char> &vKey) {
    return db.Erase(vKey);
}

bool CScriptDB::HaveData(const vector<unsigned char> &vKey) {
    return db.Exists(vKey);
}

bool CScriptDB::GetScript(const int nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue) {
    assert(nIndex >= 0 && nIndex <= 1);
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    string strPrefixTemp("def");
    //ssKeySet.insert(ssKeySet.end(), 9);
    ssKeySet.insert(ssKeySet.end(), &strPrefixTemp[0], &strPrefixTemp[3]);
    int i(0);
    if (1 == nIndex) {
        if (vScriptId.empty()) {
            return ERRORMSG("GetScript() : nIndex is 1, and vScriptId is empty");
        }
        vector<char> vId(vScriptId.begin(), vScriptId.end());
        ssKeySet.insert(ssKeySet.end(), vId.begin(), vId.end());
        vector<unsigned char> vKey(ssKeySet.begin(), ssKeySet.end());
        if (HaveData(vKey)) {  //判断传过来的key,数据库中是否已经存在
            pcursor->Seek(ssKeySet.str());
            i = nIndex;
        } else {
            pcursor->Seek(ssKeySet.str());
        }
    } else {
        pcursor->Seek(ssKeySet.str());
    }
    while (pcursor->Valid() && i-- >= 0) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            //			CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            string strScriptKey(slKey.data(), 0, slKey.size());
            //			ssKey >> strScriptKey;
            string strPrefix = strScriptKey.substr(0, 3);
            if (strPrefix == "def") {
                if (-1 == i) {
                    leveldb::Slice slValue = pcursor->value();
                    CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                    ssValue >> vValue;
                    vScriptId.clear();
                    vScriptId.insert(vScriptId.end(), slKey.data() + 3, slKey.data() + slKey.size());
                }
                pcursor->Next();
            } else {
                delete pcursor;
                return false;
            }
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;
    if (i >= 0)
        return false;
    return true;
}
bool CScriptDB::GetContractData(const int curBlockHeight, const vector<unsigned char> &vScriptId, const int &nIndex,
                                vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) {
    const int iPrefixLen   = 4;
    const int iScriptIdLen = 6;
    const int iSpaceLen    = 1;
    assert(nIndex >= 0 && nIndex <= 1);
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);

    string strPrefixTemp("data");
    ssKeySet.insert(ssKeySet.end(), &strPrefixTemp[0], &strPrefixTemp[4]);
    vector<char> vId(vScriptId.begin(), vScriptId.end());
    ssKeySet.insert(ssKeySet.end(), vId.begin(), vId.end());
    ssKeySet.insert(ssKeySet.end(), '_');
    int i(0);
    if (1 == nIndex) {
        if (vScriptKey.empty()) {
            return ERRORMSG("GetContractData() : nIndex is 1, and vScriptKey is empty");
        }
        vector<char> vsKey(vScriptKey.begin(), vScriptKey.end());
        ssKeySet.insert(ssKeySet.end(), vsKey.begin(), vsKey.end());
        vector<unsigned char> vKey(ssKeySet.begin(), ssKeySet.end());
        if (HaveData(vKey)) {  //判断传过来的key,数据库中是否已经存在
            pcursor->Seek(ssKeySet.str());
            i = nIndex;
        } else {
            pcursor->Seek(ssKeySet.str());
        }
    } else {
        pcursor->Seek(ssKeySet.str());
    }
    while (pcursor->Valid() && i-- >= 0) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            if (0 == memcmp((char *)&ssKey[0], (char *)&ssKeySet[0], 11)) {
                if (-1 == i) {
                    leveldb::Slice slValue = pcursor->value();
                    CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                    ssValue >> vScriptData;
                    vScriptKey.clear();
                    vScriptKey.insert(vScriptKey.end(), slKey.data() + iPrefixLen + iScriptIdLen + iSpaceLen,
                                      slKey.data() + slKey.size());
                }
                pcursor->Next();
            } else {
                delete pcursor;
                return false;
            }
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s\n", __func__, e.what());
        }
    }
    delete pcursor;
    if (i >= 0)
        return false;
    return true;
}

bool CScriptDB::GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);

    string strPrefixTemp("ADDR");
    ssKeySet.insert(ssKeySet.end(), &strPrefixTemp[0], &strPrefixTemp[4]);
    ssKeySet << keyId;
    ssKeySet << nHeight;
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey   = pcursor->key();
            leveldb::Slice slValue = pcursor->value();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            if (0 == memcmp((char *)&ssKey[0], (char *)&ssKeySet[0], 28)) {
                vector<unsigned char> vValue;
                vector<unsigned char> vKey;
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                ssValue >> vValue;
                vKey.insert(vKey.end(), slKey.data(), slKey.data() + slKey.size());
                mapTxHash.insert(make_pair(vKey, vValue));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s\n", __func__, e.what());
        }
    }
    delete pcursor;
    return true;
}

bool CScriptDB::GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);

    string strPrefixTemp("acct");
    ssKeySet.insert(ssKeySet.end(), &strPrefixTemp[0], &strPrefixTemp[4]);
    vector<unsigned char> vRegId = scriptId.GetRegIdRaw();
    vector<char> vId(vRegId.begin(), vRegId.end());
    ssKeySet.insert(ssKeySet.end(), vId.begin(), vId.end());
    ssKeySet.insert(ssKeySet.end(), '_');

    pcursor->Seek(ssKeySet.str());
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey   = pcursor->key();
            leveldb::Slice slValue = pcursor->value();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            if (0 == memcmp((char *)&ssKey[0], (char *)&ssKeySet[0], 11)) {
                vector<unsigned char> vValue;
                vector<unsigned char> vKey;
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                ssValue >> vValue;
                vKey.insert(vKey.end(), slKey.data(), slKey.data() + slKey.size());
                mapAcc.insert(make_pair(vKey, vValue));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s\n", __func__, e.what());
        }
    }
    delete pcursor;
    return true;
}

Object CScriptDB::ToJsonObj(string Prefix) {
    Object obj;
    Array arrayObj;

    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet.insert(ssKeySet.end(), &Prefix[0], &Prefix[Prefix.length()]);
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            string strScriptKey(slKey.data(), 0, slKey.size());
            string strPrefix       = strScriptKey.substr(0, Prefix.length());
            leveldb::Slice slValue = pcursor->value();
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            Object obj;
            if (strPrefix == Prefix) {
                if (Prefix == "def") {
                    obj.push_back(Pair("scriptid", HexStr(ssKey)));
                    obj.push_back(Pair("value", HexStr(ssValue)));
                } else if (Prefix == "data") {
                    obj.push_back(Pair("key", HexStr(ssKey)));
                    obj.push_back(Pair("value", HexStr(ssValue)));
                } else if (Prefix == "acct") {
                    obj.push_back(Pair("acctkey", HexStr(ssKey)));
                    obj.push_back(Pair("acctvalue", HexStr(ssValue)));
                }

            } else {
                obj.push_back(Pair("unkown key", HexStr(ssKey)));
                obj.push_back(Pair("unkown value", HexStr(ssValue)));
            }
            arrayObj.push_back(obj);
            pcursor->Next();
        } catch (std::exception &e) {
            if (pcursor)
                delete pcursor;
            LogPrint("ERROR", "line:%d,%s : Deserialize or I/O error - %s\n", __LINE__, __func__, e.what());
        }
    }
    delete pcursor;
    obj.push_back(Pair("scriptdb", arrayObj));
    return obj;
}
