// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keystore.h"
#include "wallet/wallet.h"
#include "crypter.h"
#include "key.h"
#include "base58.h"
using namespace json_spirit;

Object CKeyCombi::ToJsonObj()const {
	Object reply;
	if(mMainCkey.IsValid()) {
		reply.push_back(Pair("address",mMainCkey.GetPubKey().GetKeyID().ToAddress()));
		reply.push_back(Pair("mCkey",mMainCkey.ToString()));
		reply.push_back(Pair("mCkeyBase58",CCoinSecret(mMainCkey).ToString()));
		reply.push_back(Pair("mMainPk",mMainCkey.GetPubKey().ToString()));
	}

	if(mMinerCkey.IsValid()){
		reply.push_back(Pair("mMinerCkey",mMinerCkey.ToString()));
		reply.push_back(Pair("mMinerCkeyBase58",CCoinSecret(mMinerCkey).ToString()));
		reply.push_back(Pair("mMinerPk",mMinerCkey.GetPubKey().ToString()));
	}
	reply.push_back(Pair("nCreationTime",nCreationTime));
    return std::move(reply);
}

bool CKeyCombi::UnSersailFromJson(const Object& obj){
	try {
		Object reply;
		const Value& mCKey = find_value(obj, "mCkey");
		if(mCKey.type() != json_spirit::null_type) {
			auto const &tem1 = ::ParseHex(mCKey.get_str());
			mMainCkey.Set(tem1.begin(),tem1.end(),true);
		}
		const Value& mMinerKey = find_value(obj, "mMinerCkey");
		if(mMinerKey.type() != json_spirit::null_type) {
			auto const &tem2=::ParseHex(mMinerKey.get_str());
			mMinerCkey.Set(tem2.begin(),tem2.end(),true);
		}
		const Value& nTime = find_value(obj, "nCreationTime").get_int64();
		if(nTime.type() != json_spirit::null_type) {
			nCreationTime =find_value(obj, "nCreationTime").get_int64();
		}
	} catch (...) {
		ERRORMSG("UnSersailFromJson Failed !");
		return false;
	}

    return true;
}

bool CKeyCombi::CleanAll() {
	mMainCkey.Clear();
	mMinerCkey.Clear();
	mMainPKey = CPubKey();
	mMinerPKey = CPubKey();
	nCreationTime = 0 ;
    return true;
}

bool CKeyCombi::CleanMainKey(){
	return mMainCkey.Clear();
}

CKeyCombi::CKeyCombi(const CKey& inkey, int nVersion) {
	assert(inkey.IsValid());
	CleanAll();
	mMainCkey = inkey ;
	if(FEATURE_BASE == nVersion)
		mMainPKey = mMainCkey.GetPubKey();
	nCreationTime = GetTime();
}

CKeyCombi::CKeyCombi(const CKey& inkey, const CKey& minerKey, int nVersion) {
	assert(inkey.IsValid());
	assert(minerKey.IsValid());
	CleanAll();
	mMinerCkey = minerKey;
	mMainCkey = inkey ;
	if(FEATURE_BASE == nVersion) {
		mMainPKey = mMainCkey.GetPubKey();
		mMinerPKey = mMinerCkey.GetPubKey();
	}
	nCreationTime = GetTime();
}

bool CKeyCombi::GetPubKey(CPubKey& mOutKey, bool IsMine) const {
	if(IsMine == true){
		if(mMinerCkey.IsValid()){
			mOutKey = mMinerCkey.GetPubKey();
			return true;
		}
		return false;
	}
	mOutKey = mMainCkey.GetPubKey();
	return  true;
}

string CKeyCombi::ToString() const{
	string str("");
	if(mMainCkey.IsValid()) {
		str += strprintf(" MainPKey:%s MainKey:%s", mMainCkey.GetPubKey().ToString(), mMainCkey.ToString());
	}
	if(mMinerCkey.IsValid()) {
		str += strprintf(" MinerPKey:%s MinerKey:%s",mMinerCkey.GetPubKey().ToString(), mMinerCkey.ToString());
	}
	 str += strprintf(" CreationTime:%d",  nCreationTime);
	return str;
}

bool CKeyCombi::GetCKey(CKey& keyOut, bool IsMine) const {
	if(IsMine) {
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

CKeyCombi::CKeyCombi() {
	CleanAll();
}

int64_t CKeyCombi::GetBirthDay() const {
	return nCreationTime;
}

CKeyID CKeyCombi::GetCKeyID() const {
	if(mMainCkey.IsValid())
		return mMainCkey.GetPubKey().GetKeyID();
	else {
		CKeyID keyId;
		return keyId;
	}
}
void CKeyCombi::SetMainKey(CKey& mainKey)
{
	mMainCkey = mainKey;
}
void CKeyCombi::SetMinerKey(CKey & minerKey)
{
	mMinerCkey = minerKey;
}
bool CKeyCombi::IsContainMinerKey() const {
	return mMinerCkey.IsValid();
}
bool CKeyCombi::IsContainMainKey() const {
	return mMainCkey.IsValid();
}

bool CKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut, bool IsMine) const
{
    CKey key;
    if (!GetKey(address, key, IsMine))
        return false;
    vchPubKeyOut = key.GetPubKey();
    return true;
}
//
//bool CKeyStore::AddKey(const CKey &key) {
//    return AddKeyPubKey(key, key.GetPubKey());
//}
bool CBasicKeyStore::AddKeyCombi(const CKeyID & keyId, const CKeyCombi &keyCombi)
{
	LOCK(cs_KeyStore);
	mapKeys[keyId] = keyCombi;
	return true;
}

bool CBasicKeyStore::GetKeyCombi(const CKeyID & address, CKeyCombi & keyCombiOut) const
{
  	{
  		LOCK(cs_KeyStore);
  		KeyMap::const_iterator mi = mapKeys.find(address);
  		if(mi != mapKeys.end())
  		{
  			keyCombiOut = mi->second;
  			return true;
  		}
  	}
  	return false;
}

//bool CBasicKeyStore::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
//{
//    LOCK(cs_KeyStore);
//    mapKeys[pubkey.GetKeyID()] = key;
//    return true;
//}

//bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
//{
//    LOCK(cs_KeyStore);
//    mapScripts[redeemScript.GetID()] = redeemScript;
//    return true;
//}
//
//bool CBasicKeyStore::HaveCScript(const CScriptID& hash) const
//{
//    LOCK(cs_KeyStore);
//    return mapScripts.count(hash) > 0;
//}

//bool CBasicKeyStore::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
//{
//    LOCK(cs_KeyStore);
//    ScriptMap::const_iterator mi = mapScripts.find(hash);
//    if (mi != mapScripts.end())
//    {
//        redeemScriptOut = (*mi).second;
//        return true;
//    }
//    return false;
//}

