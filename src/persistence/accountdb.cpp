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

bool CAccountCache::GetAccount(const CKeyID &keyId, CAccount &account) const {
    return keyId2AccountCache.GetData(keyId, account);
}

bool CAccountCache::GetAccount(const CRegID &regId, CAccount &account) const {
    if (regId.IsEmpty())
        return false;

    CKeyID keyId;
    if (regId2KeyIdCache.GetData(regId.GetRegIdRawStr(), keyId)) {
        return keyId2AccountCache.GetData(keyId, account);
    }

    return false;
}

bool CAccountCache::GetAccount(const CUserID &userId, CAccount &account) const {
    bool ret = false;
    if (userId.type() == typeid(CRegID)) {
        ret = GetAccount(userId.get<CRegID>(), account);

    } else if (userId.type() == typeid(CKeyID)) {
        ret = GetAccount(userId.get<CKeyID>(), account);

    } else if (userId.type() == typeid(CPubKey)) {
        ret = GetAccount(userId.get<CPubKey>().GetKeyId(), account);

    } else if (userId.type() == typeid(CNickID)) {
        ret = GetAccount(userId.get<CNickID>(), account);

    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("GetAccount: userId can't be of CNullID type");
    }

    return ret;
}

bool CAccountCache::SetAccount(const CKeyID &keyId, const CAccount &account) {
    keyId2AccountCache.SetData(keyId, account);
    return true;
}
bool CAccountCache::SetAccount(const CRegID &regId, const CAccount &account) {
    CKeyID keyId;
    if (regId2KeyIdCache.GetData(regId.GetRegIdRawStr(), keyId)) {
        return keyId2AccountCache.SetData(keyId, account);
    }
    return false;
}

bool CAccountCache::HaveAccount(const CKeyID &keyId) const {
    return keyId2AccountCache.HaveData(keyId);
}

uint256 CAccountCache::GetBestBlock() const {
    uint256 blockHash;
    blockHashCache.GetData(blockHash);
    return blockHash;
}

bool CAccountCache::SetBestBlock(const uint256 &blockHashIn) {
    return blockHashCache.SetData(blockHashIn);
}
/* TODO: check and delete
bool CAccountCache::BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<CRegID,
                                CKeyID> &mapKeyIds, const uint256 &blockHashIn) {
    for (auto it = mapAccounts.begin(); it != mapAccounts.end(); ++it) {
        if (uint160() == it->second.keyID) {
            pBase->EraseAccountByKeyId(it->first);
            mapKeyId2Account.erase(it->first);
        } else {
            mapKeyId2Account[it->first] = it->second;
        }
    }

    for (auto itKeyId = mapKeyIds.begin(); itKeyId != mapKeyIds.end(); ++itKeyId)
        mapRegId2KeyId[itKeyId->first] = itKeyId->second;
    blockHash = blockHashIn;
    return true;
}
bool CAccountCache::BatchWrite(const vector<CAccount> &vAccounts) {
    for (auto it = vAccounts.begin(); it != vAccounts.end(); ++it) {
        if (it->IsEmptyValue() && !it->IsRegistered()) {
            mapKeyId2Account[it->keyID]       = *it;
            mapKeyId2Account[it->keyID].keyID = uint160();
        } else {
            mapKeyId2Account[it->keyID] = *it;
        }
    }
    return true;
}
*/

bool CAccountCache::EraseAccountByKeyId(const CKeyID &keyId) {

    return keyId2AccountCache.EraseData(keyId);
}

bool CAccountCache::SetKeyId(const CUserID &userId, const CKeyID &keyId) {
    if (userId.type() == typeid(CRegID))
        return SetKeyId(userId.get<CRegID>(), keyId);

    return false;
}

bool CAccountCache::SetKeyId(const CRegID &regId, const CKeyID &keyId) {
    return regId2KeyIdCache.SetData(regId.GetRegIdRawStr(), keyId);
}

bool CAccountCache::GetKeyId(const CRegID &regId, CKeyID &keyId) const {
    return regId2KeyIdCache.GetData(regId.GetRegIdRawStr(), keyId);
}

bool CAccountCache::GetKeyId(const CUserID &userId, CKeyID &keyId) const {
    if (userId.type() == typeid(CRegID)) {
        return GetKeyId(userId.get<CRegID>(), keyId);

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

bool CAccountCache::EraseKeyIdByRegId(const CRegID &regId) {
    return regId2KeyIdCache.EraseData(regId.GetRegIdRawStr());
}

bool CAccountCache::SaveAccount(const CAccount &account) {
    regId2KeyIdCache.SetData(account.regID.GetRegIdRawStr(), account.keyID);
    keyId2AccountCache.SetData(account.keyID, account);
    nickId2KeyIdCache.SetData(account.nickID, account.keyID);
/*
    mapRegId2KeyId[ account.regID ] = account.keyID;
    mapKeyId2Account[ account.keyID ] = account;
    if (!account.nickID.IsEmpty()) {
        mapNickId2KeyId[ account.nickID ] = account.keyID;
    }
*/
    return true;
}


bool CAccountCache::GetUserId(const string &addr, CUserID &userId) const {
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

bool CAccountCache::GetRegId(const CKeyID &keyId, CRegID &regId) const {
    CAccount acct;
    if (keyId2AccountCache.GetData(keyId, acct)) {
        regId = acct.regID;
        return true;
    }
    return false;
}

bool CAccountCache::GetRegId(const CUserID &userId, CRegID &regId) const {
    if (userId.type() == typeid(CRegID)) {
        regId = userId.get<CRegID>();
        return true;
    } else if (userId.type() == typeid(CRegID)) {
        CAccount account;
        if (GetAccount(userId.get<CKeyID>(), account)) {
            regId = account.regID;
            return !regId.IsEmpty();
        }
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
        return SetAccount(userId.get<CRegID>(), account);
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

bool CAccountCache::HaveAccount(const CUserID &userId) const {
    if (userId.type() == typeid(CKeyID)) {
        return HaveAccount(userId.get<CKeyID>());
    }
    return false;
}

bool CAccountCache::EraseKeyId(const CUserID &userId) {
    if (userId.type() == typeid(CRegID)) {
        return EraseKeyIdByRegId(userId.get<CRegID>());
    }

    return false;
}

bool CAccountCache::Flush() {
    blockHashCache.Flush();
    keyId2AccountCache.Flush();
    regId2KeyIdCache.Flush();
    nickId2KeyIdCache.Flush();
    return true;
}

int64_t CAccountCache::GetFreeBcoins(const CUserID &userId) const {
    CAccount account;
    if (GetAccount(userId, account)) {
        return account.GetFreeBcoins();
    }
    return 0;
}

unsigned int CAccountCache::GetCacheSize() const {
    /* TODO: CDBMultiValueCache::GetCacheSize()
    return  ::GetSerializeSize(mapKeyId2Account, SER_DISK, CLIENT_VERSION) +
            ::GetSerializeSize(mapRegId2KeyId, SER_DISK, CLIENT_VERSION);
    */
   return 0;
}

std::tuple<uint64_t, uint64_t> CAccountCache::TraverseAccount() {
    // TODO: GetTotalCoins
    //return pBase->TraverseAccount();
    return make_tuple<uint64_t, uint64_t>(0, 0);
}

Object CAccountCache::ToJsonObj(dbk::PrefixType prefix) {
    return Object();
/* TODO: CDBMultiValueCache::ToJsonObj()
    Object obj;
    obj.push_back(Pair("blockHash", blockHash.ToString()));

    Array arrayObj;
    for (auto& item : mapKeyId2Account) {
        Object obj;
        obj.push_back(Pair("keyId", item.first.ToString()));
        obj.push_back(Pair("account", item.second.ToString()));
        arrayObj.push_back(obj);
    }
    obj.push_back(Pair("mapKeyId2Account", arrayObj));

    for (auto& item : mapRegId2KeyId) {
        Object obj;
        obj.push_back(Pair("regId", item.first.ToString()));
        obj.push_back(Pair("keyId", item.second.ToString()));
        arrayObj.push_back(obj);
    }
    obj.push_back(Pair("mapRegId2KeyId", arrayObj));

    return obj;
    */
}

/* TODO: delete CAccountDB
bool CAccountDB::GetAccount(const CKeyID &keyId, CAccount &account) {
    string key = GenDbKey(dbk::KEYID_ACCOUNT, keyId);
    return db.Read(key, account);
}

bool CAccountDB::SetAccount(const CKeyID &keyId, const CAccount &account) {
    string key = GenDbKey(dbk::KEYID_ACCOUNT, keyId);
    bool ret = db.Write(key, account);

    // assert(!account.keyID.IsEmpty());
    // assert(!account.regID.IsEmpty());
    // assert(account.pubKey.IsValid());
    return ret;
}

bool CAccountDB::SetAccount(const CRegID &regId, const CAccount &account) {
    CKeyID keyId;
    string keyIdKey = GenDbKey(dbk::REGID_KEYID, regId.GetRegIdRawStr());
    if (db.Read(keyIdKey, keyId)) {
        string accountKey = GenDbKey(dbk::KEYID_ACCOUNT, keyId);
        return db.Write(accountKey, account);
    }

    return false;
}

bool CAccountDB::HaveAccount(const CKeyID &keyId) {
    string accountKey = GenDbKey(dbk::KEYID_ACCOUNT, keyId);
    return db.Exists(accountKey);
}

uint256 CAccountDB::GetBestBlock() {
    uint256 hash;
    if (!db.Read(dbk::GetKeyPrefix(dbk::BEST_BLOCKHASH), hash))
        return uint256();
    return hash;
}

bool CAccountDB::SetBestBlock(const uint256 &hashBlock) {
    return db.Write(dbk::GetKeyPrefix(dbk::BEST_BLOCKHASH), hashBlock);
}
*/
/* TODO: check and delete...
bool CAccountDB::BatchWrite(const map<CKeyID, CAccount> &mapAccounts,
                                const map<CRegID, CKeyID> &mapKeyIds, const uint256 &hashBlock) {
    CLevelDBBatch batch;
    auto iterAccount = mapAccounts.begin();
    for (; iterAccount != mapAccounts.end(); ++iterAccount) {
        if (iterAccount->second.keyID.IsNull()) {
            batch.Erase(dbk::GenDbKey(dbk::KEYID_ACCOUNT, iterAccount->first));
        } else {
            batch.Write(dbk::GenDbKey(dbk::KEYID_ACCOUNT, iterAccount->first), iterAccount->second);
        }
    }

    auto iterKey = mapKeyIds.begin();
    for (; iterKey != mapKeyIds.end(); ++iterKey) {
        if (iterKey->second.IsNull()) {
            batch.Erase(dbk::GenDbKey(dbk::REGID_KEYID, iterKey->first.GetRegIdRawStr()));
        } else {
            batch.Write(dbk::GenDbKey(dbk::REGID_KEYID, iterKey->first.GetRegIdRawStr()), iterKey->second);
        }
    }
    if (!hashBlock.IsNull())
        batch.Write(dbk::GetKeyPrefix(dbk::BEST_BLOCKHASH), hashBlock);

    return db.WriteBatch(batch, true);
}

bool CAccountDB::BatchWrite(const vector<CAccount> &vAccounts) {
    CLevelDBBatch batch;
    auto iterAccount = vAccounts.begin();
    for (; iterAccount != vAccounts.end(); ++iterAccount) {
        batch.Write(dbk::GenDbKey(dbk::KEYID_ACCOUNT, iterAccount->keyID), *iterAccount);

    }
    return db.WriteBatch(batch, false);
}


bool CAccountDB::EraseAccountByKeyId(const CKeyID &keyId) {
    return db.Erase(dbk::GenDbKey(dbk::KEYID_ACCOUNT, keyId));
}

bool CAccountDB::SetKeyId(const CRegID &regId, const CKeyID &keyId) {
    return db.Write(dbk::GenDbKey(dbk::REGID_KEYID, regId.GetRegIdRawStr()), keyId);
}

bool CAccountDB::GetKeyId(const CRegID &regId, CKeyID &keyId) {
    return db.Read(dbk::GenDbKey(dbk::REGID_KEYID, regId.GetRegIdRawStr()), keyId);
}

bool CAccountDB::EraseKeyIdByRegId(const CRegID &regId) {
    return db.Erase(dbk::GenDbKey(dbk::REGID_KEYID, regId.GetRegIdRawStr()));
}

bool CAccountDB::GetAccount(const CRegID &regId, CAccount &account) {
    CKeyID keyId;
    if (db.Read(dbk::GenDbKey(dbk::REGID_KEYID, regId.GetRegIdRawStr()), keyId)) {
        return db.Read(dbk::GenDbKey(dbk::KEYID_ACCOUNT, keyId), account);
    }
    return false;
}

bool CAccountDB::SaveAccount(const CAccount &account) {
    CLevelDBBatch batch;
    // TODO: should check the regid and nickid is empty?
    batch.Write(dbk::GenDbKey(dbk::REGID_KEYID, account.regID), account.keyID);
    batch.Write(dbk::GenDbKey(dbk::NICKID_KEYID, account.nickID), account.keyID);
    batch.Write(dbk::GenDbKey(dbk::KEYID_ACCOUNT, account.keyID), account);

    return db.WriteBatch(batch, false);
}

std::tuple<uint64_t, uint64_t> CAccountDB::TraverseAccount() {
    uint64_t totalCoins(0);
    uint64_t totalRegIds(0);

    leveldb::Iterator *pcursor = db.NewIterator();

    const std::string& prefix = GetKeyPrefix(dbk::KEYID_ACCOUNT);
    pcursor->Seek(prefix);

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            const leveldb::Slice &slKey = pcursor->key();
            if (slKey.starts_with(prefix)) {
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

Object CAccountDB::ToJsonObj(dbk::PrefixType prefix) {
    Object obj;
    Array arrayObj;
    leveldb::Iterator *pcursor = db.NewIterator();
    const std::string &prefixStr = dbk::GetKeyPrefix(prefix);
    pcursor->Seek(prefixStr);
    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            if (slKey.starts_with(prefixStr)) {
                leveldb::Slice slValue = pcursor->value();
                Object obj;
                if (prefix == dbk::REGID_KEYID) {
                    obj.push_back(Pair("regid:", HexStr(SliceIterator(slKey))));
                    obj.push_back(Pair("keyid", HexStr(SliceIterator(slValue))));
                } else if (prefix == dbk::KEYID_ACCOUNT) {
                    obj.push_back(Pair("keyid:", HexStr(SliceIterator(slKey))));
                    CAccount account;
                    CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
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
*/