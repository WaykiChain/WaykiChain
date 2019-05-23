// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_STAKEDB_H
#define PERSIST_STAKEDB_H

#include "leveldbwrapper.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace std;

class CAccount;
class CAccountLog;

class CStakeDBOperLog {
public:
    CStakeDBOperLog(const vector<unsigned char> &vKeyIn, const vector<unsigned char> &vValueIn) {
        vKey   = vKeyIn;
        vValue = vValueIn;
    }
    CStakeDBOperLog() {
        vKey.clear();
        vValue.clear();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vKey);
        READWRITE(vValue);
    )

    string ToString() const {
        string str;
        str += strprintf("vKey: %s, vValue: %s", HexStr(vKey), HexStr(vValue));
        return str;
    }

    friend bool operator<(const CStakeDBOperLog &log1, const CStakeDBOperLog &log2) { return log1.vKey < log2.vKey; }

private:
    vector<unsigned char> vKey;
    vector<unsigned char> vValue;
};

class IStakeView {
public:
    virtual bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue)                = 0;
    virtual bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue)                      = 0;
    virtual bool EraseKey(const vector<unsigned char> &vKey)                                                    = 0;
    virtual bool HaveData(const vector<unsigned char> &vKey)                                                    = 0;
    virtual bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &voteDb)                   = 0;
    virtual bool GetDelegateVote(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue) = 0;
    virtual bool GetStakedFcoins(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue) = 0;

    virtual ~IStakeView(){};
};

class CStakeDB : public IStakeView {
private:
    CLevelDBWrapper db;

public:
    CStakeDB(const string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    CStakeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetDelegateVote(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue);
    bool GetStakedFcoins(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue);

private:
    CStakeDB(const CStakeDB &);
    void operator=(const CStakeDB &);

    bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue);
    bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue);
    bool EraseKey(const vector<unsigned char> &vKey);
    bool HaveData(const vector<unsigned char> &vKey);
    bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &voteDb);

    bool GetData(const size_t prefixLen, const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue);
};

class CStakeCache : public IStakeView {
protected:
    IStakeView *pBase;

public:
    map<vector<unsigned char>, vector<unsigned char> > voteDb;

public:
    CStakeCache(IStakeView &pBaseIn) : pBase(&pBaseIn) { voteDb.clear(); };

    /** delegate vote */
    bool SetDelegateVote(const CAccount &delegateAcct, CStakeDBOperLog &operLog);
    bool SetDelegateVote(const vector<unsigned char> &vKey);
    bool EraseDelegateVote(const CAccountLog &delegateAcct, CStakeDBOperLog &operLog);
    bool EraseDelegateVote(const vector<unsigned char> &vKey);
    bool GetDelegateVote(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue);

    /** staked fcoins */
    bool SetStakedFcoins(const CAccount &delegateAcct, CStakeDBOperLog &operLog);
    bool SetStakedFcoins(const vector<unsigned char> &vKey);
    bool EraseStakedFcoins(const CAccountLog &delegateAcct, CStakeDBOperLog &operLog);
    bool EraseStakedFcoins(const vector<unsigned char> &vKey);
    bool GetStakedFcoins(const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue);

    bool Flush();
    unsigned int GetCacheSize();

    IStakeView *GetBaseView() { return pBase; }
    void SetBaseView(IStakeView *pBaseIn) { pBase = pBaseIn; };

private:
    bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue);
    bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue);
    bool EraseKey(const vector<unsigned char> &vKey);
    bool HaveData(const vector<unsigned char> &vKey);
    bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &voteDb);

    bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue, CStakeDBOperLog &operLog);
    bool EraseData(const vector<unsigned char> &vKey, CStakeDBOperLog &operLog);

    bool GetData(const size_t prefixLen, const int &nIndex, vector<unsigned char> &vKey, vector<unsigned char> &vValue);
};

#endif  // PERSIST_STAKEDB_H
