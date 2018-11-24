// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_KEYSTORE_H
#define COIN_KEYSTORE_H

#include "key.h"
#include "sync.h"
#include "wallet/walletdb.h"
#include <set>
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include <boost/signals2/signal.hpp>

using namespace json_spirit;

class CKeyCombi {
private:
	CPubKey mMainPKey;
	CPubKey mMinerPKey;
	CKey  mMainCkey;
	CKey  mMinerCkey; //only used for miner
	int64_t nCreationTime;

public:
	CKeyCombi();
	CKeyCombi(CKey const &inkey,CKey const &minerKey, int nVersion);
	CKeyCombi(CKey const &inkey, int nVersion);

	string ToString() const;

	Object ToJsonObj()const;
	bool UnSersailFromJson(const Object&);
	int64_t GetBirthDay()const;
	bool GetCKey(CKey& keyOut,bool IsMine = false) const ;
	bool CreateANewKey();
	bool GetPubKey(CPubKey &mOutKey,bool IsMine = false) const;
    bool CleanMainKey();
    bool CleanAll();
	bool IsContainMinerKey()const;
	bool IsContainMainKey()const;
	CKeyID GetCKeyID() const ;
	void SetMainKey(CKey& mainKey);
	void SetMinerKey(CKey & minerKey);

	IMPLEMENT_SERIALIZE
	(
		if(0 == nVersion) {
			READWRITE(mMainPKey);
		}
		READWRITE(mMainCkey);
		if(0 == nVersion) {
			READWRITE(mMinerPKey);
		}
		READWRITE(mMinerCkey);
		READWRITE(nCreationTime);
	)
};

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    // Add a key to the store.
    virtual bool AddKeyCombi(const CKeyID & keyId, const CKeyCombi &keyCombi) = 0;
//    virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) =0;
//    virtual bool AddKey(const CKey &key);

    // Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CKeyID &address) const =0;
    virtual bool GetKey(const CKeyID &address, CKey& keyOut, bool IsMine) const =0;
    virtual void GetKeys(set<CKeyID> &setAddress, bool bFlag) const =0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut, bool IsMine) const;

    // Support for BIP 0013 : see https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki
//    virtual bool AddCScript(const CScript& redeemScript) =0;
//    virtual bool HaveCScript(const CScriptID &hash) const =0;
//    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const =0;
};

typedef map<CKeyID, CKeyCombi> KeyMap;
//typedef map<CScriptID, CScript > ScriptMap;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
  //  ScriptMap mapScripts;

public:
    bool AddKeyCombi(const CKeyID & keyId, const CKeyCombi &keyCombi);
    bool HaveKey(const CKeyID &address) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }
    void GetKeys(set<CKeyID> &setAddress, bool bFlag=false) const
    {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end())
            {
            	if(!bFlag)   //return all address in wallet
            		setAddress.insert((*mi).first);
            	else if(mi->second.IsContainMinerKey() || mi->second.IsContainMainKey())  //only return satisfied mining address
            		setAddress.insert((*mi).first);
                mi++;
            }
        }
    }
    bool GetKey(const CKeyID &address, CKey &keyOut, bool IsMine=false) const
    {
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
            	return mi->second.GetCKey(keyOut, IsMine);
            }
        }
        return false;
    }
    virtual bool GetKeyCombi(const CKeyID & address, CKeyCombi & keyCombiOut) const;
    bool IsContainMainKey() {
    	for(auto &item : mapKeys) {
    		if(item.second.IsContainMainKey())
    			return true;
    	}
    	return false;
    }

//    virtual bool AddCScript(const CScript& redeemScript);
//    virtual bool HaveCScript(const CScriptID &hash) const;
//    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const;
};

typedef vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef map<CKeyID, pair<CPubKey, vector<unsigned char> > > CryptedKeyMap;



#endif
