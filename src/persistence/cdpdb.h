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

//stake in BaseCoin to get StableCoins
struct CdpStakeOp {
    uint64_t blockHeight;
    uint64_t stakedBcoins;
    uint64_t mintedScoins;
};

//TODO: CdpItem
class CdpItem {};

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

    bool IsEmpty() const {
        return ownerRegId.IsEmpty(); // TODO: need to check empty for other fields?
    }
};


class ICdpView {
public:
    virtual bool GetData(const string &vKey, CUserCdp &vValue) = 0;
    virtual bool SetData(const string &vKey, const CUserCdp &vValue) = 0;
    virtual bool BatchWrite(const map<string, CUserCdp> &mapCdps) = 0;
    virtual bool EraseKey(const string &vKey) = 0;
    virtual bool HaveData(const string &vKey) = 0;

    virtual ~ICdpView(){};
};

class CCdpCache: public ICdpView  {
public:
    CCdpCache() {};
    CCdpCache(ICdpView &base): ICdpView(), pBase(&base) {};

    bool SetStakeBcoins(uint64_t bcoinsToStake, int blockHeight, CDBOpLog &cdpDbOpLog);
    bool GetLiquidityCdpItems(vector<CdpItem> & cdpItems);

public:
    virtual bool GetData(const string &key, CUserCdp &vValue);
    virtual bool SetData(const string &vKey, const CUserCdp &vValue);
    virtual bool BatchWrite(const map<string, CUserCdp > &mapContractDb);
    virtual bool EraseKey(const string &vKey);
    virtual bool HaveData(const string &vKey);

    bool GetCdpData();
    bool SetCdpData();

private:
    ICdpView *pBase               = nullptr;
    uint64_t collateralRatio      = 0;
    uint64_t liquidityPrice       = 0;
    uint64_t forcedLiquidityPrice = 0;
    uint64_t bcoinMedianPrice     = 0;
    map<string, CUserCdp> mapCdps;  // UserRegId --> UserCDP
};

class CCdpDb: public ICdpView  {
public:

    virtual bool GetData(const string &vKey, CUserCdp &vValue) = 0;
    virtual bool SetData(const string &vKey, const CUserCdp &vValue) = 0;
    virtual bool BatchWrite(const map<string, CUserCdp > &mapContractDb) = 0;
    virtual bool EraseKey(const string &vKey) = 0;
    virtual bool HaveData(const string &vKey) = 0;

};

#endif //PERSIST_CDP_CACHE_H