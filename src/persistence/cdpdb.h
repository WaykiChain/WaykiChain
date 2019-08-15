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

class CCDPMemCache {
public:
    CCDPMemCache() {}
    CCDPMemCache(CCDPMemCache *pBaseIn) : pBase(pBaseIn) {
        SetGlobalItem(pBaseIn->global_staked_bcoins, pBaseIn->global_owed_scoins);
    }
    // Only apply to construct the global mem-cache.
    CCDPMemCache(CDBAccess *pAccessIn) : pAccess(pAccessIn) { LoadAllCDPFromDB(); }

    void SetBase(CCDPMemCache *pBaseIn);
    void Flush();

    // Usage: before modification, erase the old cdp; after modification, save the new cdp.
    bool SaveCDP(const CUserCDP &userCdp);
    bool EraseCDP(const CUserCDP &userCdp);
    bool HaveCDP(const CUserCDP &userCdp);

    bool GetCdpListByCollateralRatio(const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
                                     set<CUserCDP> &userCdps);

    uint64_t GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const;
    uint64_t GetGlobalCollateral() const;
    void GetGlobalItem(uint64_t &globalStakedBcoins, uint64_t &globalOwedScoins);

private:
    bool GetCDPList(const double ratio, set<CUserCDP> &expiredCdps, set<CUserCDP> &userCdps);
    bool GetCDPList(const double ratio, set<CUserCDP> &userCdps);

    void BatchWrite(const map<CUserCDP, uint8_t> &cdpsIn);
    void SetGlobalItem(const uint64_t globalStakedBcoins, const uint64_t globalOwedScoins);
    bool LoadAllCDPFromDB();

private:
    enum CDPState { CDP_EXPIRED = 0, CDP_VALID = 1 };

private:
    map<CUserCDP, uint8_t /* CDPState */> cdps;
    uint64_t global_staked_bcoins = 0;
    uint64_t global_owed_scoins   = 0;
    CCDPMemCache *pBase           = nullptr;
    CDBAccess *pAccess            = nullptr;
};

class CCDPDBCache {
public:
    CCDPDBCache() {}
    CCDPDBCache(CDBAccess *pDbAccess) : cdpCache(pDbAccess), regId2CDPCache(pDbAccess), cdpMemCache(pDbAccess) {}
    CCDPDBCache(CCDPDBCache *pBaseIn)
        : cdpCache(pBaseIn->cdpCache), regId2CDPCache(pBaseIn->regId2CDPCache), cdpMemCache(pBaseIn->cdpMemCache) {}


    bool NewCDP(const int32_t blockHeight, CUserCDP &cdp);
    bool UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP);

    bool GetCDPList(const CRegID &regId, vector<CUserCDP> &cdpList);

    bool GetCDP(const uint256 cdpid, CUserCDP &cdp);
    bool EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp);

    bool CheckGlobalCollateralRatioFloorReached(const uint64_t bcoinMedianPrice,
                                                const uint64_t globalCollateralRatioLimit);
    bool CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake,
                                             const uint64_t globalCollateralCeiling);
    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CCDPDBCache *pBaseIn) {
        cdpCache.SetBase(&pBaseIn->cdpCache);
        regId2CDPCache.SetBase(&pBaseIn->regId2CDPCache);
        cdpMemCache.SetBase(&pBaseIn->cdpMemCache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        cdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return cdpCache.UndoDatas();
    }

private:
    bool SaveCDPToDB(const CUserCDP &cdp);
    bool EraseCDPFromDB(const CUserCDP &cdp);
private:
/*  CCompositeKVCache     prefixType     key              value             variable  */
/*  ----------------   --------------   ------------   --------------    -------------*/
    // cdp$CTxID -> CUserCDP
    CCompositeKVCache< dbk::CDP,         uint256,       CUserCDP >       cdpCache;
    // rcdp${CRegID} -> set<CTxID>
    CCompositeKVCache< dbk::REGID_CDP,   string,        set<uint256>>    regId2CDPCache;

public:
    // Memory only cache
    CCDPMemCache cdpMemCache;
};

#endif  // PERSIST_CDPDB_H
