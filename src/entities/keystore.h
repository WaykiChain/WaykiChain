// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_KEYSTORE_H
#define ENTITIES_KEYSTORE_H

#include <set>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "entities/key.h"
#include "sync.h"
#include "wallet/walletdb.h"

using namespace json_spirit;

class CKeyCombi {
private:
    CPubKey mMainPKey;
    CKey mMainCkey;  // if existing, used for saving tx fees

    CPubKey mMinerPKey;
    CKey mMinerCkey;  // only used for mining/block-creation

    int64_t nCreationTime;

public:
    CKeyCombi();
    CKeyCombi(CKey const &inkey, CKey const &minerKey, int nVersion);
    CKeyCombi(CKey const &inkey, int nVersion);

    string ToString() const;

    Object ToJsonObj() const;
    bool UnSerializeFromJson(const Object &);
    int64_t GetBirthDay() const;
    bool GetCKey(CKey &keyOut, bool IsMiner = false) const;
    bool CreateANewKey();
    bool GetPubKey(CPubKey &mOutKey, bool IsMiner = false) const;
    bool CleanMainKey();
    bool CleanAll();
    bool HaveMinerKey() const;
    bool HaveMainKey() const;
    CKeyID GetCKeyID() const;
    void SetMainKey(CKey &mainKey);
    void SetMinerKey(CKey &minerKey);

	IMPLEMENT_SERIALIZE
	(
		if (0 == nVersion) {
			READWRITE(mMainPKey);
		}
		READWRITE(mMainCkey);
		if (0 == nVersion) {
			READWRITE(mMinerPKey);
		}
		READWRITE(mMinerCkey);
		READWRITE(nCreationTime);
	)
};

/** A virtual base class for key stores */
class CKeyStore {
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    // Add a key to the store.
    virtual bool AddKeyCombi(const CKeyID &keyId, const CKeyCombi &keyCombi) = 0;
    // virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey)        = 0;
    // virtual bool AddKey(const CKey &key);

    // Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CKeyID &address) const                           = 0;
    virtual bool GetKey(const CKeyID &address, CKey &keyOut, bool IsMine) const = 0;
    virtual void GetKeys(set<CKeyID> &setAddress, bool bFlag) const             = 0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut, bool IsMine) const;

    virtual bool AddCScript(const CMulsigScript &script)                        = 0;
    virtual bool HaveCScript(const CKeyID &keyId) const                       = 0;
    virtual bool GetCScript(const CKeyID &keyId, CMulsigScript &script) const = 0;
};

typedef map<CKeyID, CKeyCombi> KeyMap;
typedef map<CKeyID, CMulsigScript> ScriptMap;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore {
protected:
    KeyMap mapKeys;
    ScriptMap mapScripts;

public:
    bool AddKeyCombi(const CKeyID &keyId, const CKeyCombi &keyCombi);
    bool HaveKey(const CKeyID &address) const {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }
    void GetKeys(set<CKeyID> &setAddress, bool bFlag = false) const {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end()) {
                if (!bFlag)  // return all address in wallet
                    setAddress.insert((*mi).first);
                else if (mi->second.HaveMinerKey() ||
                         mi->second.HaveMainKey())  // only return satisfied mining address
                    setAddress.insert((*mi).first);

                mi++;
            }
        }
    }

    bool GetKey(const CKeyID &address, CKey &keyOut, bool IsMine = false) const {
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end()) {
                return mi->second.GetCKey(keyOut, IsMine);
            }
        }
        return false;
    }

    virtual bool GetKeyCombi(const CKeyID &address, CKeyCombi &keyCombiOut) const;

    bool HaveMainKey() {
        for (auto &item : mapKeys) {
            if (item.second.HaveMainKey()) return true;
        }

        return false;
    }

    virtual bool AddCScript(const CMulsigScript &script);
    virtual bool HaveCScript(const CKeyID &keyId) const;
    virtual bool GetCScript(const CKeyID &keyId, CMulsigScript &script) const;
};

typedef vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef map<CKeyID, pair<CPubKey, vector<unsigned char> > > CryptedKeyMap;

#endif  // ENTITIES_KEYSTORE_H
