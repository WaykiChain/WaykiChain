// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "accountdb.h"
#include "entities/key.h"
#include "commons/uint256.h"
#include "commons/util/util.h"
#include "main.h"

#include <stdint.h>

using namespace std;

extern CChain chainActive;

bool CAccountDBCache::GetFcoinGenesisAccount(CAccount &fcoinGensisAccount) const {
    return GetAccount(SysCfg().GetFcoinGenesisRegId(), fcoinGensisAccount);
}

bool CAccountDBCache::GetAccount(const CKeyID &keyId, CAccount &account) const {
    return accountCache.GetData(keyId, account);
}

bool CAccountDBCache::GetAccount(const CRegID &regId, CAccount &account) const {
    if (regId.IsEmpty())
        return false;

    CKeyID keyId;
    if (regId2KeyIdCache.GetData(regId.ToRawString(), keyId)) {
        return accountCache.GetData(keyId, account);
    }

    return false;
}

bool CAccountDBCache::GetAccount(const CNickID &nickId,  CAccount &account) const{
    if(nickId.IsEmpty())
        return false ;

    std::pair<CVarIntValue<uint32_t>,CKeyID> regHeightAndKeyId ;
    if(nickId2KeyIdCache.GetData(nickId.value, regHeightAndKeyId)){
        return accountCache.GetData(regHeightAndKeyId.second, account) ;
    }
    return false ;
}

bool CAccountDBCache::GetAccount(const CUserID &userId, CAccount &account) const {
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

bool CAccountDBCache::SetAccount(const CKeyID &keyId, const CAccount &account) {
    accountCache.SetData(keyId, account);
    return true;
}

bool CAccountDBCache::SetAccount(const CRegID &regId, const CAccount &account) {
    CKeyID keyId;
    if (regId2KeyIdCache.GetData(regId.ToRawString(), keyId)) {
        return accountCache.SetData(keyId, account);
    }
    return false;
}
bool CAccountDBCache::SetAccount(const CNickID &nickId,const CAccount &account){

    std::pair<CVarIntValue<uint32_t>, CKeyID> heightKeyID ;
    if(nickId2KeyIdCache.GetData(nickId.value, heightKeyID)){
        return accountCache.SetData(heightKeyID.second, account);
    }
    return false ;
}


bool CAccountDBCache::HaveAccount(const CKeyID &keyId) const {
    return accountCache.HaveData(keyId);
}

bool CAccountDBCache::HaveAccount(const CRegID &regId) const{
    return regId2KeyIdCache.HaveData(regId.ToRawString());
}

bool CAccountDBCache::HaveAccount(const CNickID &nickId) const{
    return nickId2KeyIdCache.HaveData(CVarIntValue<uint64_t >(nickId.value));
}

bool CAccountDBCache::HaveAccount(const CUserID &userId) const {
    if (userId.type() == typeid(CRegID)) {
        return HaveAccount(userId.get<CRegID>());

    } else if (userId.type() == typeid(CKeyID)) {
        return HaveAccount(userId.get<CKeyID>());

    } else if (userId.type() == typeid(CPubKey)) {
        return HaveAccount(userId.get<CPubKey>().GetKeyId());

    } else if (userId.type() == typeid(CNickID)) {
        return HaveAccount(userId.get<CNickID>());

    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return false;
}

bool CAccountDBCache::EraseAccount(const CKeyID &keyId) {
    return accountCache.EraseData(keyId);
}

bool CAccountDBCache::SetKeyId(const CUserID &userId, const CKeyID &keyId) {
    if (userId.type() == typeid(CRegID))
        return SetKeyId(userId.get<CRegID>(), keyId);

    return false;
}

bool CAccountDBCache::SetKeyId(const CRegID &regId, const CKeyID &keyId) {
    return regId2KeyIdCache.SetData(regId.ToRawString(), keyId);
}

bool CAccountDBCache::GetKeyId(const CRegID &regId, CKeyID &keyId) const {
    return regId2KeyIdCache.GetData(regId.ToRawString(), keyId);
}

bool CAccountDBCache::GetKeyId(const CNickID &nickId, CKeyID &keyId) const{
    std::pair<CVarIntValue<uint32_t>, CKeyID> regHeightKeyID ;
    bool getResult =  nickId2KeyIdCache.GetData(nickId.value, regHeightKeyID) ;
    if(getResult)
        keyId = regHeightKeyID.second;
    return getResult ;
}

bool CAccountDBCache::GetKeyId(const CUserID &userId, CKeyID &keyId) const {
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

bool CAccountDBCache::EraseKeyId(const CRegID &regId) {
    return regId2KeyIdCache.EraseData(regId.ToRawString());
}

bool CAccountDBCache::SaveAccount(const CAccount &account) {
    regId2KeyIdCache.SetData(account.regid.ToRawString(), account.keyid);
    accountCache.SetData(account.keyid, account);
    return true ;
}

bool CAccountDBCache::SetNickId(const CAccount account, const uint32_t height){
    auto heightKeyId = std::make_pair(height,account.keyid);
    nickId2KeyIdCache.SetData(account.nickid.value, heightKeyId);
    return true;
}

bool CAccountDBCache::GetNickIdHeight(uint64_t nickIdValue, uint32_t& regHeight) {
    std::pair<CVarIntValue<uint32_t>, CKeyID> regHeightKeyID ;
    bool getResult =  nickId2KeyIdCache.GetData(nickIdValue, regHeightKeyID) ;
    if(getResult)
        regHeight = regHeightKeyID.first.get();
    return getResult;
}

bool CAccountDBCache::GetUserId(const string &addr, CUserID &userId) const {
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

bool CAccountDBCache::GetRegId(const CKeyID &keyId, CRegID &regId) const {
    CAccount acct;
    if (accountCache.GetData(keyId, acct)) {
        regId = acct.regid;
        return true;
    }
    return false;
}

bool CAccountDBCache::GetRegId(const CUserID &userId, CRegID &regId) const {
    if (userId.type() == typeid(CRegID)) {
        regId = userId.get<CRegID>();

        return true;
    } else if (userId.type() == typeid(CKeyID)) {
        CAccount account;
        if (GetAccount(userId.get<CKeyID>(), account)) {
            regId = account.regid;

            return !regId.IsEmpty();
        }
    } else if (userId.type() == typeid(CPubKey)) {
        CAccount account;
        if (GetAccount(userId.get<CPubKey>().GetKeyId(), account)) {
            regId = account.regid;

            return !regId.IsEmpty();
        }
    }

    return false;
}

bool CAccountDBCache::SetAccount(const CUserID &userId, const CAccount &account) {
    if (userId.type() == typeid(CRegID)) {
        return SetAccount(userId.get<CRegID>(), account);

    } else if (userId.type() == typeid(CKeyID)) {
        return SetAccount(userId.get<CKeyID>(), account);

    } else if (userId.type() == typeid(CPubKey)) {
        return SetAccount(userId.get<CPubKey>().GetKeyId(), account);

    } else if (userId.type() == typeid(CNickID)) {
        return SetAccount(userId.get<CNickID>(), account);

    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return ERRORMSG("SetAccount input userid is unknow type");
}

bool CAccountDBCache::EraseAccount(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return EraseAccount(userId.get<CKeyID>());
    } else if (userId.type() == typeid(CPubKey)) {
        return EraseAccount(userId.get<CPubKey>().GetKeyId());
    } else {
        return ERRORMSG("EraseAccount account type error!");
    }
    return false;
}

bool CAccountDBCache::EraseKeyId(const CUserID &userId) {
    if (userId.type() == typeid(CRegID)) {
        return EraseKeyId(userId.get<CRegID>());
    }

    return false;
}

uint64_t CAccountDBCache::GetAccountFreeAmount(const CKeyID &keyId, const TokenSymbol &tokenSymbol) {
    CAccount account;
    GetAccount(keyId, account);

    CAccountToken accountToken = account.GetToken(tokenSymbol);
    return accountToken.free_amount;
}

bool CAccountDBCache::Flush() {
    accountCache.Flush();
    regId2KeyIdCache.Flush();
    nickId2KeyIdCache.Flush();

    return true;
}

uint32_t CAccountDBCache::GetCacheSize() const {
    return accountCache.GetCacheSize() +
        regId2KeyIdCache.GetCacheSize() +
        nickId2KeyIdCache.GetCacheSize();
}

std::tuple< uint64_t /* total regids */, 
            uint64_t /* total bcoins */,
            uint64_t /* total soins */,
            uint64_t /* total foins */> CAccountDBCache::TraverseAccount() {

    map<CKeyID, CAccount> items;

    accountCache.GetAllElements(items);

    uint64_t totalRegIds = 0;
    uint64_t totalBCoins = 0;
    uint64_t totalSCoins = 0;
    uint64_t totalFCoins = 0;

    for (auto &item : items) {
        totalRegIds++;
        totalBCoins += item.second.GetToken(SYMB::WICC).free_amount;
        totalSCoins += item.second.GetToken(SYMB::WUSD).free_amount;
        totalFCoins += item.second.GetToken(SYMB::WGRT).free_amount;
    }
    
    return std::tie(totalRegIds, totalBCoins, totalSCoins, totalFCoins);
}

Object CAccountDBCache::ToJsonObj(dbk::PrefixType prefix) {
    return Object();
/* TODO: CCompositeKVCache::ToJsonObj()
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