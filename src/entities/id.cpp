// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "id.h"
#include "persistence/accountdb.h"
#include "main.h"

extern CCacheDBManager *pCdMan;

bool CRegID::Clear() {
    height = 0 ;
    index = 0 ;
    vRegID.clear();
    return true;
}

CRegID::CRegID(const vector<unsigned char>& vIn) {
    assert(vIn.size() == 6);
    vRegID = vIn;
    height = 0;
    index = 0;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> height;
    ds >> index;
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
        string firtstr = str.substr(0, pos);
        string endstr = str.substr(pos + 1);

        return IsDigitalString(firtstr) && IsDigitalString(endstr) ;
    }
    return false;
}

bool CRegID::GetKeyId(const string & str,CKeyID &keyId) {
    CRegID regId(str);
    if (regId.IsEmpty())
        return false;

    keyId = regId.GetKeyId(*pCdMan->pAccountCache);
    return !keyId.IsEmpty();
}

bool CRegID::IsRegIdStr(const string & str) {
    return ( IsSimpleRegIdStr(str) || (str.length() == 12) );
}

void CRegID::SetRegID(string strRegID) {
    height = 0;
    index  = 0;
    vRegID.clear();

    if (IsSimpleRegIdStr(strRegID)) {
        int pos = strRegID.find('-');
        height = atoi(strRegID.substr(0, pos).c_str());
        index  = atoi(strRegID.substr(pos + 1).c_str());
        vRegID.insert(vRegID.end(), BEGIN(height), END(height));
        vRegID.insert(vRegID.end(), BEGIN(index), END(index));
        // memcpy(&vRegID.at(0), &height, sizeof(height));
        // memcpy(&vRegID[sizeof(height)], &index, sizeof(index));
    } else if (strRegID.length() == 12) {
        vRegID = ::ParseHex(strRegID);
        memcpy(&height, &vRegID[0], sizeof(height));
        memcpy(&index, &vRegID[sizeof(height)], sizeof(index));
    }
}

void CRegID::SetRegID(const vector<unsigned char> &vIn) {
    assert(vIn.size() == 6);
    vRegID = vIn;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> height;
    ds >> index;
}

const vector<unsigned char> &CRegID::GetRegIdRaw() const {
    assert(vRegID.size() == 6);
    return vRegID;
}

string CRegID::ToRawString() const {
    return string(vRegID.begin(), vRegID.end());  // TODO: change the vRegID to string
}

CRegID::CRegID(string strRegID) { SetRegID(strRegID); }

CRegID::CRegID(uint32_t nHeightIn, uint16_t nIndexIn) {
    height = nHeightIn;
    index  = nIndexIn;
    vRegID.clear();
    vRegID.insert(vRegID.end(), BEGIN(nHeightIn), END(nHeightIn));
    vRegID.insert(vRegID.end(), BEGIN(nIndexIn), END(nIndexIn));
}

string CRegID::ToString() const {
    if (IsEmpty())
        return string("");

    return strprintf("%d-%d", height, index);
}

CKeyID CRegID::GetKeyId(const CAccountDBCache &accountCache) const {
    CKeyID retKeyId;
    CAccountDBCache(accountCache).GetKeyId(*this, retKeyId);
    return retKeyId;
}

bool CRegID::IsMature(uint32_t curHeight) const {
    return ((height == 0) && (index != 0)) || ((height != 0) && curHeight > height + REG_ID_MATURITY);
}

void CRegID::SetRegIDByCompact(const vector<unsigned char> &vIn) {
    if (vIn.size() > 0) {
        CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
        ds >> *this;
    } else {
        Clear();
    }
}

///////////////////////////////////////////////////////////////////////////////
// class CUserID

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

    // TODO: how to support nick name?

    return nullptr;
}

