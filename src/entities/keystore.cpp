// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keystore.h"
#include "commons/base58.h"
#include "entities/key.h"
#include "wallet/crypter.h"
#include "wallet/wallet.h"

using namespace json_spirit;

Object CKeyCombi::ToJsonObj() const {
    Object reply;
    if (mMainCkey.IsValid()) {
        reply.push_back(Pair("address", mMainCkey.GetPubKey().GetKeyId().ToAddress()));
        reply.push_back(Pair("mCkey", mMainCkey.ToString()));
        reply.push_back(Pair("mCkeyBase58", CCoinSecret(mMainCkey).ToString()));
        reply.push_back(Pair("mMainPk", mMainCkey.GetPubKey().ToString()));
    }

    if (mMinerCkey.IsValid()) {
        reply.push_back(Pair("mMinerCkey", mMinerCkey.ToString()));
        reply.push_back(Pair("mMinerCkeyBase58", CCoinSecret(mMinerCkey).ToString()));
        reply.push_back(Pair("mMinerPk", mMinerCkey.GetPubKey().ToString()));
    }
    reply.push_back(Pair("nCreationTime", nCreationTime));

    return reply;
}

bool CKeyCombi::UnSerializeFromJson(const Object& obj) {
    try {
        Object reply;
        const Value& mCKey = find_value(obj, "mCkey");
        if (mCKey.type() != json_spirit::null_type) {
            auto const& tem1 = ::ParseHex(mCKey.get_str());
            mMainCkey.Set(tem1.begin(), tem1.end(), true);
        }
        const Value& mMinerKey = find_value(obj, "mMinerCkey");
        if (mMinerKey.type() != json_spirit::null_type) {
            auto const& tem2 = ::ParseHex(mMinerKey.get_str());
            mMinerCkey.Set(tem2.begin(), tem2.end(), true);
        }
        const Value& nTime = find_value(obj, "nCreationTime").get_int64();
        if (nTime.type() != json_spirit::null_type) {
            nCreationTime = find_value(obj, "nCreationTime").get_int64();
        }
    } catch (...) {
        ERRORMSG("UnSerializeFromJson Failed !");
        return false;
    }

    return true;
}

bool CKeyCombi::CleanAll() {
    mMainCkey.Clear();
    mMainPKey = CPubKey();

    mMinerCkey.Clear();
    mMinerPKey = CPubKey();

    nCreationTime = 0;
    return true;
}

bool CKeyCombi::CleanMainKey() { return mMainCkey.Clear(); }

CKeyCombi::CKeyCombi(const CKey& key, int32_t nVersion) {
    assert(key.IsValid());
    CleanAll();
    mMainCkey = key;
    if (FEATURE_BASE == nVersion)
        mMainPKey = mMainCkey.GetPubKey();
    nCreationTime = GetTime();
}

CKeyCombi::CKeyCombi(const CKey& key, const CKey& minerKey, int32_t nVersion) {
    assert(key.IsValid());
    assert(minerKey.IsValid());
    CleanAll();
    mMinerCkey = minerKey;
    mMainCkey  = key;
    if (FEATURE_BASE == nVersion) {
        mMainPKey  = mMainCkey.GetPubKey();
        mMinerPKey = mMinerCkey.GetPubKey();
    }
    nCreationTime = GetTime();
}

bool CKeyCombi::GetPubKey(CPubKey& mOutKey, bool IsMine) const {
    if (IsMine) {
        if (mMinerCkey.IsValid()) {
            mOutKey = mMinerCkey.GetPubKey();
            return true;
        }
        return false;
    }
    mOutKey = mMainCkey.GetPubKey();
    return true;
}

string CKeyCombi::ToString() const {
    string str("");
    if (mMainCkey.IsValid()) {
        str += strprintf(" MainPKey:%s MainKey:%s", mMainCkey.GetPubKey().ToString(), mMainCkey.ToString());
    }
    if (mMinerCkey.IsValid()) {
        str += strprintf(" MinerPKey:%s MinerKey:%s", mMinerCkey.GetPubKey().ToString(), mMinerCkey.ToString());
    }
    str += strprintf(" CreationTime:%d", nCreationTime);
    return str;
}

bool CKeyCombi::GetCKey(CKey& keyOut, bool isMiner) const {
    if (isMiner) {
        keyOut = mMinerCkey;
    } else {
        keyOut = mMainCkey;
    }
    return keyOut.IsValid();
}

bool CKeyCombi::CreateANewKey() {
    CleanAll();
    mMainCkey.MakeNewKey();
    nCreationTime = GetTime();
    return true;
}

CKeyCombi::CKeyCombi() { CleanAll(); }

int64_t CKeyCombi::GetBirthDay() const { return nCreationTime; }

CKeyID CKeyCombi::GetCKeyID() const {
    if (mMainCkey.IsValid())
        return mMainCkey.GetPubKey().GetKeyId();
    else {
        CKeyID keyId;
        return keyId;
    }
}
void CKeyCombi::SetMainKey(CKey& mainKey) { mMainCkey = mainKey; }
void CKeyCombi::SetMinerKey(CKey& minerKey) { mMinerCkey = minerKey; }
bool CKeyCombi::HaveMinerKey() const { return mMinerCkey.IsValid(); }
bool CKeyCombi::HaveMainKey() const { return mMainCkey.IsValid(); }

bool CKeyStore::GetPubKey(const CKeyID& address, CPubKey& vchPubKeyOut, bool IsMine) const {
    CKey key;
    if (!GetKey(address, key, IsMine))
        return false;

    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool CBasicKeyStore::AddKeyCombi(const CKeyID& keyId, const CKeyCombi& keyCombi) {
    LOCK(cs_KeyStore);
    mapKeys[keyId] = keyCombi;
    return true;
}

bool CBasicKeyStore::GetKeyCombi(const CKeyID& address, CKeyCombi& keyCombiOut) const {
    {
        LOCK(cs_KeyStore);
        KeyMap::const_iterator mi = mapKeys.find(address);
        if (mi != mapKeys.end()) {
            keyCombiOut = mi->second;
            return true;
        }
    }
    return false;
}

bool CBasicKeyStore::AddCScript(const CMulsigScript& script) {
    LOCK(cs_KeyStore);
    CKeyID keyId      = script.GetID();
    mapScripts[keyId] = script;
    return true;
}

bool CBasicKeyStore::HaveCScript(const CKeyID& keyId) const {
    LOCK(cs_KeyStore);
    return mapScripts.count(keyId) > 0;
}

bool CBasicKeyStore::GetCScript(const CKeyID& keyId, CMulsigScript& script) const {
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(keyId);
    if (mi != mapScripts.end()) {
        script = (*mi).second;
        return true;
    }
    return false;
}
