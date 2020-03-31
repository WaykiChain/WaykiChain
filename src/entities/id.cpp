// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "id.h"
#include "persistence/accountdb.h"
#include "main.h"
#include "vm/wasm/types/name.hpp"
#include "vm/wasm/exception/exceptions.hpp"

extern CCacheDBManager *pCdMan;

CRegID::CRegID(const string &strRegID) { SetRegID(strRegID); }

CRegID::CRegID(const vector<uint8_t> &vRawDataIn) {
    SetRegID(vRawDataIn);
}

CRegID::CRegID(const uint32_t heightIn, const uint16_t indexIn) {
    height = heightIn;
    index  = indexIn;
}
CRegID::CRegID(const uint64_t& regIdIntValue) {
    height = regIdIntValue >> 20;
    index  = regIdIntValue & 0xFFFFF;
}

bool IsDigitalString(const string str){

    if (str.length() > 10 || str.length() == 0) //int max is 4294967295 can not over 10
        return false;

    for (auto te : str) {
        if (!isdigit(te))
            return false;
    }
    return true ;
}

bool CRegID::IsSimpleRegIdStr(const string & str) {
    int len = str.length();
    if (len >= 3) {
        int pos = str.find('-');

        if (pos > len - 1) {
            return false;
        }
        string firstStr = str.substr(0, pos);
        string endStr = str.substr(pos + 1);

        return IsDigitalString(firstStr) && IsDigitalString(endStr) ;
    }
    return false;
}

bool CRegID::SetRegID(const string &regidStr) {
    Clear();
    if (!IsSimpleRegIdStr(regidStr)) return false;
    auto pos = regidStr.find('-');
    height   = atoi(regidStr.substr(0, pos).c_str());
    index    = atoi(regidStr.substr(pos + 1).c_str());
    return true;
}

bool CRegID::SetRegID(const vector<uint8_t> &vIn) {
    Clear();
    if (vIn.size() != RAW_SIZE) return false;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> height;
    ds >> index;
    return true;
}

bool CRegID::GetKeyId(const string &str, CKeyID &keyId) {
    CRegID regId(str);
    if (regId.IsEmpty())
        return false;

    keyId = regId.GetKeyId(*pCdMan->pAccountCache);
    return !keyId.IsEmpty();
}

bool CRegID::IsRegIdStr(const string & str) {
    return ( IsSimpleRegIdStr(str) || (str.length() == 12) );
}

vector<uint8_t> CRegID::GetRegIdRaw() const {
    vector<uint8_t> ret;
    ret.insert(ret.end(), BEGIN(height), END(height));
    ret.insert(ret.end(), BEGIN(index), END(index));
    return ret;
}

void CRegID::Clear() {
    height = 0;
    index  = 0;
}

string CRegID::ToString() const {
    if (IsEmpty())
        return string("");

    return strprintf("%d-%d", height, index);
}

CKeyID CRegID::GetKeyId(const CAccountDBCache &accountCache) const {
    CKeyID retKeyId;
    accountCache.GetKeyId(*this, retKeyId);

    return retKeyId;
}

bool CRegID::IsMature(uint32_t curHeight) const {
    return ((height == 0) && (index != 0)) || ((height != 0) && curHeight > height + REG_ID_MATURITY);
}

///////////////////////////////////////////////////////////////////////////////
// class CUserID

const CUserID CUserID::NULL_ID = {};
const EnumTypeMap<CUserID::VarIndex, string> CUserID::ID_NAME_MAP = {
    {IDX_NULL_ID,       "Null"  },
    {IDX_REG_ID,        "RegID" },
    {IDX_KEY_ID,        "KeyID" },
    {IDX_PUB_KEY_ID,    "PubKey"},
};

string CUserID::ToDebugString() const {
    return std::visit([&](auto&& idIn) -> string {
        using ID = std::decay_t<decltype(idIn)>;
        if constexpr (std::is_same_v<ID, CRegID>) {
            return "R:" + idIn.ToString();
        } else if constexpr (std::is_same_v<ID, CKeyID>) {
            return "K:" + idIn.ToString() + ", addr=" + idIn.ToAddress();
        } else if constexpr (std::is_same_v<ID, CPubKey>) {
            return "P:" + idIn.ToString() + ", addr=" + idIn.GetKeyId().ToAddress();
        } else {  // CNullID
            assert( (std::is_same_v<ID, CNullID>) );
            return "Null";
        }
    }, uid);
}

shared_ptr<CUserID> CUserID::ParseUserId(const string &idStr) {
    CRegID regId(idStr);
    if (!regId.IsEmpty())
        return std::make_shared<CUserID>(regId);

    CKeyID keyId(idStr);
    if (!keyId.IsEmpty())
        return std::make_shared<CUserID>(keyId);

    // ParsePubKey
    auto pubKeyBin = ParseHex(idStr);
    CPubKey pubKey(pubKeyBin);
    if (pubKey.IsFullyValid())
        return std::make_shared<CUserID>(pubKey);

    return nullptr;
}
