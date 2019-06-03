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
 * stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.1a/Log10(1+b*TNj)
 *
 */
struct CdpStakeOp {
    uint64_t lastBlockHeight;       // Hj, mem-only
    uint64_t lastTotalStakedScoins; // TNj, mem-only

    uint64_t blockHeight;           // Hj+1, persisted
    uint64_t mintedScoins;          // persisted
};

/**
 *  Persisted in LDB as:
 *      cdp$RegId --> { blockHeight, mintedScoins }
 *
 */
struct CUserCdp {
    CRegID ownerRegId;              // CDP Owner RegId
    uint64_t totalStakedBcoins;     // summed from stakeOps
    uint64_t totalMintedScoins;     // summed from stakeOps
    uint64_t owedScoins;            // owed = total minted - total redempted
    CdpStakeOp stakeOp;
};


class ICdpView {
public:
    virtual bool GetData(const string &vKey, vector<unsigned char> &vValue) = 0;
    virtual bool SetData(const string &vKey, const vector<unsigned char> &vValue) = 0;
    virtual bool BatchWrite(const map<string, vector<unsigned char> > &mapCdps) = 0;
    virtual bool EraseKey(const string &vKey) = 0;
    virtual bool HaveData(const string &vKey) = 0;

    virtual ~ICdpView(){};
};

class CCdpCache: public ICdpView  {
public:
    bool SetStakeBcoins(CUserID txUid, uint64_t bcoinsToStake, int blockHeight, CDBOpLog &cdpDbOpLog);
    bool GetLiquidityCdpItems(vector<CdpItem> & cdpItems);

public:
    virtual bool GetData(const string &key, vector<unsigned char> &vValue);
    virtual bool SetData(const string &vKey, const vector<unsigned char> &vValue);
    virtual bool BatchWrite(const map<string, vector<unsigned char> > &mapContractDb);
    virtual bool EraseKey(const string &vKey);
    virtual bool HaveData(const string &vKey);

    bool GetCdpData();
    bool SetCdpData();

private:
    uint64_t collateralRatio;
    uint64_t liquidityPrice;
    uint64_t forcedLiquidityPrice;

    uint64_t bcoinMedianPrice;
    map<string, CUserCdp> mapCdps; // UserRegId --> UserCDP
};

class CCdpDb: public ICdpView  {

public:
    ~CCdpDb() {};
    virtual bool GetData(const string &vKey, vector<unsigned char> &vValue) = 0;
    virtual bool SetData(const string &vKey, const vector<unsigned char> &vValue) = 0;
    virtual bool BatchWrite(const map<string, vector<unsigned char> > &mapContractDb) = 0;
    virtual bool EraseKey(const string &vKey) = 0;
    virtual bool HaveData(const string &vKey) = 0;

};

#endif //PERSIST_CDP_CACHE_H