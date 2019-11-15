// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_ID_H
#define ENTITIES_ID_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "crypto/hash.h"
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "key.h"

class CAccountDBCache;
class CUserID;
class CRegID;

enum AccountIDType {
    NULL_ID = 0,
    NICK_ID = 1,
    REG_ID  = 2,
    ADDRESS = 3,
};

typedef vector<uint8_t> UnsignedCharArray;
typedef CRegID CTxCord;

class CNullID {
public:
    friend bool operator==(const CNullID &a, const CNullID &b) { return true; }
    friend bool operator<(const CNullID &a, const CNullID &b) { return true; }
};

class CRegID {
public:
    CRegID(const string &strRegID);
    CRegID(const vector<uint8_t> &vIn);
    CRegID(const uint32_t height = 0, const uint16_t index = 0);

    const vector<uint8_t> &GetRegIdRaw() const;
    string ToRawString() const;
    void SetRegID(const vector<uint8_t> &vIn);
    CKeyID GetKeyId(const CAccountDBCache &accountCache) const;
    uint32_t GetHeight() const { return height; }
    uint16_t GetIndex() const { return index; }

    bool IsMature(uint32_t curHeight) const;

    bool operator==(const CRegID &other) const { return (this->height == other.height && this->index == other.index); }
    bool operator!=(const CRegID &other) const { return (this->height != other.height || this->index != other.index); }
    bool operator<(const CRegID &other) const {
        if (this->height == other.height) {
            return this->index < other.index;
        } else {
            return this->height < other.height;
        }
    }

    static bool IsSimpleRegIdStr(const string &str);
    static bool IsRegIdStr(const string &str);
    static bool GetKeyId(const string &str, CKeyID &keyId);
    bool IsEmpty() const { return (height == 0 && index == 0); }
    void SetEmpty() { Clear(); }
    bool Clear();
    string ToString() const;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(height));
        READWRITE(VARINT(index));
        if (fRead) {
            vRegID.clear();
            vRegID.insert(vRegID.end(), BEGIN(height), END(height));
            vRegID.insert(vRegID.end(), BEGIN(index), END(index));
        }
    )

private:
    uint32_t height;
    uint16_t index;
    mutable vector<uint8_t> vRegID;

    void SetRegID(string strRegID);
    void SetRegIDByCompact(const vector<uint8_t> &vIn);

    friend CUserID;
};

class CNickID {


public:
    string nickId;
    uint32_t regHeight;
    CNickID() {}
    CNickID(string nickIdIn) {
        if (nickIdIn.size() != 12) throw ios_base::failure(strprintf("Nickname length must be 12, but %s", nickIdIn.c_str()));
        nickId = nickIdIn;
    }

    CNickID(string nickIdIn, uint32_t height) {
        if (nickIdIn.size() != 12) throw ios_base::failure(strprintf("Nickname length must be 12, but %s", nickIdIn.c_str()));
        nickId = nickIdIn;
        regHeight = height ;
    }

    bool IsMature(const uint32_t currHeight) const ;
    const string &GetNickIdRaw() const { return nickId; }
    bool IsEmpty() const { return (nickId.size() == 0); }
    void Clear() { nickId.clear(); }
    string ToString() const { return nickId; }

    IMPLEMENT_SERIALIZE(READWRITE(nickId);)

    // Comparator implementation.
    friend bool operator==(const CNickID &a, const CNickID &b) { return a.nickId == b.nickId; }
    friend bool operator!=(const CNickID &a, const CNickID &b) { return !(a == b); }
    friend bool operator<(const CNickID &a, const CNickID &b) { return a.nickId < b.nickId; }
};

class CUserID {
private:
    boost::variant<CNullID, CRegID, CKeyID, CPubKey, CNickID> uid;

public:
    enum SerializeFlag {
        FlagNullType    = 0,
        FlagRegIDMin    = 2,
        FlagRegIDMax    = 10,
        FlagKeyID       = 20,
        FlagPubKey      = 33,  // public key
        FlagNickID      = 100
    };

public:
    static shared_ptr<CUserID> ParseUserId(const string &idStr);
    static const CUserID NULL_ID;
public:
    CUserID() : uid(CNullID()) {}

    template <typename ID>
    CUserID(const ID &id) : uid(CNullID()) {
        set(id);
    }

    template <typename ID>
    CUserID &operator=(const ID &id) {
        set(id);
        return *this;
    }

    template <typename ID>
    ID& get() {
        return boost::get<ID>(uid);
    }

    template <typename ID>
    const ID& get() const {
        return boost::get<ID>(uid);
    }

    template <typename ID>
    bool is() const {
        return uid.type() == typeid(ID);
    }

    bool IsEmpty() const {
        return is<CNullID>();
    }

    void SetEmpty() { uid = CNullID(); }

    template <typename ID>
    void set(const ID &idIn) {
        uid = idIn;
        if (!is<CNullID>() && ContentIsEmpty()) {
            uid = CNullID();
        }
    }

    const std::type_info &type() const { return uid.type(); }

public:
    std::string GetIDName() const {
        if (is<CRegID>()) {
            return "RegID";
        } else if (is<CKeyID>()) {
            return "KeyID";
        } else if (is<CPubKey>()) {
            return "PubKey";
        } else if (is<CNickID>()) {
            return "NickID";
        } else {
            assert(is<CNullID>());
            return "Null";
        }
    }

    string ToString() const {
        if (is<CRegID>()) {
            return get<CRegID>().ToString();
        } else if (is<CKeyID>()) {
            return get<CKeyID>().ToString();
        } else if (is<CPubKey>()) {
            return get<CPubKey>().ToString();
        } else if (is<CNickID>()) {
            return get<CNickID>().ToString();
        } else {
            assert(is<CNullID>());
            return "Null";
        }
    }

    string ToDebugString() const;

    friend bool operator==(const CUserID &id1, const CUserID &id2) {
        if (id1.type() != id2.type()) {
            return false;
        }
        if (id1.is<CRegID>()) {
            return id1.get<CRegID>() == id2.get<CRegID>();
        } else if (id1.is<CKeyID>()) {
            return id1.get<CKeyID>() == id2.get<CKeyID>();
        } else if (id1.is<CPubKey>()) {
            return id1.get<CPubKey>() == id2.get<CPubKey>();
        } else if (id1.is<CNickID>()) {
            return id1.get<CNickID>() == id2.get<CNickID>();
        } else {  // CNullID
            return true;
        }
    }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;
        string id = ToString();
        obj.push_back(json_spirit::Pair("id_type", GetIDName()));
        obj.push_back(json_spirit::Pair("id", id));
        return obj;
    }

    inline unsigned int GetSerializeSize(int nType, int nVersion) const {
        assert(is<CNullID>() || !ContentIsEmpty());
        if (uid.type() == typeid(CRegID)) {
            CRegID regId    = boost::get<CRegID>(uid);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            return sizeof(uint8_t) + sz;
        } else if (uid.type() == typeid(CKeyID)) {
            CKeyID keyId    = boost::get<CKeyID>(uid);
            unsigned int sz = keyId.GetSerializeSize(nType, nVersion);
            assert(sz == FlagKeyID);
            return sizeof(uint8_t) + sz;
        } else if (uid.type() == typeid(CPubKey)) {
            CPubKey pubKey  = boost::get<CPubKey>(uid);
            unsigned int sz = pubKey.GetSerializeSize(nType, nVersion);
            // If the public key is empty, length of serialized data is 1, otherwise, 34.
            assert(sz == sizeof(uint8_t) || sizeof(uint8_t) + FlagPubKey);
            return sz;
        } else if (uid.type() == typeid(CNickID)) {
            CNickID nickId = boost::get<CNickID>(uid);
            return sizeof(uint8_t) + nickId.GetSerializeSize(nType, nVersion);
        } else {  // CNullID
            return sizeof(uint8_t);
        }
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        assert(is<CNullID>() || !ContentIsEmpty());
        if (is<CRegID>()) {
            CRegID regId    = boost::get<CRegID>(uid);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            s << (uint8_t)sz << regId;
        } else if (is<CKeyID>()) {
            CKeyID keyId = boost::get<CKeyID>(uid);
            assert(keyId.GetSerializeSize(nType, nVersion) == FlagKeyID);
            s << (uint8_t)FlagKeyID << keyId;
        } else if (is<CPubKey>()) {
            CPubKey pubKey = boost::get<CPubKey>(uid);
            // If the public key is empty, length of serialized data is 1, otherwise, 34.
            assert(pubKey.GetSerializeSize(nType, nVersion) == sizeof(uint8_t) ||
                   pubKey.GetSerializeSize(nType, nVersion) == sizeof(uint8_t) + FlagPubKey);
            s << pubKey;
        } else if (is<CNickID>()) {
            CNickID nickId = boost::get<CNickID>(uid);
            s << (uint8_t)FlagNickID << nickId;
        } else {
            assert(is<CNullID>());
            s << (uint8_t)0;
        }
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        bool invalidId         = false;
        SerializeFlag typeFlag = (SerializeFlag)ReadCompactSize(s);

        if (typeFlag == FlagNickID) {  // idType >= 100
            CNickID nickId;
            s >> nickId;
            uid = nickId;
        } else {  // typeFlag is the length of id content
            int len = typeFlag;
            if (FlagRegIDMin <= len && len <= FlagRegIDMax) {
                CRegID regId(0, 0);
                s >> regId;
                uid = regId;
            } else if (len == FlagKeyID) {
                UnsignedCharArray vchData;
                vchData.resize(len);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                uint160 data = uint160(vchData);
                CKeyID keyId(data);
                uid = keyId;
            } else if (len == FlagPubKey) {  // public key
                UnsignedCharArray vchData;
                vchData.resize(len);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                CPubKey pubKey(vchData);
                uid = pubKey;
            } else if (len == FlagNullType) {  // null id type
                uid = CNullID();
            } else {
                invalidId = true;
            }
        }

        if (invalidId || (!is<CNullID>() && ContentIsEmpty())) {
            LogPrint("ERROR", "Invalid Unserialize CUserID\n");
            throw ios_base::failure("Unserialize CUserID error");
        }
    }
private:
    bool ContentIsEmpty() const {
        if (is<CRegID>()) {
            return get<CRegID>().IsEmpty();
        } else if (is<CKeyID>()) {
            return get<CKeyID>().IsEmpty();
        } else if (is<CPubKey>()) {
            return get<CPubKey>().IsEmpty();
        } else if (is<CNickID>()) {
            return get<CNickID>().IsEmpty();
        } else {
            assert(is<CNullID>());
            return true;
        }
    }
};

#endif //ENTITIES_ID_H