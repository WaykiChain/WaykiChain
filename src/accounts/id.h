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

#include "id.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "chainparams.h"
#include "crypto/hash.h"
#include "vote.h"

typedef vector<unsigned char> vector_unsigned_char;
typedef boost::variant<CNullID, CRegID, CKeyID, CPubKey> CUserID;

class CID;
class CNullID;
class CAccountViewCache;

enum IDTypeEnum {
    NullType = 0,
    PubKey = 1,
    RegID = 2,
    KeyID = 3,
    NickID = 4,
};

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

public:
    friend class CID;
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

class CID {
private:
    vector_unsigned_char vchData;
    IDTypeEnum idType;

public:
    CID() {}
    CID(const CUserID &dest) { Set(dest); }

    const vector_unsigned_char &GetID() { return vchData; }
    static const vector_unsigned_char &UserIDToVector(const CUserID &userid) { return CID(userid).GetID(); }
    bool Set(const CRegID &id);
    bool Set(const CKeyID &id);
    bool Set(const CPubKey &id);
    bool Set(const CNullID &id);
    bool Set(const CUserID &userid);

    CUserID GetUserId() const;
    vector_unsigned_char GetData() const { return vchData; };

    string GetIDName() const {
        switch(idType) {
            case IDTypeEnum::NullType: return "Null";
            case IDTypeEnum::RegID: return "RegID";
            case IDTypeEnum::KeyID: return "KeyID";
            case IDTypeEnum::PubKey: return "PubKey";
            default:
                return "UnknownType";
        }
    }

    void SetIDType(IDTypeEnum idTypeIn) { idType = idTypeIn; }
    IDTypeEnum GetIDType() { return idType; }

    friend bool operator==(const CID &id1, const CID &id2) {
        return (id1.GetData() == id2.GetData());
    }

    string ToString() const {
        CUserID uid = GetUserId();
         switch(idType) {
            case IDTypeEnum::NullType:
                return "Null";
            case IDTypeEnum::RegID:
                return boost::get<CRegID>(uid).ToString();
            case IDTypeEnum::KeyID:
                return boost::get<CKeyID>(uid).ToString();
            case IDTypeEnum::PubKey:
                return boost::get<CPubKey>(uid).ToString();
            default:
                return "Unknown";
        }
    }

    Object ToJson() const {
        Object obj;
        string id = ToString();
        obj.push_back(Pair("idType", idType));
        obj.push_back(Pair("id", id));
        return obj;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vchData);)
};

class CIDVisitor : public boost::static_visitor<bool> {
private:
    CID *pId;

public:
    CIDVisitor(CID *pIdIn) : pId(pIdIn) {}

    bool operator()(const CRegID &id) const { pId->SetIDType(IDTypeEnum::RegID); return pId->Set(id); }
    bool operator()(const CKeyID &id) const { pId->SetIDType(IDTypeEnum::KeyID); return pId->Set(id); }
    bool operator()(const CPubKey &id) const { pId->SetIDType(IDTypeEnum::PubKey); return pId->Set(id); }
    bool operator()(const CNullID &no) const { return true; }
};

class CNickID  {
private:
    vector_unsigned_char nickId;

public:
    CNickID() {}
    CNickID(vector_unsigned_char nickIdIn) {
        if (nickIdIn.size() > 32)
            throw ios_base::failure("Nickname ID length > 32 not allowed!");

        nickId = nickIdIn;
     }

    vector_unsigned_char GetNickId() const { return nickId; }

    string ToString() const { return std::string(nickId.begin(), nickId.end()); }

    IMPLEMENT_SERIALIZE(
        READWRITE(nickId);)
};

#endif