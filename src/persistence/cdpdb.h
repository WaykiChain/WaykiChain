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

class CCDPDBCache {
public:
    CCDPDBCache() {}
    CCDPDBCache(CDBAccess *pDbAccess);
    CCDPDBCache(CCDPDBCache *pBaseIn);

    bool NewCDP(const int32_t blockHeight, CUserCDP &cdp);
    bool EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp);
    bool UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP);

    bool GetCDPList(const CRegID &regId, vector<CUserCDP> &cdpList);
    bool GetCDP(const uint256 cdpid, CUserCDP &cdp);

    bool GetCdpListByCollateralRatio(const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
                                     set<CUserCDP> &userCdps);

    inline uint64_t GetGlobalStakedBcoins() const;
    inline uint64_t GetGlobalOwedScoins() const;
    void GetGlobalItem(uint64_t &globalStakedBcoins, uint64_t &globalOwedScoins) const;
    uint64_t GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const;

    bool CheckGlobalCollateralRatioFloorReached(const uint64_t bcoinMedianPrice,
                                                const uint64_t globalCollateralRatioLimit);
    bool CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake, const uint64_t globalCollateralCeiling);

    void SetBaseViewPtr(CCDPDBCache *pBaseIn);
    void SetDbOpLogMap(CDBOpLogMap * pDbOpLogMapIn);
    bool UndoDatas();
    uint32_t GetCacheSize() const;
    bool Flush();

private:
    bool SaveCDPToDB(const CUserCDP &cdp);
    bool EraseCDPFromDB(const CUserCDP &cdp);

    // Usage: before modification, erase the old cdp; after modification, save the new cdp.
    bool SaveCDPToRatioDB(const CUserCDP &userCdp);
    bool EraseCDPFromRatioDB(const CUserCDP &userCdp);

private:
    /*  CSimpleKVCache          prefixType                     value               variable           */
    /*  -------------------- --------------------           -------------       --------------------- */
    CSimpleKVCache<         dbk::CDP_GLOBAL_STAKED_BCOINS,   uint64_t>      globalStakedBcoinsCache;
    CSimpleKVCache<         dbk::CDP_GLOBAL_OWED_SCOINS,     uint64_t>      globalOwedScoinsCache;

    /*  CCompositeKVCache     prefixType     key                            value             variable  */
    /*  ----------------   --------------   ------------                --------------    ----- --------*/
    // cdp{$cdpid} -> CUserCDP
    CCompositeKVCache<      dbk::CDP,       uint256,                    CUserCDP>           cdpCache;
    // rcdp${CRegID} -> set<cdpid>
    CCompositeKVCache<      dbk::REGID_CDP, string,                     set<uint256>>       regId2CDPCache;
    // cdpr{Ratio}{$cdpid} -> CUserCDP
    CCompositeKVCache<      dbk::CDP_RATIO, std::pair<string, uint256>, CUserCDP>            ratioCDPIdCache;
};

enum CDPCloseType: uint8_t {
    BY_REDEEM = 0,
    BY_MAN_LIQUIDATE,
    BY_FORCE_LIQUIDATE
};

class CClosedCdpDBCache {
public:
    CClosedCdpDBCache() {}
    CClosedCdpDBCache(CDBAccess *pDbAccess) : closedCdpCache(pDbAccess) {};
    CClosedCdpDBCache(CClosedCdpDBCache *pBaseIn) : closedCdpCache(pBaseIn->closedCdpCache) {};

public:
    bool AddClosedCdp(const string& cdpId, CDPCloseType& closeType) {
        return closedCdpCache.SetData(cdpId, closeType);
    }

    bool UndoClosedCdp(const string& cdpId) {
        return closedCdpCache.EraseData(cdpId);
    }

    bool GetClosedCdpById(const string& cdpId, CDPCloseType& closeType) {
        return closedCdpCache.GetData(cdpId, (uint8_t&) closeType);
    }

private:
  // ccdp{$cdpid} -> CDPCloseType enum
    CCompositeKVCache< dbk::CDP_CLOSED, string, uint8_t>  closedCdpCache;
};

#endif  // PERSIST_CDPDB_H
