// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "tx/tx.h"
#include "id.h"
#include "persistence/accountview.h"

extern CAccountViewCache *pAccountViewTip; /** global account db cache*/

bool CRegID::Clean() {
    nHeight = 0 ;
    nIndex = 0 ;
    vRegID.clear();
    return true;
}

CRegID::CRegID(const vector<unsigned char>& vIn) {
    assert(vIn.size() == 6);
    vRegID = vIn;
    nHeight = 0;
    nIndex = 0;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> nHeight;
    ds >> nIndex;
}

bool CRegID::IsSimpleRegIdStr(const string & str)
{
    int len = str.length();
    if (len >= 3) {
        int pos = str.find('-');

        if (pos > len - 1) {
            return false;
        }
        string firtstr = str.substr(0, pos);

        if (firtstr.length() > 10 || firtstr.length() == 0) //int max is 4294967295 can not over 10
            return false;

        for (auto te : firtstr) {
            if (!isdigit(te))
                return false;
        }
        string endstr = str.substr(pos + 1);
        if (endstr.length() > 10 || endstr.length() == 0) //int max is 4294967295 can not over 10
            return false;
        for (auto te : endstr) {
            if (!isdigit(te))
                return false;
        }
        return true;
    }
    return false;
}

bool CRegID::GetKeyId(const string & str,CKeyID &keyId) {
    CRegID regId(str);
    if (regId.IsEmpty())
        return false;

    keyId = regId.GetKeyId(*pAccountViewTip);
    return !keyId.IsEmpty();
}

bool CRegID::IsRegIdStr(const string & str) {
    return ( IsSimpleRegIdStr(str) || (str.length() == 12) );
}

void CRegID::SetRegID(string strRegID)
{
    nHeight = 0;
    nIndex = 0;
    vRegID.clear();

    if (IsSimpleRegIdStr(strRegID)) {
        int pos = strRegID.find('-');
        nHeight = atoi(strRegID.substr(0, pos).c_str());
        nIndex = atoi(strRegID.substr(pos+1).c_str());
        vRegID.insert(vRegID.end(), BEGIN(nHeight), END(nHeight));
        vRegID.insert(vRegID.end(), BEGIN(nIndex), END(nIndex));
//      memcpy(&vRegID.at(0),&nHeight,sizeof(nHeight));
//      memcpy(&vRegID[sizeof(nHeight)],&nIndex,sizeof(nIndex));
    } else if (strRegID.length() == 12) {
        vRegID = ::ParseHex(strRegID);
        memcpy(&nHeight,&vRegID[0],sizeof(nHeight));
        memcpy(&nIndex,&vRegID[sizeof(nHeight)],sizeof(nIndex));
    }
}

void CRegID::SetRegID(const vector<unsigned char>& vIn)
{
    assert(vIn.size() == 6);
    vRegID = vIn;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> nHeight;
    ds >> nIndex;
}

CRegID::CRegID(string strRegID)
{
    SetRegID(strRegID);
}

CRegID::CRegID(uint32_t nHeightIn, uint16_t nIndexIn)
{
    nHeight = nHeightIn;
    nIndex = nIndexIn;
    vRegID.clear();
    vRegID.insert(vRegID.end(), BEGIN(nHeightIn), END(nHeightIn));
    vRegID.insert(vRegID.end(), BEGIN(nIndexIn), END(nIndexIn));
}

string CRegID::ToString() const
{
    if (IsEmpty())
        return string("");

    return  strprintf("%d-%d", nHeight, nIndex);
}

CKeyID CRegID::GetKeyId(const CAccountViewCache &view)const
{
    CKeyID retKeyId;
    CAccountViewCache(view).GetKeyId(*this, retKeyId);
    return retKeyId;
}

void CRegID::SetRegIDByCompact(const vector<unsigned char> &vIn)
{
    if (vIn.size() > 0) {
        CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
        ds >> *this;
    } else {
        Clean();
    }
}