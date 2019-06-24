// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contractdb.h"

#include "accounts/account.h"
#include "accounts/id.h"
#include "accounts/key.h"
#include "commons/uint256.h"
#include "util.h"
#include "vm/vmrunenv.h"

#include <stdint.h>

using namespace std;

/*
bool CContractDB::BatchWrite(const map<string, string > &mapContractDb) {
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

bool CContractDB::EraseKey(const string &vKey) {
    return db.Erase(vKey);
}

bool CContractDB::HaveData(const string &vKey) {
    return db.Exists(vKey);
}

bool CContractDB::GetScript(const int nIndex, string &vScriptId, string &vValue) {
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
        string vKey(ssKeySet.begin(), ssKeySet.end());
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

bool CContractDB::GetContractData(const int curBlockHeight, const string &vScriptId, const int &nIndex,
                                string &vScriptKey, string &vScriptData) {
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
        string vKey(ssKeySet.begin(), ssKeySet.end());
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

bool CContractDB::GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<string, string > &mapTxHash) {
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
                string vValue;
                string vKey;
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

bool CContractDB::GetAllContractAcc(const CRegID &scriptId, map<string, string > &mapAcc) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);

    string strPrefixTemp("acct");
    ssKeySet.insert(ssKeySet.end(), &strPrefixTemp[0], &strPrefixTemp[4]);
    string vRegId = scriptId.GetRegIdRaw();
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
                string vValue;
                string vKey;
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

Object CContractDB::ToJsonObj(string Prefix) {
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
                    obj.push_back(Pair("contract_id", HexStr(ssKey)));
                    obj.push_back(Pair("value", HexStr(ssValue)));
                } else if (Prefix == "data") {
                    obj.push_back(Pair("key", HexStr(ssKey)));
                    obj.push_back(Pair("value", HexStr(ssValue)));
                } else if (Prefix == "acct") {
                    obj.push_back(Pair("acct_key", HexStr(ssKey)));
                    obj.push_back(Pair("acct_value", HexStr(ssValue)));
                }

            } else {
                obj.push_back(Pair("unkown_key", HexStr(ssKey)));
                obj.push_back(Pair("unkown_value", HexStr(ssValue)));
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
    obj.push_back(Pair("contract_db", arrayObj));
    return obj;
}

*/

/* TODO:...
bool CContractCache::SetData(const string &vKey, const string &vValue) {
    mapContractDb[vKey] = vValue;
    return true;
}
*/
/* TODO:...
bool CContractCache::UndoScriptData(const string &vKey, const string &vValue) {

    string vPrefix(vKey.begin(), vKey.begin() + 4);
    string vScriptDataPrefix = {'d', 'a', 't', 'a'};
    if (vPrefix == vScriptDataPrefix) {
        assert(vKey.size() > 10);
        if (vKey.size() < 10) {
            return ERRORMSG("UndoScriptData() : vKey=%s error!\n", HexStr(vKey));
        }
        string vScriptCountKey = {'s', 'd', 'n', 'u', 'm'};
        string vScriptId(vKey.begin() + 4, vKey.begin() + 10);
        string vOldValue;
        if (mapContractDb.count(vKey)) {
            vOldValue = mapContractDb[vKey];
        } else {
            GetData(vKey, vOldValue);
        }
        vScriptCountKey.insert(vScriptCountKey.end(), vScriptId.begin(), vScriptId.end());
        CDataStream ds(SER_DISK, CLIENT_VERSION);

        int nCount(0);
        if (vValue.empty()) {  //key所对应的值由非空设置为空，计数减1
            if (!vOldValue.empty()) {
                if (!GetContractItemCount(vScriptId, nCount))
                    return false;
                --nCount;
                if (!SetContractItemCount(vScriptId, nCount))
                    return false;
            }
        } else {  //key所对应的值由空设置为非空，计数加1
            if (vOldValue.empty()) {
                GetContractItemCount(vScriptId, nCount);
                ++nCount;
                if (!SetContractItemCount(vScriptId, nCount))
                    return false;
            }
        }
    }
    mapContractDb[vKey] = vValue;
    return true;

}
*/

/*TODO:
bool CContractCache::BatchWrite(const map<string, string > &mapData) {
    for (auto &items : mapData) {
        mapContractDb[items.first] = items.second;
    }
    return true;
}
*/

/*TODO:
bool CContractCache::EraseKey(const string &vKey) {
    if (mapContractDb.count(vKey) > 0) {
        mapContractDb[vKey].clear();
    } else {
        string vValue;
        if (pBase->GetData(vKey, vValue)) {
            vValue.clear();
            mapContractDb[vKey] = vValue;
        } else {
            return false;
        }
    }
    return true;
}

bool CContractCache::HaveData(const string &vKey) {
    if (mapContractDb.count(vKey) > 0) {
        if (!mapContractDb[vKey].empty())
            return true;
        else
            return false;
    }
    return pBase->HaveData(vKey);
}
*/

bool CContractCache::GetScript(const int nIndex, string &scriptId, string &value) {
    return false;
/* TODO: ....
    if (0 == nIndex) {
        string scriptKey = {'d', 'e', 'f'};
        string vDataKey;
        string vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        for (auto &item : mapContractDb) {  //遍历本级缓存数据，找出合法的最小的key值
            string vTemp(item.first.begin(), item.first.begin() + 3);
            if (scriptKey == vTemp) {
                if (item.second.empty()) {
                    continue;
                }
                vDataKey   = item.first;
                vDataValue = item.second;
                break;
            }
        }
        if (!pBase->GetScript(nIndex, vScriptId, vValue)) {  //上级没有获取符合条件的key值
            if (vDataKey.empty())
                return false;
            else {  //返回本级缓存的查询结果
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        } else {                     //上级获取到符合条件的key值
            if (vDataKey.empty()) {  //缓存中没有符合条件的key，直接返回上级的查询结果
                return true;
            }
            string dataKeyTemp = {'d', 'e', 'f'};
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptId.begin(), vScriptId.end());  //上级得到的key值
            if (dataKeyTemp < vDataKey) {                                               //若上级查询的key小于本级缓存的key,且此key在缓存中没有，则直接返回数据库中查询的结果
                if (mapContractDb.count(dataKeyTemp) == 0)
                    return true;
                else {
                    mapContractDb[dataKeyTemp].clear();           //在缓存中dataKeyTemp已经被删除过了，重新将此key对应的value清除
                    return GetScript(nIndex, vScriptId, vValue);  //重新从数据库中获取下一条数据
                }
            } else {  //若上级查询的key大于等于本级缓存的key,返回本级的数据
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        }
    } else if (1 == nIndex) {
        string vKey = {'d', 'e', 'f'};
        vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
        string vPreKey(vKey);
        map<string, string >::iterator iterFindKey = mapContractDb.upper_bound(vPreKey);
        string vDataKey;
        string vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        string vKeyTemp = {'d', 'e', 'f'};
        while (iterFindKey != mapContractDb.end()) {
            string vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + 3);
            if (vKeyTemp == vTemp) {
                if (iterFindKey->second.empty()) {
                    ++iterFindKey;
                    continue;
                } else {
                    vDataKey   = iterFindKey->first;
                    vDataValue = iterFindKey->second;
                    break;
                }
            } else {
                ++iterFindKey;
            }
        }
        if (!pBase->GetScript(nIndex, vScriptId, vValue)) {  //从BASE获取指定键值之后的下一个值
            if (vDataKey.empty())
                return false;
            else {
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        } else {
            if (vDataKey.empty())  //缓存中没有符合条件的key，直接返回上级的查询结果
                return true;
            string dataKeyTemp = {'d', 'e', 'f'};
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptId.begin(), vScriptId.end());  //上级得到的key值
            if (dataKeyTemp < vDataKey) {
                if (mapContractDb.count(dataKeyTemp) == 0)
                    return true;
                else {
                    mapContractDb[dataKeyTemp].clear();           //在缓存中dataKeyTemp已经被删除过了，重新将此key对应的value清除
                    return GetScript(nIndex, vScriptId, vValue);  //重新从数据库中获取下一条数据
                }
            } else {  //若上级查询的key大于等于本级缓存的key,返回本级的数据
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        }
    }
    return true;
*/
}

bool CContractCache::SetScript(const string &scriptId, const string &content) {

/*TODO:....
    if (!scriptCache.HaveData(scriptId)) {
        int nCount(0);
        GetScriptCount(nCount);
        ++nCount;
        if (!SetScriptCount(nCount))
            return false;
    }
*/
    return scriptCache.SetData(scriptId, content);
}

bool CContractCache::Flush() {
    return false;
    scriptCache.Flush();
    txOutputCache.Flush();
    acctTxListCache.Flush();
    txDiskPosCache.Flush();
    contractRelatedKidCache.Flush();
}

unsigned int CContractCache::GetCacheSize() {
    return false;
    /* TODO:
    return ::GetSerializeSize(mapContractDb, SER_DISK, CLIENT_VERSION);
    */
}

bool CContractCache::WriteTxOutput(const uint256 &txid, const vector<CVmOperate> &vOutput, CDBOpLogsMap &dbOpLogsMap) {
    return txOutputCache.SetData(txid, vOutput, dbOpLogsMap);
}

bool CContractCache::SetTxHashByAddress(const CKeyID &keyId, uint32_t height, uint32_t index, const uint256 &txid,
                                        CDBOpLogsMap &dbOpLogsMap) {
    auto key = make_tuple(keyId, height, index);
    return acctTxListCache.SetData(key, txid, dbOpLogsMap);
}

bool CContractCache::UndoTxHashByAddress(CDBOpLogsMap &dbOpLogsMap) {
    return acctTxListCache.UndoData(dbOpLogsMap);
}

bool CContractCache::GetTxHashByAddress(const CKeyID &keyId, uint32_t height, map<string, string> &mapTxHash) {
    return false;
    /* TODO: implements get list in cache
        pBase->GetTxHashByAddress(keyId, nHeight, mapTxHash);

        string vPreKey = {'A', 'D', 'D', 'R'};
        CDataStream ds1(SER_DISK, CLIENT_VERSION);
        ds1 << keyId;
        ds1 << nHeight;
        vPreKey.insert(vPreKey.end(), ds1.begin(), ds1.end());

        map<string, string >::iterator iterFindKey =
            mapContractDb.upper_bound(vPreKey);
        while (iterFindKey != mapContractDb.end()) {
            if (0 == memcmp((char *)&iterFindKey->first[0], (char *)&vPreKey[0], 28)) {
                if (iterFindKey->second.empty())
                    mapTxHash.erase(iterFindKey->first);
                else {
                    mapTxHash.insert(make_pair(iterFindKey->first, iterFindKey->second));
                }
            } else {
                break;
            }
        }
        return true;
    */
}

bool CContractCache::GetAllContractAcc(
    const CRegID &scriptId, map<string, string > &mapAcc) {

    return false;
    /* TODO: GetAllContractAcc
    return pBase->GetAllContractAcc(scriptId, mapAcc);
    */
}

bool CContractCache::GetTxOutput(const uint256 &txid, vector<CVmOperate> &vOutput) {

    vector<CVmOperate> value;
    if (!txOutputCache.GetData(txid, value))
        return false;
    return true;
}

bool CContractCache::UndoTxOutput(CDBOpLogsMap &dbOpLogsMap) {
    return txOutputCache.UndoData(dbOpLogsMap);
}

bool CContractCache::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return txDiskPosCache.GetData(txid, pos);
}

bool CContractCache::WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list, CDBOpLogsMap &dbOpLogsMap) {
    for (auto it : list) {
        LogPrint("txindex", "txhash:%s dispos: nFile=%d, nPos=%d nTxOffset=%d\n",
            it.first.GetHex(), it.second.nFile, it.second.nPos, it.second.nTxOffset);

        if (!txDiskPosCache.SetData(it.first, it.second, dbOpLogsMap)) {
            return false;
        }
    }
    return true;
}

bool CContractCache::GetScript(const string &scriptId, string &content) {
    return scriptCache.GetData(scriptId, content);
}

bool CContractCache::GetScript(const CRegID &scriptId, string &vValue) {
    return GetScript(scriptId.ToRawString(), vValue);
}

bool CContractCache::GetContractData(const int nCurBlockHeight, const string &scriptId,
                                         const string &scriptKey, string &scriptData) {
    // TODO: delete the arg nCurBlockHeight??
    return contractDataCache.GetData(make_pair(scriptId, scriptKey), scriptData);
}

bool CContractCache::GetContractData(const int nCurBlockHeight, const string &vScriptId,
                                         const int &nIndex, string &vScriptKey,
                                         string &vScriptData) {
    return false;
/* TODO: ...
    if (0 == nIndex) {
        string vKey = {'d', 'a', 't', 'a'};
        vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
        vKey.push_back('_');
        string vDataKey;
        string vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        map<string, string >::iterator iterFindKey = mapContractDb.upper_bound(vKey);
        while (iterFindKey != mapContractDb.end()) {
            string vKeyTemp(vKey.begin(), vKey.begin() + vScriptId.size() + 5);
            string vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + vScriptId.size() + 5);
            if (vKeyTemp == vTemp) {
                if (iterFindKey->second.empty()) {
                    ++iterFindKey;
                    continue;
                } else {
                    vDataKey   = iterFindKey->first;
                    vDataValue = iterFindKey->second;
                    break;
                }
            } else {
                break;
            }
        }
        bool bUpLevelRet(false);
        int nIndexTemp = nIndex;
        while ((bUpLevelRet = pBase->GetContractData(nCurBlockHeight, vScriptId, nIndexTemp, vScriptKey, vScriptData))) {
            nIndexTemp = 1;
            string dataKeyTemp(vKey.begin(), vKey.end());
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptKey.begin(), vScriptKey.end());
            if (vDataKey.empty()) {  //缓存中没有符合条件的key，直接返回上级的查询结果
                if (!mapContractDb.count(dataKeyTemp)) {
                    return true;
                } else {
                    continue;  //重新从数据库中获取下一条数据
                }
            } else {
                if (dataKeyTemp < vDataKey) {
                    if (!mapContractDb.count(dataKeyTemp)) {
                        return true;
                    } else {
                        continue;  //重新从数据库中获取下一条数据
                    }
                } else {
                    if (vDataValue.empty()) {  //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
                        continue;
                    }
                    vScriptKey.clear();
                    vScriptData.clear();
                    vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                    vScriptData = vDataValue;
                    return true;
                }
            }
        }
        if (!bUpLevelRet) {
            if (vDataKey.empty())
                return false;
            else {
                vScriptKey.clear();
                vScriptData.clear();
                vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                if (vDataValue.empty()) {
                    return false;
                }
                vScriptData = vDataValue;
                return true;
            }
        }
    } else if (1 == nIndex) {
        string vKey = {'d', 'a', 't', 'a'};
        vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
        vKey.push_back('_');
        string vPreKey(vKey);
        vPreKey.insert(vPreKey.end(), vScriptKey.begin(), vScriptKey.end());
        map<string, string >::iterator iterFindKey = mapContractDb.upper_bound(vPreKey);
        string vDataKey;
        string vDataValue;
        vDataValue.clear();
        vDataKey.clear();
        while (iterFindKey != mapContractDb.end()) {
            string vKeyTemp(vKey.begin(), vKey.begin() + vScriptId.size() + 5);
            string vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + vScriptId.size() + 5);
            if (vKeyTemp == vTemp) {
                if (iterFindKey->second.empty()) {
                    ++iterFindKey;
                    continue;
                } else {
                    vDataKey   = iterFindKey->first;
                    vDataValue = iterFindKey->second;
                    break;
                }
            } else {
                break;
            }
        }
        bool bUpLevelRet(false);
        while ((bUpLevelRet = pBase->GetContractData(nCurBlockHeight, vScriptId, nIndex, vScriptKey, vScriptData))) {
            string dataKeyTemp(vKey.begin(), vKey.end());
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptKey.begin(), vScriptKey.end());
            if (vDataKey.empty()) {  //缓存中没有符合条件的key，直接返回上级的查询结果
                if (!mapContractDb.count(dataKeyTemp)) {
                    return true;
                } else {
                    continue;  //重新从数据库中获取下一条数据
                }
            } else {
                if (dataKeyTemp < vDataKey) {
                    if (!mapContractDb.count(dataKeyTemp))
                        return true;
                    else {
                        continue;  //在缓存中dataKeyTemp已经被删除过了，重新从数据库中获取下一条数据
                    }
                } else {
                    if (vDataValue.empty()) {  //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
                        continue;
                    }
                    vScriptKey.clear();
                    vScriptData.clear();
                    vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                    vScriptData = vDataValue;
                    return true;
                }
            }
        }
        if (!bUpLevelRet) {
            if (vDataKey.empty())
                return false;
            else {
                vScriptKey.clear();
                vScriptData.clear();
                vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                if (vDataValue.empty()) {
                    return false;
                }
                vScriptData = vDataValue;
                return true;
            }
        }
    } else {
        return ERRORMSG("GetContractData, nIndex > 1 error");
    }

    return true;
*/
}
bool CContractCache::SetContractData(const string &scriptId, const string &scriptKey,
                                     const string &scriptData, CDBOpLogsMap &dbOpLogsMap) {
    auto key = make_pair(scriptId, scriptKey);
    string oldData;
    if (!contractDataCache.GetData(key, oldData)) {
        if(!IncContractItemCount(scriptId, 1)) {
            return false;
        }
    }

    return contractDataCache.SetData(key, scriptData, dbOpLogsMap);
}

bool CContractCache::HaveScript(const string &scriptId) {
    return scriptCache.HaveData(scriptId);
}


bool CContractCache::GetScriptCount(int &nCount) {

    return false;
    /* TODO: get count by leveldb
    string scriptKey = {'s', 'n', 'u', 'm'};
    string vValue;
    if (!GetData(scriptKey, vValue))
        return false;

    CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
    ds >> nCount;
    return true;
    */

}


/* TODO:...
bool CContractCache::SetScriptCount(const int nCount) {
    return false;
    string scriptKey = {'s', 'n', 'u', 'm'};
    string vValue;
    vValue.clear();
    if (nCount > 0) {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << nCount;
        vValue.insert(vValue.end(), ds.begin(), ds.end());
    }  else if (nCount < 0) {
        return false;
    }
    // If nCount = 0, set an empty value to trigger deleting it in level DB.

    if (!SetData(scriptKey, vValue))
        return false;

    return true;

}
*/

bool CContractCache::EraseScript(const string &scriptId) {

/* TODO: delete
    if (HaveScript(vScriptId)) {
        int nCount(0);
        if (!GetScriptCount(nCount))
            return false;
        if (!SetScriptCount(--nCount))
            return false;
    }
*/
    scriptCache.EraseData(scriptId);
    return true;
}

// TODO: change count from int to int64_t
bool CContractCache::GetContractItemCount(const string &scriptId, int &count) {
    CDBCountValue countValue;
    if (contractItemCountCache.GetData(scriptId, countValue)) {
        count = countValue.value;
        return true;
    }
    return false;
}


bool CContractCache::IncContractItemCount(const string &contractRegId, int count) {

    CDBCountValue countValue;
    contractItemCountCache.GetData(contractRegId, countValue);
    countValue.value += count;
    if (countValue.value < 0) {
        LogPrint("vm","[ERROR]IncContractItemCount failed contractRegId=%s, count=%d", HexStr(contractRegId), countValue.value);
        return false;
    }
    return contractItemCountCache.SetData(contractRegId, countValue);
}

bool CContractCache::SetContractItemCount(const string &vScriptId, int nCount) {
    return false;
    /* TODO:
    if (nCount < 0)
        return false;

    string scriptKey = {'s', 'd', 'n', 'u', 'm'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());

    string vValue;
    vValue.clear();

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << nCount;
    vValue.insert(vValue.end(), ds.begin(), ds.end());

    if (!SetData(scriptKey, vValue))
        return false;

    return true;
    */
}

bool CContractCache::EraseContractData(const string &scriptId,
                                      const string &scriptKey, CDBOpLogsMap &dbOpLogsMap) {

    auto key = make_pair(scriptId, scriptKey);
    string oldData;
    if (contractDataCache.GetData(key, oldData)) {
        IncContractItemCount(scriptId, 1);
        if (!contractDataCache.EraseData(key, dbOpLogsMap))
            return false;
    }
    return true;
}

bool CContractCache::UndoContractData(CDBOpLogsMap &dbOpLogsMap) {
    /* TODO: need to handle UndoContractData()

        string vPrefix(vKey.begin(), vKey.begin() + 4);
        string vScriptDataPrefix = {'d', 'a', 't', 'a'};
        if (vPrefix == vScriptDataPrefix) {
            assert(vKey.size() > 10);
            if (vKey.size() < 10) {
                return ERRORMSG("UndoScriptData() : vKey=%s error!\n", HexStr(vKey));
            }
            string vScriptCountKey = {'s', 'd', 'n', 'u', 'm'};
            string vScriptId(vKey.begin() + 4, vKey.begin() + 10);
            string vOldValue;
            if (mapContractDb.count(vKey)) {
                vOldValue = mapContractDb[vKey];
            } else {
                GetData(vKey, vOldValue);
            }
            vScriptCountKey.insert(vScriptCountKey.end(), vScriptId.begin(), vScriptId.end());
            CDataStream ds(SER_DISK, CLIENT_VERSION);

            int nCount(0);
            if (vValue.empty()) {  //key所对应的值由非空设置为空，计数减1
                if (!vOldValue.empty()) {
                    if (!GetContractItemCount(vScriptId, nCount))
                        return false;
                    --nCount;
                    if (!SetContractItemCount(vScriptId, nCount))
                        return false;
                }
            } else {  //key所对应的值由空设置为非空，计数加1
                if (vOldValue.empty()) {
                    GetContractItemCount(vScriptId, nCount);
                    ++nCount;
                    if (!SetContractItemCount(vScriptId, nCount))
                        return false;
                }
            }
        }
        mapContractDb[vKey] = vValue;
        return true;
    */    
    return contractDataCache.UndoData(dbOpLogsMap);
}

/*TODO: TODO: delete?
bool CContractCache::EraseContractData(const string &vKey) {

    if (vKey.size() < 12) {
        return ERRORMSG("EraseContractData delete script data key value error!");
    }
    string vScriptId(vKey.begin() + 4, vKey.begin() + 10);
    string vScriptKey(vKey.begin() + 11, vKey.end());
    CDbOpLog operLog;
    return EraseContractData(vScriptId, vScriptKey, operLog);

}
*/

/*TODO: delete?
bool CContractCache::HaveScriptData(const string &vScriptId, const string &vScriptKey) {

    string scriptKey = {'d', 'a', 't', 'a'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());
    scriptKey.push_back('_');
    scriptKey.insert(scriptKey.end(), vScriptKey.begin(), vScriptKey.end());
    return HaveData(scriptKey);

}
*/

bool CContractCache::GetScript(const int nIndex, CRegID &scriptId, string &vValue) {
    return false;
    /*
    string tem;
    if (nIndex != 0) {
        tem = scriptId.GetRegIdRaw();
    }
    if (GetScript(nIndex, tem, vValue)) {
        scriptId.SetRegID(tem);
        return true;
    }

    return false;
    */
}

bool CContractCache::SetScript(const CRegID &scriptId, const string &vValue) {
    return SetScript(scriptId.ToRawString(), vValue);
}

bool CContractCache::HaveScript(const CRegID &scriptId) {
    return HaveScript(scriptId.GetRegIdRaw());
}

bool CContractCache::EraseScript(const CRegID &scriptId) {
    return EraseScript(scriptId.GetRegIdRaw());
}

bool CContractCache::GetContractItemCount(const CRegID &scriptId, int &nCount) {
    return GetContractItemCount(scriptId.GetRegIdRaw(), nCount);
}

bool CContractCache::EraseContractData(const CRegID &scriptId, const string &vScriptKey, CDBOpLogsMap &dbOpLogsMap) {
    return EraseContractData(scriptId.GetRegIdRaw(), vScriptKey, dbOpLogsMap);
}

bool CContractCache::HaveScriptData(const CRegID &scriptId, const string &vScriptKey) {
    return HaveScriptData(scriptId.GetRegIdRaw(), vScriptKey);
}

bool CContractCache::GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const string &vScriptKey,
                                         string &vScriptData) {
    return GetContractData(nCurBlockHeight, scriptId.GetRegIdRaw(), vScriptKey, vScriptData);
}

bool CContractCache::GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex,
                                         string &vScriptKey, string &vScriptData) {
    return GetContractData(nCurBlockHeight, scriptId.GetRegIdRaw(), nIndex, vScriptKey, vScriptData);
}

bool CContractCache::SetContractData(const CRegID &scriptId, const string &vScriptKey,
                                         const string &vScriptData, CDBOpLogsMap &dbOpLogsMap) {
    return SetContractData(scriptId.GetRegIdRaw(), vScriptKey, vScriptData, dbOpLogsMap);
}

bool CContractCache::SetTxRelAccout(const uint256 &txHash, const set<CKeyID> &relAccount) {
    return contractRelatedKidCache.SetData(txHash, relAccount);
}
bool CContractCache::GetTxRelAccount(const uint256 &txHash, set<CKeyID> &relAccount) {
    return contractRelatedKidCache.GetData(txHash, relAccount);
}

bool CContractCache::EraseTxRelAccout(const uint256 &txHash) {
    return contractRelatedKidCache.EraseData(txHash);
}

Object CContractCache::ToJsonObj() const {
    return Object();
    /* TODO:
    Object obj;
    Array arrayObj;
    for (auto& item : mapContractDb) {
        Object obj;
        obj.push_back(Pair("key", HexStr(item.first)));
        obj.push_back(Pair("value", HexStr(item.second)));
        arrayObj.push_back(obj);
    }
    // obj.push_back(Pair("mapContractDb", arrayObj));
    arrayObj.push_back(pBase->ToJsonObj("def"));
    arrayObj.push_back(pBase->ToJsonObj("data"));
    arrayObj.push_back(pBase->ToJsonObj("author"));

    obj.push_back(Pair("mapContractDb", arrayObj));
    return obj;
    */
}

string CContractCache::ToString() {
    return "";
    /* TODO:
    string str("");
    string vPrefix = {'d', 'a', 't', 'a'};
    for (auto &item : mapContractDb) {
        string vTemp(item.first.begin(), item.first.begin() + 4);
        if (vTemp == vPrefix) {
            str = strprintf("vKey=%s\n vData=%s\n", HexStr(item.first), HexStr(item.second));
        }
    }
    return str;
    */
}

bool CContractCache::GetScriptAcc(const CRegID &scriptId, const string &accKey,
                                      CAppUserAccount &appAccOut) {
    return contractAccountCache.GetData(make_pair(scriptId.ToRawString(), accKey), appAccOut);
}

bool CContractCache::SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccIn,
                                      CDBOpLogsMap &dbOpLogsMap) {
    if (appAccIn.IsEmpty()) {
        return false;
    }
    auto key = make_pair(scriptId.ToRawString(), appAccIn.GetAccUserId());
    return contractAccountCache.SetData(key, appAccIn, dbOpLogsMap);
}

/* TODO: unused, delete ??
bool CContractCache::EraseScriptAcc(const CRegID &scriptId, const string &vKey) {
    return false;
    string scriptKey = {'a', 'c', 'c', 't'};
    string vRegId    = scriptId.GetRegIdRaw();
    scriptKey.insert(scriptKey.end(), vRegId.begin(), vRegId.end());
    scriptKey.push_back('_');
    scriptKey.insert(scriptKey.end(), vKey.begin(), vKey.end());
    string vValue;

    //LogPrint("vm","%s",HexStr(scriptKey));
    if (!GetData(scriptKey, vValue)) {
        return false;
    }

    return EraseKey(scriptKey);

}
*/

bool CContractCache::UndoScriptAcc(CDBOpLogsMap &dbOpLogsMap) {
    return contractAccountCache.UndoData(dbOpLogsMap);
}
