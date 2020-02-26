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
    if (regId2KeyIdCache.GetData(regId, keyId)) {
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
    if (userId.is<CRegID>()) {
        ret = GetAccount(userId.get<CRegID>(), account);

    } else if (userId.is<CKeyID>()) {
        ret = GetAccount(userId.get<CKeyID>(), account);

    } else if (userId.is<CPubKey>()) {
        ret = GetAccount(userId.get<CPubKey>().GetKeyId(), account);

    } else if (userId.is<CNickID>()) {
        ret = GetAccount(userId.get<CNickID>(), account);

    } else if (userId.is<CNullID>()) {
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
    if (regId2KeyIdCache.GetData(regId, keyId)) {
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
    return regId2KeyIdCache.HaveData(regId);
}

bool CAccountDBCache::HaveAccount(const CNickID &nickId) const{
    return nickId2KeyIdCache.HaveData(CVarIntValue<uint64_t >(nickId.value));
}

bool CAccountDBCache::HaveAccount(const CUserID &userId) const {
    if (userId.is<CRegID>()) {
        return HaveAccount(userId.get<CRegID>());

    } else if (userId.is<CKeyID>()) {
        return HaveAccount(userId.get<CKeyID>());

    } else if (userId.is<CPubKey>()) {
        return HaveAccount(userId.get<CPubKey>().GetKeyId());

    } else if (userId.is<CNickID>()) {
        return HaveAccount(userId.get<CNickID>());

    } else if (userId.is<CNullID>()) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return false;
}

bool CAccountDBCache::EraseAccount(const CKeyID &keyId) {
    return accountCache.EraseData(keyId);
}

bool CAccountDBCache::SetKeyId(const CUserID &userId, const CKeyID &keyId) {
    if (userId.is<CRegID>())
        return SetKeyId(userId.get<CRegID>(), keyId);

    return false;
}

bool CAccountDBCache::SetKeyId(const CRegID &regId, const CKeyID &keyId) {
    return regId2KeyIdCache.SetData(regId, keyId);
}

bool CAccountDBCache::GetKeyId(const CRegID &regId, CKeyID &keyId) const {
    return regId2KeyIdCache.GetData(regId, keyId);
}

bool CAccountDBCache::GetKeyId(const CNickID &nickId, CKeyID &keyId) const{
    std::pair<CVarIntValue<uint32_t>, CKeyID> regHeightKeyID ;
    if(nickId.IsEmpty())
        return false;

    bool getResult =  nickId2KeyIdCache.GetData(nickId.value, regHeightKeyID) ;

    if(getResult)
        keyId = regHeightKeyID.second;
    return getResult ;
}

bool CAccountDBCache::GetKeyId(const CUserID &userId, CKeyID &keyId) const {
    if (userId.is<CRegID>()) {
        return GetKeyId(userId.get<CRegID>(), keyId);
    } else if (userId.is<CPubKey>()) {
        keyId = userId.get<CPubKey>().GetKeyId();
        return true;
    } else if (userId.is<CKeyID>()) {
        keyId = userId.get<CKeyID>();
        return true;
    } else if (userId.is<CNullID>()) {
        return ERRORMSG("GetKeyId: userId can't be of CNullID type");
    }

    return ERRORMSG("GetKeyId: userid type is unknown");
}

bool CAccountDBCache::EraseKeyId(const CRegID &regId) {
    return regId2KeyIdCache.EraseData(regId);
}

bool CAccountDBCache::SaveAccount(const CAccount &account) {
    regId2KeyIdCache.SetData(account.regid, account.keyid);
    accountCache.SetData(account.keyid, account);
    return true ;
}

bool CAccountDBCache::SetNickId(const CAccount account, const uint32_t height){
    auto heightKeyId = std::make_pair(CVarIntValue(height),account.keyid);
    nickId2KeyIdCache.SetData(CVarIntValue(account.nickid.value), heightKeyId);
    return true;
}

bool CAccountDBCache::GetNickIdHeight(uint64_t nickIdValue, uint32_t& regHeight) {
    std::pair<CVarIntValue<uint32_t>, CKeyID> regHeightKeyID ;
    bool getResult =  nickId2KeyIdCache.GetData(CVarIntValue(nickIdValue), regHeightKeyID) ;
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
    if (userId.is<CRegID>()) {
        regId = userId.get<CRegID>();

        return true;
    } else if (userId.is<CKeyID>()) {
        CAccount account;
        if (GetAccount(userId.get<CKeyID>(), account)) {
            regId = account.regid;

            return !regId.IsEmpty();
        }
    } else if (userId.is<CPubKey>()) {
        CAccount account;
        if (GetAccount(userId.get<CPubKey>().GetKeyId(), account)) {
            regId = account.regid;

            return !regId.IsEmpty();
        }
    }

    return false;
}

bool CAccountDBCache::SetAccount(const CUserID &userId, const CAccount &account) {
    if (userId.is<CRegID>()) {
        return SetAccount(userId.get<CRegID>(), account);

    } else if (userId.is<CKeyID>()) {
        return SetAccount(userId.get<CKeyID>(), account);

    } else if (userId.is<CPubKey>()) {
        return SetAccount(userId.get<CPubKey>().GetKeyId(), account);

    } else if (userId.is<CNickID>()) {
        return SetAccount(userId.get<CNickID>(), account);

    } else if (userId.is<CNullID>()) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return ERRORMSG("SetAccount input userid is unknow type");
}

bool CAccountDBCache::EraseAccount(const CUserID &userId) {
    if (userId.is<CKeyID>()) {
        return EraseAccount(userId.get<CKeyID>());
    } else if (userId.is<CPubKey>()) {
        return EraseAccount(userId.get<CPubKey>().GetKeyId());
    } else {
        return ERRORMSG("EraseAccount account type error!");
    }
    return false;
}

bool CAccountDBCache::EraseKeyId(const CUserID &userId) {
    if (userId.is<CRegID>()) {
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

Object CAccountDBCache::GetAccountDBStats() {
    uint64_t totalRegIds(0);
    uint64_t totalBCoins(0);
    uint64_t totalSCoins(0);
    uint64_t totalFCoins(0);
    uint64_t bcoinsStates[5] = {0};
    uint64_t scoinsStates[5] = {0};
    uint64_t fcoinsStates[5] = {0};

    map<CKeyID, CAccount> items;
    accountCache.GetAllElements(items);
    for (auto &item : items) {
        totalRegIds++;

        CAccountToken wicc = item.second.GetToken(SYMB::WICC);
        CAccountToken wusd = item.second.GetToken(SYMB::WUSD);
        CAccountToken wgrt = item.second.GetToken(SYMB::WGRT);
        
        bcoinsStates[0] += wicc.free_amount;
        bcoinsStates[1] += wicc.voted_amount;
        bcoinsStates[2] += wicc.frozen_amount;
        bcoinsStates[3] += wicc.staked_amount;
        bcoinsStates[4] += wicc.pledged_amount;
        
        scoinsStates[0] += wusd.free_amount;
        scoinsStates[1] += wusd.voted_amount;
        scoinsStates[2] += wusd.frozen_amount;
        scoinsStates[3] += wusd.staked_amount;
        scoinsStates[4] += wusd.pledged_amount;

        fcoinsStates[0] += wgrt.free_amount;
        fcoinsStates[1] += wgrt.voted_amount;
        fcoinsStates[2] += wgrt.frozen_amount;
        fcoinsStates[3] += wgrt.staked_amount;
        fcoinsStates[4] += wgrt.pledged_amount;

        totalBCoins += wicc.free_amount + wicc.voted_amount + wicc.frozen_amount + wicc.staked_amount + wicc.pledged_amount;
        totalSCoins += wusd.free_amount + wusd.voted_amount + wusd.frozen_amount + wusd.staked_amount + wicc.pledged_amount;
        totalFCoins += wgrt.free_amount + wgrt.voted_amount + wgrt.frozen_amount + wgrt.staked_amount + wicc.pledged_amount;
    }

    Object obj_wicc;
    obj_wicc.push_back(Pair("free_amount",      ValueFromAmount(bcoinsStates[0])));
    obj_wicc.push_back(Pair("voted_amount",     ValueFromAmount(bcoinsStates[1])));
    obj_wicc.push_back(Pair("frozen_amount",    ValueFromAmount(bcoinsStates[2])));
    obj_wicc.push_back(Pair("staked_amount",    ValueFromAmount(bcoinsStates[3])));
    obj_wicc.push_back(Pair("pledged_amount",   ValueFromAmount(bcoinsStates[4])));
    obj_wicc.push_back(Pair("total_amount",     ValueFromAmount(totalBCoins)));

    Object obj_wusd;
    obj_wusd.push_back(Pair("free_amount",      ValueFromAmount(scoinsStates[0])));
    obj_wusd.push_back(Pair("voted_amount",     ValueFromAmount(scoinsStates[1])));
    obj_wusd.push_back(Pair("frozen_amount",    ValueFromAmount(scoinsStates[2])));
    obj_wusd.push_back(Pair("staked_amount",    ValueFromAmount(scoinsStates[3])));
    obj_wusd.push_back(Pair("pledged_amount",   ValueFromAmount(scoinsStates[4])));
    obj_wicc.push_back(Pair("total_amount",     ValueFromAmount(totalSCoins)));

    Object obj_wgrt;
    obj_wgrt.push_back(Pair("free_amount",      ValueFromAmount(fcoinsStates[0])));
    obj_wgrt.push_back(Pair("voted_amount",     ValueFromAmount(fcoinsStates[1])));
    obj_wgrt.push_back(Pair("frozen_amount",    ValueFromAmount(fcoinsStates[2])));
    obj_wgrt.push_back(Pair("staked_amount",    ValueFromAmount(fcoinsStates[3])));
    obj_wgrt.push_back(Pair("pledged_amount",   ValueFromAmount(fcoinsStates[4])));
    obj_wicc.push_back(Pair("total_amount",     ValueFromAmount(totalFCoins)));

    Object obj;
    obj.push_back(Pair("WICC",          obj_wicc));
    obj.push_back(Pair("WUSD",          obj_wusd));
    obj.push_back(Pair("WGRT",          obj_wgrt));
    obj.push_back(Pair("total_regids",  totalRegIds));

    return obj;
    
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