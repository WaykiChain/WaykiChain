// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Coin developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MINER_H
#define COIN_MINER_H

#include <stdint.h>
#include <vector>
#include <map>
#include <set>

#include "uint256.h"
#include "key.h"
#include "boost/tuple/tuple.hpp"
#include <memory>

class CBlock;
class CBlockIndex;
struct CBlockTemplate;
//class CReserveKey;
//class CScript;
class CWallet;
class CBaseTransaction;
class COrphan;
class CAccountViewCache;
class CTransactionDBCache;
class CScriptDBViewCache;
class CAccount;

typedef boost::tuple<double, double, std::shared_ptr<CBaseTransaction> > TxPriority;
class TxPriorityCompare
{
    bool byFee;
public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }
    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        }
        else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

/** Run the miner threads */
void GenerateCoinBlock(bool fGenerate, CWallet* pwallet, int nThreads);
/** Generate a new block */
CBlockTemplate* CreateNewBlock(CAccountViewCache &view, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache);
/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
/** Do mining precalculation */
void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1);

bool CreatePosTx(const int64_t currentTime, const CAccount &delegate, CAccountViewCache &view, CBlock *pBlock);

bool GetDelegatesAcctList(vector<CAccount> & vDelegatesAcctList);
bool GetDelegatesAcctList(vector<CAccount> & vDelegatesAcctList, CAccountViewCache &accViewIn, CTransactionDBCache &txCacheIn, CScriptDBViewCache &scriptCacheIn);

void ShuffleDelegates(const int nCurHeight, vector<CAccount> &vDelegatesList);

bool GetCurrentDelegate(const int64_t currentTime,  const vector<CAccount> &vDelegatesAcctList, CAccount &delegateAcct);

bool VerifyPosTx(const CBlock *pBlock, CAccountViewCache &accView, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache, bool bNeedRunTx = false);
/** Check mined block */
bool CheckWork(CBlock* pblock, CWallet& wallet);
/** Base sha256 mining transform */
void SHA256Transform(void* pstate, void* pinput, const void* pinit);
/** Get burn element */
int GetElementForBurn(CBlockIndex *pindex);

void GetPriorityTx(vector<TxPriority> &vecPriority, int nFuelRate);

extern uint256 CreateBlockWithAppointedAddr(CKeyID const &keyID);

#endif // COIN_MINER_H
