// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ID_H
#define ID_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "crypto/hash.h"

class CAccountCache;
class CUserID;
class CUserID;

typedef vector<unsigned char> vector_unsigned_char;
typedef CRegID TxCord;

class CNullID {
public:
    friend bool operator==(const CNullID &a, const CNullID &b) { return true; }
    friend bool operator<(const CNullID &a, const CNullID &b) { return true; }
};

class CRegID {
private:
    uint32_t nHeight;
    uint16_t nIndex;
    mutable vector<unsigned char> vRegID;

    void SetRegID(string strRegID);
    void SetRegIDByCompact(const vector<unsigned char> &vIn);

    friend CUserID;
public:
    CRegID(string strRegID);
    CRegID(const vector<unsigned char> &vIn);
    CRegID(uint32_t nHeight = 0, uint16_t nIndex = 0);

    const vector<unsigned char> &GetRegIdRaw() const {
        assert(vRegID.size() == 6);
        return vRegID;
    }

    string ToRawString() const {
        return string(vRegID.begin(), vRegID.end()); // TODO: change the vRegID to string
    }

    void SetRegID(const vector<unsigned char> &vIn);
    CKeyID GetKeyId(const CAccountCache &view) const;
    uint32_t GetHeight() const { return nHeight; }
    uint16_t GetIndex() const { return nIndex; }
    bool operator==(const CRegID &other) const { return (this->nHeight == other.nHeight && this->nIndex == other.nIndex); }
    bool operator!=(const CRegID &other) const { return (this->nHeight != other.nHeight || this->nIndex != other.nIndex); }
    bool operator<(const CRegID &other) const { return (this->nHeight < other.nHeight || this->nIndex < other.nIndex); }
    static bool IsSimpleRegIdStr(const string &str);
    static bool IsRegIdStr(const string &str);
    static bool GetKeyId(const string &str, CKeyID &keyId);
    bool IsEmpty() const { return (nHeight == 0 && nIndex == 0); };
    bool Clean();
    string ToString() const;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nHeight));
        READWRITE(VARINT(nIndex));
        if (fRead) {
            vRegID.clear();
            vRegID.insert(vRegID.end(), BEGIN(nHeight), END(nHeight));
            vRegID.insert(vRegID.end(), BEGIN(nIndex), END(nIndex));
        })
};

class CNickID {
private:
    string nickId;

public:
    CNickID() {}
    CNickID(string nickIdIn) {
        if (nickIdIn.size() > 32)
            throw ios_base::failure("Nickname ID length > 32 not allowed!");

        nickId = nickIdIn;
    }

    const string& GetNickIdRaw() const { return nickId; }
    bool IsEmpty() const { return (nickId.size() == 0); }
    void Clean() { nickId.clear(); }
    string ToString() const { return nickId; }

    IMPLEMENT_SERIALIZE(
        READWRITE(nickId);)

    // Comparator implementation.
    friend bool operator==(const CNickID &a, const CNickID &b) { return a.nickId == b.nickId; }
    friend bool operator!=(const CNickID &a, const CNickID &b) { return !(a == b); }
    friend bool operator<(const CNickID &a, const CNickID &b) { return a.nickId < b.nickId; }
};

class CUserID {
private:
    boost::variant<CNullID, CRegID, CKeyID, CPubKey, CNickID>  uid;

public:
    enum SerializeFlag {
        FlagNullType = 0,
        FlagRegIDMin = 2,
        FlagRegIDMax = 10,
        FlagKeyID    = 20,
        FlagPubKey   = 33,
        FlagNickID   = 100
    };

public:
    CUserID(): uid(CNullID()) {}

    template <typename ID>
    CUserID(const ID &id): uid(id) {}

    template <typename ID>
    CUserID& operator=(const ID& id) {
        uid = id;
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

    const std::type_info& type() const {
        return uid.type();
    }

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

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;
        string id = ToString();
        obj.push_back(json_spirit::Pair("idType", GetIDName()));
        obj.push_back(json_spirit::Pair("id", id));
        return obj;
    }

    inline unsigned int GetSerializeSize(int nType, int nVersion) const {
        if (uid.type() == typeid(CRegID)) {
            CRegID regId = boost::get<CRegID>(uid);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            return sizeof(unsigned char) + sz;
        } else if (uid.type() == typeid(CKeyID)) {
            CKeyID keyId = boost::get<CKeyID>(uid);
            unsigned int sz = keyId.GetSerializeSize(nType, nVersion);
            assert(sz == FlagKeyID);
            return sizeof(unsigned char) + sz;
        } else if (uid.type() == typeid(CPubKey)) {
            CPubKey pubKey = boost::get<CPubKey>(uid);
            unsigned int sz = pubKey.GetSerializeSize(nType, nVersion);
            assert(sz == sizeof(unsigned char) + FlagPubKey);
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
            CRegID regId = boost::get<CRegID>(uid);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            s << (unsigned char)sz << regId;
        } else if (uid.type() == typeid(CKeyID)) {
            CKeyID keyId = boost::get<CKeyID>(uid);
            assert(keyId.GetSerializeSize(nType, nVersion) == FlagKeyID);
            s << (unsigned char)FlagKeyID << keyId;
        } else if (uid.type() == typeid(CPubKey)) {
            CPubKey pubKey = boost::get<CPubKey>(uid);
            assert(pubKey.GetSerializeSize(nType, nVersion) == sizeof(unsigned char) + FlagPubKey);
            s << pubKey;
        } else if (uid.type() == typeid(CNickID)) {
            CNickID nickId = boost::get<CNickID>(uid);
            s << (unsigned char)FlagNickID << nickId;
        } else {  // CNullID
            s << (unsigned char)0;
        }
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        bool invalidId         = false;
        SerializeFlag typeFlag = (SerializeFlag)ReadCompactSize(s);

        if (typeFlag == FlagNullType) {
            uid = CNullID();
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
                vector_unsigned_char vchData;
                vchData.resize(len);
                assert(len > 0);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                uint160 data = uint160(vchData);
                CKeyID keyId(data);
                uid = keyId;
            } else if (len == FlagPubKey) {
                vector_unsigned_char vchData;
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
            LogPrint("ERROR", "Invalid Unserialize CUserId\n");
            throw ios_base::failure("Unserialize CUserId error");
        }
    }
};

#endif