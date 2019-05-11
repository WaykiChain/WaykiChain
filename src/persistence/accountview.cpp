
bool CAccountViewCache::GetAccount(const CKeyID &keyId, CAccount &account) {
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

bool CAccountViewCache::GetAccount(const vector<unsigned char> &accountRegId, CAccount &account) {
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

bool CAccountViewCache::SetAccount(const CKeyID &keyId, const CAccount &account) {
    mapKeyId2Account[keyId] = account;
    return true;
}
bool CAccountViewCache::SetAccount(const vector<unsigned char> &accountRegId, const CAccount &account) {
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
bool CAccountViewCache::HaveAccount(const CKeyID &keyId) {
    if (mapKeyId2Account.count(keyId))
        return true;
    else
        return pBase->HaveAccount(keyId);
}
uint256 CAccountViewCache::GetBestBlock() {
    if (blockHash == uint256())
        return pBase->GetBestBlock();

    return blockHash;
}
bool CAccountViewCache::SetBestBlock(const uint256 &blockHashIn) {
    blockHash = blockHashIn;
    return true;
}

bool CAccountViewCache::BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>,
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
bool CAccountViewCache::BatchWrite(const vector<CAccount> &vAccounts) {
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
bool CAccountViewCache::EraseAccountByKeyId(const CKeyID &keyId) {
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

bool CAccountViewCache::SetKeyId(const CUserID &userId, const CKeyID &keyId) {
    if (userId.type() == typeid(CRegID))
        return SetKeyId(userId.get<CRegID>().GetRegIdRaw(), keyId);

    return false;
}

bool CAccountViewCache::SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId) {
    if (accountId.empty())
        return false;
    mapRegId2KeyId[accountId] = keyId;
    return true;
}

bool CAccountViewCache::GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId) {
    if (accountId.empty())
        return false;

    if (mapRegId2KeyId.count(accountId)) {
        keyId = mapRegId2KeyId[accountId];
        return (keyId != uint160());
    }

    if (pBase->GetKeyId(accountId, keyId)) {
        mapRegId2KeyId.insert(make_pair(accountId, keyId));
        //mapRegId2KeyId[accountId] = keyId;
        return true;
    }

    return false;
}

bool CAccountViewCache::EraseKeyIdByRegId(const vector<unsigned char> &accountRegId) {
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


bool CAccountViewCache::SaveAccountInfo(const CAccount  &account) {

    mapRegId2KeyId[account.regID.GetRegIdRaw()]          = account.keyID;

    if (!account.nickID.IsEmpty())
        mapNickId2KeyId[account.nickID.GetNickIdRaw()]   = account.keyID;

    mapKeyId2Account[account.keyID]                            = account;

    return true;
}

bool CAccountViewCache::GetAccount(const CUserID &userId, CAccount &account) {
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

bool CAccountViewCache::GetKeyId(const CUserID &userId, CKeyID &keyId) {
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

bool CAccountViewCache::GetUserId(const string &addr, CUserID &userId) {
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

bool CAccountViewCache::GetRegId(const CKeyID &keyId, CRegID &regId) {
    CAccount acct;
    if (CAccountViewCache::GetAccount(CUserID(keyId), acct)) {
        regId = acct.regID;
        return true;
    } else {
        return false;
    }
}

bool CAccountViewCache::GetRegId(const CUserID &userId, CRegID &regId) const {
    CAccountViewCache tempView(*this);
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

bool CAccountViewCache::RegIDIsMature(const CRegID &regId) const {
    if (regId.IsEmpty())
        return false;

    int height = chainActive.Height();
    if ((regId.GetHeight() == 0) || (height - regId.GetHeight() > kRegIdMaturePeriodByBlock))
        return true;
    else
        return false;
}

bool CAccountViewCache::SetAccount(const CUserID &userId, const CAccount &account) {
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

bool CAccountViewCache::EraseAccountByKeyId(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return EraseAccountByKeyId(userId.get<CKeyID>());
    } else if (userId.type() == typeid(CPubKey)) {
        return EraseAccountByKeyId(userId.get<CPubKey>().GetKeyId());
    } else {
        return ERRORMSG("EraseAccount account type error!");
    }
    return false;
}

bool CAccountViewCache::HaveAccount(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return HaveAccount(userId.get<CKeyID>());
    }
    return false;
}

bool CAccountViewCache::EraseKeyId(const CUserID &userId) {
    if (userId.type() == typeid(CRegID)) {
        return EraseKeyIdByRegId(userId.get<CRegID>().GetRegIdRaw());
    }

    return false;
}

bool CAccountViewCache::Flush() {
    bool fOk = pBase->BatchWrite(mapKeyId2Account, mapRegId2KeyId, blockHash);
    if (fOk) {
        mapKeyId2Account.clear();
        mapRegId2KeyId.clear();
    }
    return fOk;
}

int64_t CAccountViewCache::GetFreeBCoins(const CUserID &userId) const {
    CAccountViewCache tempvew(*this);
    CAccount account;
    if (tempvew.GetAccount(userId, account)) {
        return account.GetFreeBCoins();
    }
    return 0;
}

unsigned int CAccountViewCache::GetCacheSize() {
    return  ::GetSerializeSize(mapKeyId2Account, SER_DISK, CLIENT_VERSION) +
            ::GetSerializeSize(mapRegId2KeyId, SER_DISK, CLIENT_VERSION);
}

std::tuple<uint64_t, uint64_t> CAccountViewCache::TraverseAccount() {
    return pBase->TraverseAccount();
}

Object CAccountViewCache::ToJsonObj() const {
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

bool CScriptDBViewCache::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    if (mapContractDb.count(vKey) > 0) {
        if (!mapContractDb[vKey].empty()) {
            vValue = mapContractDb[vKey];
            return true;
        } else {
            return false;
        }
    }

    if (!pBase->GetData(vKey, vValue)) {
        return false;
    }
    mapContractDb[vKey] = vValue;  //cache it here for speed in-mem access
    return true;
}