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
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"

class CAccountDBCache;
class CUserID;
class CRegID;

enum AccountIDType {
    NULL_ID = 0,
    NICK_ID,
    REG_ID,
    ADDRESS
};

typedef tuple<AccountIDType, string> ComboAccountID;
typedef vector<unsigned char> UnsignedCharArray;
typedef CRegID CTxCord;

class CNullID {
public:
    friend bool operator==(const CNullID &a, const CNullID &b) { return true; }
    friend bool operator<(const CNullID &a, const CNullID &b) { return true; }
};

class CRegID {
private:
    uint32_t height;
    uint16_t index;
    mutable vector<unsigned char> vRegID;

    void SetRegID(string strRegID);
    void SetRegIDByCompact(const vector<unsigned char> &vIn);

    friend CUserID;

public:
    CRegID(string strRegID);
    CRegID(const vector<unsigned char> &vIn);
    CRegID(uint32_t height = 0, uint16_t index = 0);

    const vector<unsigned char> &GetRegIdRaw() const;
    string ToRawString() const;
    void SetRegID(const vector<unsigned char> &vIn);
    CKeyID GetKeyId(const CAccountDBCache &accountCache) const;
    uint32_t GetHeight() const { return height; }
    uint16_t GetIndex() const { return index; }

    bool IsMature(uint32_t curHeight) const;

    bool operator==(const CRegID &other) const {
        return (this->height == other.height && this->index == other.index);
    }
    bool operator!=(const CRegID &other) const {
        return (this->height != other.height || this->index != other.index);
    }
    bool operator<(const CRegID &other) const { return (this->height < other.height || this->index < other.index); }
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
};

/**
 * tx cord, locate a tx with its block height and index
 */
typedef CRegID CTxCord;

class CNickID {
private:
    string nickId;

public:
    CNickID() {}
    CNickID(string nickIdIn) {
        if (nickIdIn.size() > 32) throw ios_base::failure("Nickname ID length > 32 not allowed!");

        nickId = nickIdIn;
    }

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
        FlagEmptyPubKey = 0,  // public key
        FlagRegIDMin    = 2,
        FlagRegIDMax    = 10,
        FlagKeyID       = 20,
        FlagPubKey      = 33,  // public key
        FlagNullType    = 100,
        FlagNickID      = 101,
    };

public:
    static shared_ptr<CUserID> ParseUserId(const string &idStr);

public:
    CUserID() : uid(CNullID()) {}

    template <typename ID>
    CUserID(const ID &id) : uid(id) {}

    template <typename ID>
    CUserID &operator=(const ID &id) {
        uid = id;
        return *this;
    }

    template <typename ID>
    ID &get() {
        return boost::get<ID>(uid);
    }

    template <typename ID>
    const ID &get() const {
        return boost::get<ID>(uid);
    }

    const std::type_info &type() const { return uid.type(); }

public:
    std::string GetIDName() const {
        if (uid.type() == typeid(CRegID)) {
            return "RegID";
        } else if (uid.type() == typeid(CKeyID)) {
            return "KeyID";
        } else if (uid.type() == typeid(CPubKey)) {
            return "PubKey";
        } else if (uid.type() == typeid(CNickID)) {
            return "NickID";
        } else if (uid.type() == typeid(CNullID)) {
            return "Null";
        } else {
            return "UnknownType";
        }
    }

    string ToString() const {
        if (uid.type() == typeid(CRegID)) {
            return boost::get<CRegID>(uid).ToString();
        } else if (type() == typeid(CKeyID)) {
            return boost::get<CKeyID>(uid).ToString();
        } else if (type() == typeid(CPubKey)) {
            return boost::get<CPubKey>(uid).ToString();
        } else if (type() == typeid(CNickID)) {
            return boost::get<CNickID>(uid).ToString();
        } else if (type() == typeid(CNullID)) {
            return "Null";
        } else {
            return "Unknown";
        }
    }

    friend bool operator==(const CUserID &id1, const CUserID &id2) {
        if (id1.type() != id2.type()) {
            return false;
        }
        if (id1.type() == typeid(CRegID)) {
            return id1.get<CRegID>() == id2.get<CRegID>();
        } else if (id1.type() == typeid(CKeyID)) {
            return id1.get<CKeyID>() == id2.get<CKeyID>();
        } else if (id1.type() == typeid(CPubKey)) {
            return id1.get<CPubKey>() == id2.get<CPubKey>();
        } else if (id1.type() == typeid(CNickID)) {
            return id1.get<CNickID>() == id2.get<CNickID>();
        } else {  // CNullID
            return sizeof(unsigned char);
        }
    }

    bool IsEmpty() const { return type() == typeid(CNullID); }
    void SetEmpty() { uid = CNullID(); }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;
        string id = ToString();
        obj.push_back(json_spirit::Pair("id_type", GetIDName()));
        obj.push_back(json_spirit::Pair("id", id));
        return obj;
    }

    inline unsigned int GetSerializeSize(int nType, int nVersion) const {
        if (uid.type() == typeid(CRegID)) {
            CRegID regId    = boost::get<CRegID>(uid);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            return sizeof(unsigned char) + sz;
        } else if (uid.type() == typeid(CKeyID)) {
            CKeyID keyId    = boost::get<CKeyID>(uid);
            unsigned int sz = keyId.GetSerializeSize(nType, nVersion);
            assert(sz == FlagKeyID);
            return sizeof(unsigned char) + sz;
        } else if (uid.type() == typeid(CPubKey)) {
            CPubKey pubKey  = boost::get<CPubKey>(uid);
            unsigned int sz = pubKey.GetSerializeSize(nType, nVersion);
            // If the public key is empty, length of serialized data is 1, otherwise, 34.
            assert(sz == sizeof(unsigned char) || sizeof(unsigned char) + FlagPubKey);
            return sz;
        } else if (uid.type() == typeid(CNickID)) {
            CNickID nickId = boost::get<CNickID>(uid);
            return sizeof(unsigned char) + nickId.GetSerializeSize(nType, nVersion);
        } else {  // CNullID
            return sizeof(unsigned char);
        }
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        if (uid.type() == typeid(CRegID)) {
            CRegID regId    = boost::get<CRegID>(uid);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            s << (unsigned char)sz << regId;
        } else if (uid.type() == typeid(CKeyID)) {
            CKeyID keyId = boost::get<CKeyID>(uid);
            assert(keyId.GetSerializeSize(nType, nVersion) == FlagKeyID);
            s << (unsigned char)FlagKeyID << keyId;
        } else if (uid.type() == typeid(CPubKey)) {
            CPubKey pubKey = boost::get<CPubKey>(uid);
            // If the public key is empty, length of serialized data is 1, otherwise, 34.
            assert(pubKey.GetSerializeSize(nType, nVersion) == sizeof(unsigned char) ||
                   pubKey.GetSerializeSize(nType, nVersion) == sizeof(unsigned char) + FlagPubKey);
            s << pubKey;
        } else if (uid.type() == typeid(CNickID)) {
            CNickID nickId = boost::get<CNickID>(uid);
            s << (unsigned char)FlagNickID << nickId;
        } else {  // CNullID
            s << (unsigned char)100;
        }
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        bool invalidId         = false;
        SerializeFlag typeFlag = (SerializeFlag)ReadCompactSize(s);

        if (typeFlag == FlagNullType) {
            uid = CNullID();
        } else if (typeFlag == FlagEmptyPubKey) {  // public key
            uid = CPubKey();
        } else if (typeFlag == FlagNickID) {  // idType >= 100
            CNickID nickId;
            s >> nickId;
            uid = nickId;
        } else {  // it is the length of id content
            int len = typeFlag;
            if (FlagRegIDMin <= len && len <= FlagRegIDMax) {
                CRegID regId(0, 0);
                s >> regId;
                uid = regId;
            } else if (len == FlagKeyID) {
                UnsignedCharArray vchData;
                vchData.resize(len);
                assert(len > 0);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                uint160 data = uint160(vchData);
                CKeyID keyId(data);
                uid = keyId;
            } else if (len == FlagPubKey) {  // public key
                UnsignedCharArray vchData;
                vchData.resize(len);
                assert(len > 0);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                CPubKey pubKey(vchData);
                uid = pubKey;
            } else {
                invalidId = true;
            }
        }

        if (invalidId) {
            LogPrint("ERROR", "Invalid Unserialize CUserID\n");
            throw ios_base::failure("Unserialize CUserID error");
        }
    }
};

#endif //ENTITIES_ID_H