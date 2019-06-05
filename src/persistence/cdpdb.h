// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDP_CACHE_H
#define PERSIST_CDP_CACHE_H

#include <map>
#include <string>
#include "leveldbwrapper.h"
#include "accounts/id.h"

using namespace std;

/**
 * CDP Cache Item: stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.1a/Log10(1+b*TNj)
 *
 * Persisted in LDB as:
 *      cdp$RegId --> { blockHeight, mintedScoins, totalOwedScoins }
 *
 */
struct CUserCdp {
    CRegID ownerRegId;              // CDP Owner RegId

    uint64_t lastBlockHeight;       // Hj, mem-only
    uint64_t lastOwedScoins;        // TNj, mem-only
    uint64_t collateralRatio;       // ratio = bcoins / mintedScoins, must be >= 200%

    uint64_t blockHeight;           // persisted: Hj+1
    uint64_t mintedScoins;          // persisted: mintedScoins = bcoins/rate
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCdp(): lastOwedScoins(0), mintedScoins(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(blockHeight);
        READWRITE(mintedScoins);
        READWRITE(totalStakedBcoins);
        READWRITE(totalOwedScoins);
    )

    bool IsEmpty() const {
        return ownerRegId.IsEmpty(); // TODO: need to check empty for other fields?
    }
};


class ICdpView {
public:
    virtual bool GetData(const string &key, CUserCdp &value) = 0;
    virtual bool SetData(const string &key, const CUserCdp &value) = 0;
    virtual bool BatchWrite(const map<string, CUserCdp> &mapCdps) = 0;
    virtual bool EraseKey(const string &key) = 0;
    virtual bool HaveData(const string &key) = 0;

    virtual ~ICdpView(){};
};

class CCdpCache: public ICdpView  {
public:
    CCdpCache() {};
    CCdpCache(ICdpView &base): ICdpView(), pBase(&base) {};

    bool SetStakeBcoins(CUserID txUid, uint64_t bcoinsToStake, uint64_t collateralRatio, uint64_t mintedScoins
                        int blockHeight, CDbOpLog &cdpDbOpLog);
    bool GetUnderLiquidityCdps(vector<CUserCdp> & userCdps);

public:
    virtual bool GetData(const string &key, CUserCdp &value);
    virtual bool SetData(const string &key, const CUserCdp &value);
    virtual bool BatchWrite(const map<string, CUserCdp > &mapContractDb);
    virtual bool EraseKey(const string &key);
    virtual bool HaveData(const string &key);

    bool GetCdpData();
    bool SetCdpData();

private:
    ICdpView *pBase               = nullptr;
    uint64_t collateralRatio      = 0;
    uint64_t liquidityPrice       = 0;
    uint64_t forcedLiquidityPrice = 0;
    uint64_t bcoinMedianPrice     = 0;

    map<string, CUserCdp> mapCdps;  //CUserID.ToString() --> CUserCdp
};

class CCdpDb: public ICdpView  {
private:
    CLevelDBWrapper db;

public:
    CCdpDb(const string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    CCdpDb(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    virtual bool GetData(const string &key, CUserCdp &value);
    virtual bool SetData(const string &key, const CUserCdp &value);
    virtual bool BatchWrite(const map<string, CUserCdp > &mapContractDb);
    virtual bool EraseKey(const string &key);
    virtual bool HaveData(const string &key);

private:
    CCdpDb(const CCdpDb &);
    void operator=(const CCdpDb &);

};

#endif //PERSIST_CDP_CACHE_H