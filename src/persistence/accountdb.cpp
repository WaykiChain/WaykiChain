// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "accountdb.h"
#include "accounts/key.h"
#include "commons/uint256.h"
#include "util.h"
#include "main.h"

#include <stdint.h>

using namespace std;

extern CChain chainActive;

bool CAccountCache::GetAccount(const CKeyID &keyId, CAccount &account) {
    if (mapKeyId2Account.count(keyId)) {
        if (mapKeyId2Account[keyId].keyID != uint160()) {
            account = mapKeyId2Account[keyId];
            return true;
        } else
            return false;
    }

    if (pBase->GetAccount(keyId, account)) {
        mapKeyId2Account.insert(make_pair(keyId, account));
        // mapKeyId2Account[keyId] = account;
        return true;
    }

    return false;
}

bool CAccountCache::GetAccount(const vector<unsigned char> &accountRegId, CAccount &account) {
    if (accountRegId.empty())
        return false;

    if (mapRegId2KeyId.count(accountRegId)) {
        CKeyID keyId(mapRegId2KeyId[accountRegId]);
        if (keyId != uint160()) {
            if (mapKeyId2Account.count(keyId)) {
                account = mapKeyId2Account[keyId];
                return (account.keyID != uint160()); // return true if the account exists, otherwise return false
            }

            return pBase->GetAccount(keyId, account);  //缓存map中没有，从上级存取
        } else
            return false;  //accountRegId已删除说明账户信息也已删除

    } else {
        CKeyID keyId;
        if (pBase->GetKeyId(accountRegId, keyId)) {
            mapRegId2KeyId[ accountRegId ] = keyId;

            if (mapKeyId2Account.count(keyId) > 0) {
                account = mapKeyId2Account[keyId];
                return (account.keyID != uint160()); // return true if the account exists, otherwise return false
            }

            if (pBase->GetAccount(keyId, account)) {
                mapKeyId2Account[keyId] = account;
                return true;
            }
        }
    }

    return false;
}

bool CAccountCache::SetAccount(const CKeyID &keyId, const CAccount &account) {
    mapKeyId2Account[keyId] = account;
    return true;
}
bool CAccountCache::SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account) {
    if (accountRegId.empty()) {
        return false;
    }
    if (mapRegId2KeyId.count(accountRegId)) {
        CKeyID keyId = mapRegId2KeyId[ accountRegId ];
        mapKeyId2Account[ keyId ] = account;
        return true;
    }
    return false;
}
bool CAccountCache::HaveAccount(const CKeyID &keyId) {
    if (mapKeyId2Account.count(keyId))
        return true;
    else
        return pBase->HaveAccount(keyId);
}
uint256 CAccountCache::GetBestBlock() {
    if (blockHash == uint256())
        return pBase->GetBestBlock();

    return blockHash;
}
bool CAccountCache::SetBestBlock(const uint256 &blockHashIn) {
    blockHash = blockHashIn;
    return true;
}

bool CAccountCache::BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>,
                                CKeyID> &mapKeyIds, const uint256 &blockHashIn) {
    for (map<CKeyID, CAccount>::const_iterator it = mapAccounts.begin(); it != mapAccounts.end(); ++it) {
        if (uint160() == it->second.keyID) {
            pBase->EraseAccountByKeyId(it->first);
            mapKeyId2Account.erase(it->first);
        } else {
            mapKeyId2Account[it->first] = it->second;
        }
    }

    for (map<vector<unsigned char>, CKeyID>::const_iterator itKeyId = mapKeyIds.begin(); itKeyId != mapKeyIds.end(); ++itKeyId)
        mapRegId2KeyId[itKeyId->first] = itKeyId->second;
    blockHash = blockHashIn;
    return true;
}
bool CAccountCache::BatchWrite(const vector<CAccount> &vAccounts) {
    for (vector<CAccount>::const_iterator it = vAccounts.begin(); it != vAccounts.end(); ++it) {
        if (it->IsEmptyValue() && !it->IsRegistered()) {
            mapKeyId2Account[it->keyID]       = *it;
            mapKeyId2Account[it->keyID].keyID = uint160();
        } else {
            mapKeyId2Account[it->keyID] = *it;
        }
    }
    return true;
}
bool CAccountCache::EraseAccountByKeyId(const CKeyID &keyId) {
    if (mapKeyId2Account.count(keyId))
        mapKeyId2Account[keyId].keyID = uint160();
    else {
        CAccount account;
        if (pBase->GetAccount(keyId, account)) {
            account.keyID        = uint160();
            mapKeyId2Account[keyId] = account;
        }
    }
    return true;
}

bool CAccountCache::SetKeyId(const CUserID &userId, const CKeyID &keyId) {
    if (userId.type() == typeid(CRegID))
        return SetKeyId(userId.get<CRegID>().GetRegIdRaw(), keyId);

    return false;
}

bool CAccountCache::SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId) {
    if (accountId.empty())
        return false;
    mapRegId2KeyId[accountId] = keyId;
    return true;
}

bool CAccountCache::GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId) {
    if (accountId.empty())
        return false;

    if (mapRegId2KeyId.count(accountId)) {
        keyId = mapRegId2KeyId[ accountId ];
        return (keyId != uint160());
    }

    if (pBase->GetKeyId(accountId, keyId)) {
        mapRegId2KeyId.insert(make_pair(accountId, keyId));
        //mapRegId2KeyId[accountId] = keyId;
        return true;
    }

    return false;
}

bool CAccountCache::GetKeyId(const CUserID &userId, CKeyID &keyId) {
    if (userId.type() == typeid(CRegID)) {
        return GetKeyId(userId.get<CRegID>().GetRegIdRaw(), keyId);

    } else if (userId.type() == typeid(CPubKey)) {
        keyId = userId.get<CPubKey>().GetKeyId();
        return true;

    } else if (userId.type() == typeid(CKeyID)) {
        keyId = userId.get<CKeyID>();
        return true;

    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("GetKeyId: userId can't be of CNullID type");
    }

    return ERRORMSG("GetKeyId: userid type is unknown");
}

bool CAccountCache::EraseKeyIdByRegId(const vector<unsigned char> &accountRegId) {
    if (accountRegId.empty())
        return false;

    if (mapRegId2KeyId.count(accountRegId))
        mapRegId2KeyId[ accountRegId ] = uint160();
    else {
        CKeyID keyId;
        if (pBase->GetKeyId(accountRegId, keyId)) {
            mapRegId2KeyId[ accountRegId ] = uint160();
        }
    }
    return true;
}


bool CAccountCache::SaveAccount(const CAccount  &account) {
    mapRegId2KeyId[ account.regID.GetRegIdRaw() ] = account.keyID;
    mapKeyId2Account[ account.keyID ] = account;
    if (!account.nickID.IsEmpty()) {
        mapNickId2KeyId[ account.nickID.GetNickIdRaw() ] = account.keyID;
    }

    return true;
}

bool CAccountCache::GetAccount(const CUserID &userId, CAccount &account) {
    bool ret = false;
    if (userId.type() == typeid(CRegID)) {
        ret = GetAccount(userId.get<CRegID>().GetRegIdRaw(), account);

    } else if (userId.type() == typeid(CKeyID)) {
        ret = GetAccount(userId.get<CKeyID>(), account);

    } else if (userId.type() == typeid(CPubKey)) {
        ret = GetAccount(userId.get<CPubKey>().GetKeyId(), account);

    } else if (userId.type() == typeid(CNickID)) {
        ret = GetAccount(userId.get<CNickID>().GetNickIdRaw(), account);

    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("GetAccount: userId can't be of CNullID type");
    }

    return ret;
}

bool CAccountCache::GetUserId(const string &addr, CUserID &userId) {
    CRegID regId(addr);
    if (!regId.IsEmpty()) {
        userId = regId;
        return true;
    }

    CKeyID keyId(addr);
    if (!keyId.IsEmpty()) {
        userId = keyId;
        return true;
    }

    return false;
}

bool CAccountCache::GetRegId(const CKeyID &keyId, CRegID &regId) {
    CAccount acct;
    if (CAccountCache::GetAccount(CUserID(keyId), acct)) {
        regId = acct.regID;
        return true;
    } else {
        return false;
    }
}

bool CAccountCache::GetRegId(const CUserID &userId, CRegID &regId) const {
    CAccountCache tempView(*this);
    CAccount account;
    if (userId.type() == typeid(CRegID)) {
        regId = userId.get<CRegID>();
        return true;
    }
    if (tempView.GetAccount(userId, account)) {
        regId = account.regID;
        return !regId.IsEmpty();
    }
    return false;
}

bool CAccountCache::RegIDIsMature(const CRegID &regId) const {
    if (regId.IsEmpty())
        return false;

    int height = chainActive.Height();
    if ((regId.GetHeight() == 0) || (height - regId.GetHeight() > kRegIdMaturePeriodByBlock))
        return true;
    else
        return false;
}

bool CAccountCache::SetAccount(const CUserID &userId, const CAccount &account) {
    if (userId.type() == typeid(CRegID)) {
        return SetAccount(userId.get<CRegID>().GetRegIdRaw(), account);
    } else if (userId.type() == typeid(CKeyID)) {
        return SetAccount(userId.get<CKeyID>(), account);
    } else if (userId.type() == typeid(CPubKey)) {
        return SetAccount(userId.get<CPubKey>().GetKeyId(), account);
    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return ERRORMSG("SetAccount input userid is unknow type");
}

bool CAccountCache::EraseAccountByKeyId(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return EraseAccountByKeyId(userId.get<CKeyID>());
    } else if (userId.type() == typeid(CPubKey)) {
        return EraseAccountByKeyId(userId.get<CPubKey>().GetKeyId());
    } else {
        return ERRORMSG("EraseAccount account type error!");
    }
    return false;
}

bool CAccountCache::HaveAccount(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return HaveAccount(userId.get<CKeyID>());
    }
    return false;
}

bool CAccountCache::EraseKeyId(const CUserID &userId) {
    if (userId.type() == typeid(CRegID)) {
        return EraseKeyIdByRegId(userId.get<CRegID>().GetRegIdRaw());
    }

    return false;
}

bool CAccountCache::Flush() {
    bool fOk = pBase->BatchWrite(mapKeyId2Account, mapRegId2KeyId, blockHash);
    if (fOk) {
        mapKeyId2Account.clear();
        mapRegId2KeyId.clear();
    }
    return fOk;
}

int64_t CAccountCache::GetFreeBCoins(const CUserID &userId) const {
    CAccountCache tempvew(*this);
    CAccount account;
    if (tempvew.GetAccount(userId, account)) {
        return account.GetFreeBCoins();
    }
    return 0;
}

unsigned int CAccountCache::GetCacheSize() {
    return  ::GetSerializeSize(mapKeyId2Account, SER_DISK, CLIENT_VERSION) +
            ::GetSerializeSize(mapRegId2KeyId, SER_DISK, CLIENT_VERSION);
}

std::tuple<uint64_t, uint64_t> CAccountCache::TraverseAccount() {
    return pBase->TraverseAccount();
}

Object CAccountCache::ToJsonObj() const {
    Object obj;
    obj.push_back(Pair("blockHash", blockHash.ToString()));

    Array arrayObj;
    for (auto& item : mapKeyId2Account) {
        Object obj;
        obj.push_back(Pair("keyID", item.first.ToString()));
        obj.push_back(Pair("account", item.second.ToString()));
        arrayObj.push_back(obj);
    }
    obj.push_back(Pair("mapKeyId2Account", arrayObj));

    for (auto& item : mapRegId2KeyId) {
        Object obj;
        obj.push_back(Pair("accountID", HexStr(item.first)));
        obj.push_back(Pair("keyID", item.second.ToString()));
        arrayObj.push_back(obj);
    }
    obj.push_back(Pair("mapRegId2KeyId", arrayObj));

    return obj;
}

bool CAccountDB::GetAccount(const CKeyID &keyId, CAccount &account) {
    return db.Read(make_pair('k', keyId), account);
}

bool CAccountDB::SetAccount(const CKeyID &keyId, const CAccount &account) {
    bool ret = db.Write(make_pair('k', keyId), account);

    assert(!account.keyID.IsEmpty());
    assert(!account.regID.IsEmpty());
    assert(account.pubKey.IsValid());
    return ret;
}

bool CAccountDB::SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account) {
    CKeyID keyId;
    if (db.Read(make_pair('r', accountRegId), keyId)) {
        return db.Write(make_pair('k', keyId), account);
    } else
        return false;
}

bool CAccountDB::HaveAccount(const CKeyID &keyId) {
    return db.Exists(make_pair('k', keyId));
}

uint256 CAccountDB::GetBestBlock() {
    uint256 hash;
    if (!db.Read('B', hash))
        return uint256();
    return hash;
}

bool CAccountDB::SetBestBlock(const uint256 &hashBlock) {
    return db.Write('B', hashBlock);
}

bool CAccountDB::BatchWrite(const map<CKeyID, CAccount> &mapAccounts,
                                const map<vector<unsigned char>, CKeyID> &mapKeyIds, const uint256 &hashBlock) {
    CLevelDBBatch batch;
    map<CKeyID, CAccount>::const_iterator iterAccount = mapAccounts.begin();
    for (; iterAccount != mapAccounts.end(); ++iterAccount) {
        if (iterAccount->second.keyID.IsNull()) {
            batch.Erase(make_pair('k', iterAccount->first));
        } else {
            batch.Write(make_pair('k', iterAccount->first), iterAccount->second);
        }
    }

    map<vector<unsigned char>, CKeyID>::const_iterator iterKey = mapKeyIds.begin();
    for (; iterKey != mapKeyIds.end(); ++iterKey) {
        if (iterKey->second.IsNull()) {
            batch.Erase(make_pair('r', iterKey->first));
        } else {
            batch.Write(make_pair('r', iterKey->first), iterKey->second);
        }
    }
    if (!hashBlock.IsNull())
        batch.Write('B', hashBlock);

    return db.WriteBatch(batch, true);
}

bool CAccountDB::BatchWrite(const vector<CAccount> &vAccounts) {
    CLevelDBBatch batch;
    vector<CAccount>::const_iterator iterAccount = vAccounts.begin();
    for (; iterAccount != vAccounts.end(); ++iterAccount) {
        batch.Write(make_pair('k', iterAccount->keyID), *iterAccount);
    }
    return db.WriteBatch(batch, false);
}

bool CAccountDB::EraseAccountByKeyId(const CKeyID &keyId) {
    return db.Erase(make_pair('k', keyId));
}

bool CAccountDB::SetKeyId(const vector<unsigned char> &accountRegId, const CKeyID &keyId) {
    return db.Write(make_pair('r', accountRegId), keyId);
}

bool CAccountDB::GetKeyId(const vector<unsigned char> &accountRegId, CKeyID &keyId) {
    return db.Read(make_pair('r', accountRegId), keyId);
}

bool CAccountDB::EraseKeyIdByRegId(const vector<unsigned char> &accountRegId) {
    return db.Erase(make_pair('r', accountRegId));
}

bool CAccountDB::GetAccount(const vector<unsigned char> &accountRegId, CAccount &account) {
    CKeyID keyId;
    if (db.Read(make_pair('r', accountRegId), keyId)) {
        return db.Read(make_pair('k', keyId), account);
    }
    return false;
}

bool CAccountDB::SaveAccount(const CAccount &account) {
    CLevelDBBatch batch;
    batch.Write(make_pair('r', account.regID), account.keyID);
    batch.Write(make_pair('n', account.nickID), account.keyID);
    batch.Write(make_pair('k', account.keyID), account);

    return db.WriteBatch(batch, false);
}

std::tuple<uint64_t, uint64_t> CAccountDB::TraverseAccount() {
    uint64_t totalCoins(0);
    uint64_t totalRegIds(0);

    leveldb::Iterator *pcursor = db.NewIterator();

    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair('k', CKeyID());
    pcursor->Seek(ssKeySet.str());

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType == 'k') {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                CAccount account;
                ssValue >> account;
                totalCoins += account.bcoins;

                CRegID regId;
                if (account.GetRegId(regId)) {
                    // LogPrint("ERROR", "[%d] regId: %s\n", totalRegIds, regId.ToString());
                    totalRegIds++;
                }

                pcursor->Next();
            } else {
                break;  // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            throw runtime_error("CAccountViewDB::TraverseAccount(): Deserialize or I/O error");
        }
    }
    delete pcursor;

    return std::make_tuple(totalCoins, totalRegIds);
}

Object CAccountDB::ToJsonObj(char Prefix) {
    Object obj;
    Array arrayObj;
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    if (Prefix == 'a') {
        vector<unsigned char> account;
        ssKeySet << make_pair('a', account);
    } else {
        CKeyID keyid;
        ssKeySet << make_pair('k', keyid);
    }
    pcursor->Seek(ssKeySet.str());
    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType == Prefix) {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                Object obj;
                if (Prefix == 'a') {
                    obj.push_back(Pair("accountId:", HexStr(ssKey)));
                    obj.push_back(Pair("keyid", HexStr(ssValue)));
                } else if (Prefix == 'k') {
                    obj.push_back(Pair("keyid:", HexStr(ssKey)));
                    CAccount account;
                    ssValue >> account;
                    obj.push_back(Pair("account", account.ToJsonObj()));
                }
                arrayObj.push_back(obj);
                pcursor->Next();
            } else {
                break;  // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            LogPrint("ERROR", "line:%d,%s : Deserialize or I/O error - %s\n", __LINE__, __func__, e.what());
        }
    }
    delete pcursor;
    obj.push_back(Pair("scriptdb", arrayObj));
    return obj;
}