// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef ID_H
#define ID_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "chainparams.h"
#include "crypto/hash.h"

class CAccountViewCache;
class CUserID;

typedef vector<unsigned char> vector_unsigned_char;

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

    const vector<unsigned char> &GetVec6() const {
        assert(vRegID.size() == 6);
        return vRegID;
    }
    void SetRegID(const vector<unsigned char> &vIn);
    CKeyID GetKeyId(const CAccountViewCache &view) const;
    uint32_t GetHeight() const { return nHeight; }
    bool operator==(const CRegID &co) const { return (this->nHeight == co.nHeight && this->nIndex == co.nIndex); }
    bool operator!=(const CRegID &co) const { return (this->nHeight != co.nHeight || this->nIndex != co.nIndex); }
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
    vector_unsigned_char nickId;

public:
    CNickID() {}
    CNickID(vector_unsigned_char nickIdIn) {
        if (nickIdIn.size() > 32) throw ios_base::failure("Nickname ID length > 32 not allowed!");

        nickId = nickIdIn;
    }

    vector_unsigned_char GetNickId() const { return nickId; }

    string ToString() const { return std::string(nickId.begin(), nickId.end()); }

    IMPLEMENT_SERIALIZE(READWRITE(nickId);)

    // Comparator implementation.
    friend bool operator==(const CNickID &a, const CNickID &b) { return a == b; }
    friend bool operator!=(const CNickID &a, const CNickID &b) { return !(a == b); }
    friend bool operator<(const CNickID &a, const CNickID &b) { return a < b; }
};

typedef boost::variant<CNullID, CRegID, CKeyID, CPubKey, CNickID> __CUserID;

class CUserID : public __CUserID {
    using __CUserID::__CUserID;

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
    std::string GetIDName() const {
        if (type() == typeid(CRegID)) {
            return "RegID";
        } else if (type() == typeid(CKeyID)) {
            return "KeyID";
        } else if (type() == typeid(CPubKey)) {
            return "PubKey";
        } else if (type() == typeid(CNickID)) {
            return "NickID";
        } else if (type() == typeid(CNullID)) {
            return "Null";
        } else {
            return "UnknownType";
        }
    }

    string ToString() const {
        if (type() == typeid(CRegID)) {
            return boost::get<CRegID>(*this).ToString();
        } else if (type() == typeid(CKeyID)) {
            return boost::get<CKeyID>(*this).ToString();
        } else if (type() == typeid(CPubKey)) {
            return boost::get<CPubKey>(*this).ToString();
        } else if (type() == typeid(CNickID)) {
            return boost::get<CNickID>(*this).ToString();
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
            return boost::get<CRegID>(id1) == boost::get<CRegID>(id2);
        } else if (id1.type() == typeid(CKeyID)) {
            return boost::get<CKeyID>(id1) == boost::get<CKeyID>(id2);
        } else if (id1.type() == typeid(CPubKey)) {
            return boost::get<CPubKey>(id1) == boost::get<CPubKey>(id2);
        } else if (id1.type() == typeid(CNickID)) {
            return boost::get<CNickID>(id1) == boost::get<CNickID>(id2);
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
        if (type() == typeid(CRegID)) {
            CRegID regId = boost::get<CRegID>(*this);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            return sizeof(unsigned char) + sz;
        } else if (type() == typeid(CKeyID)) {
            CKeyID keyId = boost::get<CKeyID>(*this);
            unsigned int sz = keyId.GetSerializeSize(nType, nVersion);
            assert(sz == FlagKeyID);
            return sizeof(unsigned char) + sz;
        } else if (type() == typeid(CPubKey)) {
            CPubKey pubKey = boost::get<CPubKey>(*this);
            unsigned int sz = pubKey.GetSerializeSize(nType, nVersion);
            assert(sz == sizeof(unsigned char) + FlagPubKey);
            return sz;
        } else if (type() == typeid(CNickID)) {
            CNickID nickId = boost::get<CNickID>(*this);
            return sizeof(unsigned char) + nickId.GetSerializeSize(nType, nVersion);
        } else {  // CNullID
            return sizeof(unsigned char);
        }
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        if (type() == typeid(CRegID)) {
            CRegID regId = boost::get<CRegID>(*this);
            unsigned int sz = regId.GetSerializeSize(nType, nVersion);
            assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
            s << (unsigned char)sz << regId;
        } else if (type() == typeid(CKeyID)) {
            CKeyID keyId = boost::get<CKeyID>(*this);
            assert(keyId.GetSerializeSize(nType, nVersion) == FlagKeyID);
            s << (unsigned char)FlagKeyID << keyId;
        } else if (type() == typeid(CPubKey)) {
            CPubKey pubKey = boost::get<CPubKey>(*this);
            assert(pubKey.GetSerializeSize(nType, nVersion) == sizeof(unsigned char) + FlagPubKey);
            s << pubKey;
        } else if (type() == typeid(CNickID)) {
            CNickID nickId = boost::get<CNickID>(*this);
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
            *this = CNullID();
        } else if (typeFlag == FlagNickID) {  // idType >= 100
            CNickID nickId;
            s >> nickId;
            *this = nickId;
        } else {  // it is the length of id content
            int len = typeFlag;
            if (FlagRegIDMin <= len && len <= FlagRegIDMax) {
                CRegID regId(0, 0);
                s >> regId;
                *this = regId;
            } else if (len == FlagKeyID) {
                vector_unsigned_char vchData;
                vchData.resize(len);
                assert(len > 0);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                uint160 data = uint160(vchData);
                CKeyID keyId(data);
                *this = keyId;
            } else if (len == FlagPubKey) {
                vector_unsigned_char vchData;
                vchData.resize(len);
                assert(len > 0);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                CPubKey pubKey(vchData);
                *this = pubKey;
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