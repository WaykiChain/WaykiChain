// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "accountdb.h"
#include "accounts/key.h"
#include "commons/uint256.h"
#include "util.h"

#include <stdint.h>

using namespace std;

CAccountViewDB::CAccountViewDB(size_t nCacheSize, bool fMemory, bool fWipe) :
    db( GetDataDir() / "blocks" / "account", nCacheSize, fMemory, fWipe ) {}

CAccountViewDB::CAccountViewDB(const string &name, size_t nCacheSize, bool fMemory, bool fWipe) :
    db( GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe ) {}

bool CAccountViewDB::GetAccount(const CKeyID &keyId, CAccount &account) {
    return db.Read(make_pair('k', keyId), account);
}

bool CAccountViewDB::SetAccount(const CKeyID &keyId, const CAccount &account) {
    bool ret = db.Write(make_pair('k', keyId), account);

    assert(!account.keyID.IsEmpty());
    assert(!account.regID.IsEmpty());
    assert(account.pubKey.IsValid());
    return ret;
}

bool CAccountViewDB::SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account) {
    CKeyID keyId;
    if (db.Read(make_pair('r', accountRegId), keyId)) {
        return db.Write(make_pair('k', keyId), account);
    } else
        return false;
}

bool CAccountViewDB::HaveAccount(const CKeyID &keyId) {
    return db.Exists(make_pair('k', keyId));
}

uint256 CAccountViewDB::GetBestBlock() {
    uint256 hash;
    if (!db.Read('B', hash))
        return uint256();
    return hash;
}

bool CAccountViewDB::SetBestBlock(const uint256 &hashBlock) {
    return db.Write('B', hashBlock);
}

bool CAccountViewDB::BatchWrite(const map<CKeyID, CAccount> &mapAccounts,
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

bool CAccountViewDB::BatchWrite(const vector<CAccount> &vAccounts) {
    CLevelDBBatch batch;
    vector<CAccount>::const_iterator iterAccount = vAccounts.begin();
    for (; iterAccount != vAccounts.end(); ++iterAccount) {
        batch.Write(make_pair('k', iterAccount->keyID), *iterAccount);
    }
    return db.WriteBatch(batch, false);
}

bool CAccountViewDB::EraseAccountByKeyId(const CKeyID &keyId) {
    return db.Erase(make_pair('k', keyId));
}

bool CAccountViewDB::SetKeyId(const vector<unsigned char> &accountRegId, const CKeyID &keyId) {
    return db.Write(make_pair('r', accountRegId), keyId);
}

bool CAccountViewDB::GetKeyId(const vector<unsigned char> &accountRegId, CKeyID &keyId) {
    return db.Read(make_pair('r', accountRegId), keyId);
}

bool CAccountViewDB::EraseKeyIdByRegId(const vector<unsigned char> &accountRegId) {
    return db.Erase(make_pair('r', accountRegId));
}

bool CAccountViewDB::GetAccount(const vector<unsigned char> &accountRegId, CAccount &account) {
    CKeyID keyId;
    if (db.Read(make_pair('r', accountRegId), keyId)) {
        return db.Read(make_pair('k', keyId), account);
    }
    return false;
}

bool CAccountViewDB::SaveAccountInfo(const CAccount &account) {
    CLevelDBBatch batch;
    batch.Write(make_pair('r', account.regID), account.keyID);
    batch.Write(make_pair('n', account.nickID), account.keyID);
    batch.Write(make_pair('k', account.keyID), account);

    return db.WriteBatch(batch, false);
}

std::tuple<uint64_t, uint64_t> CAccountViewDB::TraverseAccount() {
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

Object CAccountViewDB::ToJsonObj(char Prefix) {
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