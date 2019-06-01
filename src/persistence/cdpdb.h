// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDP_CACHE_H
#define PERSIST_CDP_CACHE_H

#include "leveldbwrapper.h"

//stake in BaseCoin to get StableCoins
struct CdpStakeOp {
    uint64_t blockHeight;
    uint64_t stakedBcoins;
    uint64_t mintedScoins;
};

/**
 *  Persisted in LDB as:
 *      cdp$RegId --> { owedScoins, list_of{blockHeight, stakedBcoins, mintedBcoins} }
 *
 */
struct CUserCdp {
    CRegID ownerRegId;              // CDP Owner RegId
    uint64_t totalStakedBcoins;     // summed from stakeOps
    uint64_t totalMintedScoins;     // summed from stakeOps
    uint64_t owedScoins;            // owed = total minted - total redempted
    vector<CdpStakeOp> stakeOps;
};


class ICdpView {
public:
    virtual bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) = 0;
    virtual bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) = 0;
    virtual bool BatchWrite(const map<string, vector<unsigned char> > &mapCdps) = 0;
    virtual bool EraseKey(const vector<unsigned char> &vKey) = 0;
    virtual bool HaveData(const vector<unsigned char> &vKey) = 0;

    virtual ~ICdpView(){};
};

class CCdpCache: public ICdpView  {
public:
    bool SetStakeBcoins(CUserID txUid, uint64_t bcoinsToStake, int blockHeight, CDBOpLog &cdpDbOpLog);
    bool GetLiquidityCdpItems(vector<CdpItem> & cdpItems);

public:
    virtual bool GetData(const string key, vector<unsigned char> &vValue);
    virtual bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue);
    virtual bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb);
    virtual bool EraseKey(const vector<unsigned char> &vKey);
    virtual bool HaveData(const vector<unsigned char> &vKey);

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
    virtual bool GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) = 0;
    virtual bool SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) = 0;
    virtual bool BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb) = 0;
    virtual bool EraseKey(const vector<unsigned char> &vKey) = 0;
    virtual bool HaveData(const vector<unsigned char> &vKey) = 0;

};

#endif //PERSIST_CDP_CACHE_H