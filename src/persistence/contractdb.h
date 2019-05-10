// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CONTRACTDB_H
#define PERSIST_CONTRACTDB_H

#include "commons/arith_uint256.h"
#include "database.h"
#include "leveldbwrapper.h"
#include "main.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class uint256;
class CKeyID;
class CTransactionDBCache;
// -dbcache default (MiB)
static const int64_t nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t nMaxDbCache = sizeof(void *) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t nMinDbCache = 4;

// class CTransactionDB : public CTransactionDBView {
// private:
//     CLevelDBWrapper db;

// public:
//     CTransactionDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false) : 
//         db(GetDataDir() / "blocks" / "txcache", nCacheSize, fMemory, fWipe) {};
//     ~CTransactionDB() {};

// private:
//     CTransactionDB(const CTransactionDB &);
//     void operator=(const CTransactionDB &);

// public:
//     virtual bool IsContainBlock(const CBlock &block);
//     virtual bool BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHash);
//     int64_t GetDbCount() { return db.GetDbCount(); }
// };

class CScriptDB : public CScriptDBView {
private:
    CLevelDBWrapper db;

public:
    CScriptDB(const string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    CScriptDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

private:
    CScriptDB(const CScriptDB &);
    void operator=(const CScriptDB &);

public:
    bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue);
    bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue);
    bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb);
    bool EraseKey(const vector<unsigned char> &vKey);
    bool HaveData(const vector<unsigned char> &vKey);
    bool GetScript(const int nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue);
    bool GetContractData(const int curBlockHeight, const vector<unsigned char> &vScriptId, const int &nIndex, 
                        vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData);
    int64_t GetDbCount() { return db.GetDbCount(); }
    bool GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &mapTxHash);
    Object ToJsonObj(string Prefix);
    bool GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc);
};

#endif  // PERSIST_CONTRACTDB_H
