// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDPDB_H
#define PERSIST_CDPDB_H

#include "commons/uint256.h"
#include "entities/cdp.h"
#include "dbaccess.h"

#include <map>
#include <set>
#include <string>
#include <cstdint>

using namespace std;

class CCdpMemCache {
public:
    CCdpMemCache() {}
    CCdpMemCache(CCdpMemCache *pBaseIn) : pBase(pBaseIn) {}
    // Only apply to construct the global mem-cache.
    CCdpMemCache(CDBAccess *pAccessIn) : pAccess(pAccessIn) {}

    bool LoadAllCdpFromDB();
    void SetBase(CCdpMemCache *pBaseIn);
    void Flush();

    // Usage: before modification, erase the old cdp; after modification, save the new cdp.
    bool SaveCdp(const CUserCDP &userCdp);
    bool EraseCdp(const CUserCDP &userCdp);
    bool HaveCdp(const CUserCDP &userCdp);

    bool GetCdpListByCollateralRatio(const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
                                     set<CUserCDP> &userCdps);

    uint64_t GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const;
    uint64_t GetGlobalCollateral() const;

private:
    bool GetCdpList(const double ratio, set<CUserCDP> &expiredCdps, set<CUserCDP> &userCdps);
    bool GetCdpList(const double ratio, set<CUserCDP> &userCdps);

    void BatchWrite(const map<CUserCDP, uint8_t> &cdpsIn);

private:
    enum CDPState { CDP_EXPIRED = 0, CDP_VALID = 1 };

private:
    map<CUserCDP, uint8_t> cdps;  // map: CUserCDP -> flag(0: valid; 1: invalid)
    uint64_t total_staked_bcoins = 0;
    uint64_t total_owed_scoins   = 0;
    CCdpMemCache *pBase          = nullptr;
    CDBAccess *pAccess           = nullptr;
};

class CCdpDBCache {
public:
    CCdpDBCache() {}
    CCdpDBCache(CDBAccess *pDbAccess) : cdpCache(pDbAccess), regId2CdpCache(pDbAccess), cdpMemCache(pDbAccess) {}
    CCdpDBCache(CCdpDBCache *pBaseIn)
        : cdpCache(pBaseIn->cdpCache), regId2CdpCache(pBaseIn->regId2CdpCache), cdpMemCache(pBaseIn->cdpMemCache) {}


    bool NewCdp(const int32_t blockHeight, CUserCDP &cdp);

    bool StakeBcoinsToCdp(const int32_t blockHeight, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                          CUserCDP &cdp);
    bool RedeemBcoinsFromCdp(const int32_t blockHeight, const uint64_t bcoinsToRedeem, const uint64_t scoinsToRepay,
                             CUserCDP &cdp);

    bool GetCdpList(const CRegID &regId, vector<CUserCDP> &cdpList);

    bool GetCdp(const uint256 cdpid, CUserCDP &cdp);
    bool SaveCdp(CUserCDP &cdp);
    bool EraseCdp(const CUserCDP &cdp);

    bool CheckGlobalCollateralRatioFloorReached(const uint64_t bcoinMedianPrice,
                                                const uint64_t globalCollateralRatioLimit);
    bool CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake,
                                             const uint64_t globalCollateralCeiling);
    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CCdpDBCache *pBaseIn) {
        cdpCache.SetBase(&pBaseIn->cdpCache);
        regId2CdpCache.SetBase(&pBaseIn->regId2CdpCache);
        cdpMemCache.SetBase(&pBaseIn->cdpMemCache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        cdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return cdpCache.UndoDatas();
    }

private:
    // Attention: changedBcoins/changedScoins could be positive or negative values.
    bool UpdateCdp(const int32_t blockHeight, int64_t changedBcoins, const int64_t changedScoins, CUserCDP &cdp);

private:
/*  CCompositeKVCache     prefixType     key              value             variable  */
/*  ----------------   --------------   ------------   --------------    -------------*/
    // cdp$CTxID -> CUserCDP
    CCompositeKVCache< dbk::CDP,         uint256,       CUserCDP >       cdpCache;
    // rcdp${CRegID} -> set<CTxID>
    CCompositeKVCache< dbk::REGID_CDP,   string,        set<uint256>>    regId2CdpCache;

public:
    // Memory only cache
    CCdpMemCache cdpMemCache;
};

#endif  // PERSIST_CDPDB_H
