// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stakedb.h"

#include "accounts/account.h"
#include "util.h"  // log

CStakeDB::CStakeDB(const string &name, size_t nCacheSize, bool fMemory, bool fWipe)
    : db(GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe) {}

CStakeDB::CStakeDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : db(GetDataDir() / "blocks" / "stake", nCacheSize, fMemory, fWipe) {}

bool CStakeDB::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) {
    return db.Write(vKey, vValue);
}

bool CStakeDB::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    return db.Read(vKey, vValue);
}

bool CStakeDB::EraseKey(const vector<unsigned char> &vKey) { return db.Erase(vKey); }

bool CStakeDB::HaveData(const vector<unsigned char> &vKey) { return db.Exists(vKey); }

bool CStakeDB::BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &voteDb) {
    CLevelDBBatch batch;
    for (auto &item : voteDb) {
        if (item.second.empty()) {
            batch.Erase(item.first);
        } else {
            batch.Write(item.first, item.second);
        }
    }
    return db.WriteBatch(batch, true);
}

bool CStakeDB::GetData(const size_t prefixLen, const int &nIndex, vector<unsigned char> &vKey,
                       vector<unsigned char> &vValue) {
    assert(nIndex >= 0 && nIndex <= 1);

    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);

    int i = 0;
    if (1 == nIndex) {
        if (vKey.empty()) {
            return ERRORMSG("GetDelegateVote() : nIndex is 1, and vKey is empty");
        }
        vector<char> vsKey(vKey.begin(), vKey.end());
        ssKeySet.insert(ssKeySet.end(), vsKey.begin(), vsKey.end());
        vector<unsigned char> finalKey(ssKeySet.begin(), ssKeySet.end());
        if (HaveData(finalKey)) {
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
            if (0 == memcmp((char *)&ssKey[0], (char *)&ssKeySet[0], prefixLen)) {
                if (-1 == i) {
                    leveldb::Slice slValue = pcursor->value();
                    CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                    ssValue >> vValue;
                    vKey.clear();
                    vKey.insert(vKey.end(), slKey.data(), slKey.data() + slKey.size());
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
    if (i >= 0) return false;
    return true;
}

bool CStakeDB::GetDelegateVote(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    // prefix: delegate_
    return GetData(9, nIndex, vKey, vValue);
}

bool CStakeDB::GetStakedFcoins(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    // prefix: fcoin_
    return GetData(6, nIndex, vKey, vValue);
}

bool CStakeCache::GetTopDelegates(unordered_set<string> &topDelegateRegIds) {
    //TODO

    return true;
}

bool CStakeCache::SetDelegateVote(const CAccount &delegateAcct, CStakeDBOperLog &operLog) {
    // delegate_{(uint64t)MAX-receivedVotes)}_{delRegID} --> 1

    vector<unsigned char> vKey   = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    uint64_t nMaxNumber          = 0xFFFFFFFFFFFFFFFF;
    uint64_t nReceivedVotes      = delegateAcct.receivedVotes;
    vector<unsigned char> vRegId = delegateAcct.regID.GetRegIdRaw();
    string strVotes              = strprintf("%016x", nMaxNumber - nReceivedVotes);

    vKey.insert(vKey.end(), strVotes.begin(), strVotes.end());
    vKey.push_back('_');
    vKey.insert(vKey.end(), vRegId.begin(), vRegId.end());

    vector<unsigned char> vValue;
    vValue.push_back(1);

    return SetData(vKey, vValue, operLog);
}

bool CStakeCache::SetDelegateVote(const vector<unsigned char> &vKey) {
    if (vKey.empty()) {
        return true;
    }
    vector<unsigned char> vValue;
    vValue.push_back(1);

    return SetData(vKey, vValue);
}

bool CStakeCache::EraseDelegateVote(const CAccountLog &delegateAcct, CStakeDBOperLog &operLog) {
    // delegate_{(uint64t)MAX-receivedVotes)}_{delRegID} --> 1
    vector<unsigned char> vOldKey = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    uint64_t nMaxNumber           = 0xFFFFFFFFFFFFFFFF;
    uint64_t nReceivedVotes       = delegateAcct.receivedVotes;
    vector<unsigned char> vRegId  = delegateAcct.regID.GetRegIdRaw();
    string strOldVoltes           = strprintf("%016x", nMaxNumber - nReceivedVotes);

    vOldKey.insert(vOldKey.end(), strOldVoltes.begin(), strOldVoltes.end());
    vOldKey.push_back('_');
    vOldKey.insert(vOldKey.end(), vRegId.begin(), vRegId.end());

    return EraseData(vOldKey, operLog);
}

bool CStakeCache::EraseDelegateVote(const vector<unsigned char> &vKey) { return EraseKey(vKey); }

bool CStakeCache::GetDelegateVote(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    // prefix: delegate_
    return GetData(9, nIndex, vKey, vValue);
}

bool CStakeCache::SetStakedFcoins(const CAccount &delegateAcct, CStakeDBOperLog &operLog) {
    // fcoin_{(uint64t)MAX-stakedFcoins)}_{delRegID} --> 1
    vector<unsigned char> vKey   = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    uint64_t nMaxNumber          = 0xFFFFFFFFFFFFFFFF;
    uint64_t nStakedFcoins       = delegateAcct.stakedFcoins;
    vector<unsigned char> vRegId = delegateAcct.regID.GetRegIdRaw();
    string strStakedFcoins       = strprintf("%016x", nMaxNumber - nStakedFcoins);

    vKey.insert(vKey.end(), strStakedFcoins.begin(), strStakedFcoins.end());
    vKey.push_back('_');
    vKey.insert(vKey.end(), vRegId.begin(), vRegId.end());

    vector<unsigned char> vValue;
    vValue.push_back(1);

    return SetData(vKey, vValue, operLog);
}

bool CStakeCache::SetStakedFcoins(const vector<unsigned char> &vKey) {
    if (vKey.empty()) {
        return true;
    }
    vector<unsigned char> vValue;
    vValue.push_back(1);

    return SetData(vKey, vValue);
}

bool CStakeCache::EraseStakedFcoins(const CAccountLog &delegateAcct, CStakeDBOperLog &operLog) {
    // fcoin_{(uint64t)MAX-stakedFcoins)}_{delRegID} --> 1
    vector<unsigned char> vOldKey = {'f', 'c', 'o', 'i', 'n', '_'};
    uint64_t nMaxNumber           = 0xFFFFFFFFFFFFFFFF;
    uint64_t nStakedFcoins        = delegateAcct.stakedFcoins;
    vector<unsigned char> vRegId  = delegateAcct.regID.GetRegIdRaw();
    string strOldStakedFcoins     = strprintf("%016x", nMaxNumber - nStakedFcoins);

    vOldKey.insert(vOldKey.end(), strOldStakedFcoins.begin(), strOldStakedFcoins.end());
    vOldKey.push_back('_');
    vOldKey.insert(vOldKey.end(), vRegId.begin(), vRegId.end());

    return EraseData(vOldKey, operLog);
}

bool CStakeCache::EraseStakedFcoins(const vector<unsigned char> &vKey) { return EraseKey(vKey); }

bool CStakeCache::GetStakedFcoins(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    // prefix: fcoin_
    return GetData(6, nIndex, vKey, vValue);
}

bool CStakeCache::GetData(const size_t prefixLen, const int &nIndex, vector<unsigned char> &vKey,
                          vector<unsigned char> &vValue) {
    if (0 == nIndex) {
        vector<unsigned char> vDataKey;
        vector<unsigned char> vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = voteDb.upper_bound(vKey);
        while (iterFindKey != voteDb.end()) {
            vector<unsigned char> vKey1(vKey.begin(), vKey.begin() + prefixLen);
            vector<unsigned char> vKey2(iterFindKey->first.begin(), iterFindKey->first.begin() + prefixLen);
            if (vKey1 == vKey2) {
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
        while ((bUpLevelRet = pBase->GetDelegateVote(nIndexTemp, vKey, vValue))) {
            nIndexTemp = 1;
            vector<unsigned char> vDataKeyTemp(vKey.begin(), vKey.end());
            if (vDataKey.empty()) {
                if (!voteDb.count(vDataKeyTemp)) {
                    return true;
                } else {
                    continue;
                }
            } else {
                if (vDataKeyTemp < vDataKey) {
                    if (!voteDb.count(vDataKeyTemp)) {
                        return true;
                    } else {
                        continue;
                    }
                } else {
                    if (vDataValue.empty()) {
                        continue;
                    }
                    vKey.clear();
                    vValue.clear();
                    vKey.insert(vKey.end(), vDataKey.begin(), vDataKey.end());
                    vValue = vDataValue;
                    return true;
                }
            }
        }
        if (!bUpLevelRet) {
            if (vDataKey.empty())
                return false;
            else {
                vKey.clear();
                vValue.clear();
                vKey.insert(vKey.end(), vDataKey.begin(), vDataKey.end());
                if (vDataValue.empty()) {
                    return false;
                }
                vValue = vDataValue;
                return true;
            }
        }
    } else if (1 == nIndex) {
        map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = voteDb.upper_bound(vKey);
        vector<unsigned char> vDataKey;
        vector<unsigned char> vDataValue;
        vDataValue.clear();
        vDataKey.clear();
        while (iterFindKey != voteDb.end()) {
            vector<unsigned char> vKey1(vKey.begin(), vKey.begin() + prefixLen);
            vector<unsigned char> vKey2(iterFindKey->first.begin(), iterFindKey->first.begin() + prefixLen);
            if (vKey1 == vKey2) {
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
        while ((bUpLevelRet = pBase->GetDelegateVote(nIndex, vKey, vValue))) {
            vector<unsigned char> vDataKeyTemp(vKey.begin(), vKey.end());
            if (vDataKey.empty()) {
                if (!voteDb.count(vDataKeyTemp)) {
                    return true;
                } else {
                    continue;
                }
            } else {
                if (vDataKeyTemp < vDataKey) {
                    if (!voteDb.count(vDataKeyTemp))
                        return true;
                    else {
                        continue;
                    }
                } else {
                    if (vDataValue.empty()) {
                        continue;
                    }
                    vKey.clear();
                    vValue.clear();
                    vKey.insert(vKey.end(), vDataKey.begin(), vDataKey.end());
                    vValue = vDataValue;
                    return true;
                }
            }
        }
        if (!bUpLevelRet) {
            if (vDataKey.empty())
                return false;
            else {
                vKey.clear();
                vValue.clear();
                vKey.insert(vKey.end(), vDataKey.begin(), vDataKey.end());
                if (vDataValue.empty()) {
                    return false;
                }
                vValue = vDataValue;
                return true;
            }
        }
    } else {
        return ERRORMSG("GetDelegateVote, nIndex > 1 error");
    }

    return true;
}

bool CStakeCache::Flush() {
    bool ok = pBase->BatchWrite(voteDb);
    if (ok) {
        voteDb.clear();
    }
    return ok;
}

unsigned int CStakeCache::GetCacheSize() { return ::GetSerializeSize(voteDb, SER_DISK, CLIENT_VERSION); }

bool CStakeCache::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) {
    voteDb[vKey] = vValue;
    return true;
}

bool CStakeCache::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    if (voteDb.count(vKey) > 0) {
        if (!voteDb[vKey].empty()) {
            vValue = voteDb[vKey];
            return true;
        } else {
            return false;
        }
    }

    if (!pBase->GetData(vKey, vValue)) {
        return false;
    }
    voteDb[vKey] = vValue;  // cache it here for speed in-mem access
    return true;
}

bool CStakeCache::EraseKey(const vector<unsigned char> &vKey) {
    if (voteDb.count(vKey) > 0) {
        voteDb[vKey].clear();
    } else {
        vector<unsigned char> vValue;
        if (pBase->GetData(vKey, vValue)) {
            vValue.clear();
            voteDb[vKey] = vValue;
        } else {
            return false;
        }
    }
    return true;
}

bool CStakeCache::HaveData(const vector<unsigned char> &vKey) {
    if (voteDb.count(vKey) > 0) {
        if (!voteDb[vKey].empty())
            return true;
        else
            return false;
    }
    return pBase->HaveData(vKey);
}

bool CStakeCache::BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapData) {
    for (auto &items : mapData) {
        voteDb[items.first] = items.second;
    }
    return true;
}

bool CStakeCache::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue,
                          CStakeDBOperLog &operLog) {
    vector<unsigned char> oldValue;
    oldValue.clear();
    GetData(vKey, oldValue);
    operLog  = CStakeDBOperLog(vKey, oldValue);
    bool ret = SetData(vKey, vValue);
    return ret;
}

bool CStakeCache::EraseData(const vector<unsigned char> &vKey, CStakeDBOperLog &operLog) {
    if (HaveData(vKey)) {
        vector<unsigned char> vValue;
        if (!GetData(vKey, vValue)) return false;

        operLog = CStakeDBOperLog(vKey, vValue);

        if (!EraseKey(vKey)) return false;
    }

    return true;
}