// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"
#include "persistence/dbiterator.h"

CCdpDBCache::CCdpDBCache(CDBAccess *pDbAccess)
    : globalStakedBcoinsCache(pDbAccess),
      globalOwedScoinsCache(pDbAccess),
      cdpCache(pDbAccess),
      userCdpCache(pDbAccess),
      ratioCDPIdCache(pDbAccess) {}

CCdpDBCache::CCdpDBCache(CCdpDBCache *pBaseIn)
    : globalStakedBcoinsCache(pBaseIn->globalStakedBcoinsCache),
      globalOwedScoinsCache(pBaseIn->globalOwedScoinsCache),
      cdpCache(pBaseIn->cdpCache),
      userCdpCache(pBaseIn->userCdpCache),
      ratioCDPIdCache(pBaseIn->ratioCDPIdCache) {}

bool CCdpDBCache::NewCDP(const int32_t blockHeight, CUserCDP &cdp) {
    assert(!cdpCache.HaveData(cdp.cdpid));
    assert(!userCdpCache.HaveData(make_tuple(CRegIDKey(cdp.owner_regid), cdp.bcoin_symbol, cdp.scoin_symbol)));

    return cdpCache.SetData(cdp.cdpid, cdp) &&
        userCdpCache.SetData(make_tuple(CRegIDKey(cdp.owner_regid), cdp.bcoin_symbol, cdp.scoin_symbol), cdp.cdpid) &&
        SaveCDPToRatioDB(cdp);
}

bool CCdpDBCache::EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp) {
    return cdpCache.SetData(cdp.cdpid, cdp) &&
        userCdpCache.EraseData(make_tuple(CRegIDKey(cdp.owner_regid), cdp.bcoin_symbol, cdp.scoin_symbol)) &&
        EraseCDPFromRatioDB(oldCDP);
}

// Need to delete the old cdp(before updating cdp), then save the new cdp if necessary.
bool CCdpDBCache::UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP) {
    assert(!newCDP.IsEmpty());
    return cdpCache.SetData(newCDP.cdpid, newCDP) && EraseCDPFromRatioDB(oldCDP) && SaveCDPToRatioDB(newCDP);
}

bool CCdpDBCache::UserHaveCdp(const CRegID &regid, const TokenSymbol &assetSymbol, const TokenSymbol &scoinSymbol) {
    return userCdpCache.HaveData(make_tuple(regid, assetSymbol, scoinSymbol));
}

bool CCdpDBCache::GetCDPList(const CRegID &regid, vector<CUserCDP> &cdpList) {

    CRegIDKey prefixKey(regid);
    CDBPrefixIterator<decltype(userCdpCache), CRegIDKey> dbIt(userCdpCache, prefixKey);
    dbIt.First();
    for(dbIt.First(); dbIt.IsValid(); dbIt.Next()) {
        CUserCDP userCdp;
        if (!cdpCache.GetData(dbIt.GetValue().value(), userCdp)) {
            return false; // has invalid data
        }

        cdpList.push_back(userCdp);
    }

    return true;
}

bool CCdpDBCache::GetCDP(const uint256 cdpid, CUserCDP &cdp) {
    return cdpCache.GetData(cdpid, cdp);
}

// Attention: update cdpCache and userCdpCache synchronously.
bool CCdpDBCache::SaveCDPToDB(const CUserCDP &cdp) {
    return cdpCache.SetData(cdp.cdpid, cdp);
}

bool CCdpDBCache::EraseCDPFromDB(const CUserCDP &cdp) {
    return cdpCache.EraseData(cdp.cdpid);
}

bool CCdpDBCache::SaveCDPToRatioDB(const CUserCDP &userCdp) {
    uint64_t globalStakedBcoins = GetGlobalStakedBcoins();
    uint64_t globalOwedScoins   = GetGlobalOwedScoins();

    globalStakedBcoins += userCdp.total_staked_bcoins;
    globalOwedScoins += userCdp.total_owed_scoins;

    globalStakedBcoinsCache.SetData(globalStakedBcoins);
    globalOwedScoinsCache.SetData(globalOwedScoins);

    // cdpr{Ratio}{cdpid} -> CUserCDP
    uint64_t boostedRatio = userCdp.collateral_ratio_base * CDP_BASE_RATIO_BOOST;
    uint64_t ratio        = (boostedRatio < userCdp.collateral_ratio_base /* overflown */) ? UINT64_MAX : boostedRatio;
    string strRatio       = strprintf("%016x", ratio);
    string heightStr      = strprintf("%016x", userCdp.block_height);
    RatioCDPIdCache::KeyType key(strRatio, heightStr, userCdp.cdpid);

    ratioCDPIdCache.SetData(key, userCdp);

    return true;
}

bool CCdpDBCache::EraseCDPFromRatioDB(const CUserCDP &userCdp) {
    uint64_t globalStakedBcoins = GetGlobalStakedBcoins();
    uint64_t globalOwedScoins   = GetGlobalOwedScoins();

    globalStakedBcoins -= userCdp.total_staked_bcoins;
    globalOwedScoins -= userCdp.total_owed_scoins;

    globalStakedBcoinsCache.SetData(globalStakedBcoins);
    globalOwedScoinsCache.SetData(globalOwedScoins);

    uint64_t boostedRatio = userCdp.collateral_ratio_base * CDP_BASE_RATIO_BOOST;
    uint64_t ratio        = (boostedRatio < userCdp.collateral_ratio_base /* overflown */) ? UINT64_MAX : boostedRatio;
    string strRatio       = strprintf("%016x", ratio);
    string heightStr      = strprintf("%016x", userCdp.block_height);
    RatioCDPIdCache::KeyType key(strRatio, heightStr, userCdp.cdpid);

    ratioCDPIdCache.EraseData(key);

    return true;
}

// global collateral ratio floor check
bool CCdpDBCache::CheckGlobalCollateralRatioFloorReached(const uint64_t bcoinMedianPrice,
                                                         const uint64_t globalCollateralRatioLimit) {
    return GetGlobalCollateralRatio(bcoinMedianPrice) < globalCollateralRatioLimit;
}

// global collateral amount ceiling check
bool CCdpDBCache::CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake,
                                                      const uint64_t globalCollateralCeiling) {
    return (newBcoinsToStake + GetGlobalStakedBcoins()) > globalCollateralCeiling * COIN;
}

bool CCdpDBCache::GetCdpListByCollateralRatio(const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
                                              RatioCDPIdCache::Map &userCdps) {
    double ratio = (double(collateralRatio) / RATIO_BOOST) / (double(bcoinMedianPrice) / PRICE_BOOST);
    assert(uint64_t(ratio * CDP_BASE_RATIO_BOOST) < UINT64_MAX);
    uint64_t ratioBoost = uint64_t(ratio * CDP_BASE_RATIO_BOOST) + 1;
    string strRatio = strprintf("%016x", ratioBoost);
    string heightStr      = strprintf("%016x", 0);
    RatioCDPIdCache::KeyType endKey(strRatio, heightStr, uint256());

    return ratioCDPIdCache.GetAllElements(endKey, userCdps);
}

uint64_t CCdpDBCache::GetGlobalStakedBcoins() const {
    uint64_t globalStakedBcoins = 0;
    globalStakedBcoinsCache.GetData(globalStakedBcoins);

    return globalStakedBcoins;
}

inline uint64_t CCdpDBCache::GetGlobalOwedScoins() const {
    uint64_t globalOwedScoins = 0;
    globalOwedScoinsCache.GetData(globalOwedScoins);

    return globalOwedScoins;
}

void CCdpDBCache::GetGlobalItem(uint64_t &globalStakedBcoins, uint64_t &globalOwedScoins) const {
    globalStakedBcoinsCache.GetData(globalStakedBcoins);
    globalOwedScoinsCache.GetData(globalOwedScoins);
}

uint64_t CCdpDBCache::GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const {
    // If total owed scoins equal to zero, the global collateral ratio becomes infinite.
    uint64_t globalOwedScoins = GetGlobalOwedScoins();
    if (globalOwedScoins == 0) {
        return UINT64_MAX;
    }

    uint64_t globalStakedBcoins = GetGlobalStakedBcoins();

    return double(globalStakedBcoins) * bcoinMedianPrice / PRICE_BOOST / globalOwedScoins * RATIO_BOOST;
}

void CCdpDBCache::SetBaseViewPtr(CCdpDBCache *pBaseIn) {
    globalStakedBcoinsCache.SetBase(&pBaseIn->globalStakedBcoinsCache);
    globalOwedScoinsCache.SetBase(&pBaseIn->globalOwedScoinsCache);
    cdpCache.SetBase(&pBaseIn->cdpCache);
    userCdpCache.SetBase(&pBaseIn->userCdpCache);
    ratioCDPIdCache.SetBase(&pBaseIn->ratioCDPIdCache);
}

void CCdpDBCache::SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
    globalStakedBcoinsCache.SetDbOpLogMap(pDbOpLogMapIn);
    globalOwedScoinsCache.SetDbOpLogMap(pDbOpLogMapIn);
    cdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    userCdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    ratioCDPIdCache.SetDbOpLogMap(pDbOpLogMapIn);
}

uint32_t CCdpDBCache::GetCacheSize() const {
    return globalStakedBcoinsCache.GetCacheSize() + globalOwedScoinsCache.GetCacheSize() + cdpCache.GetCacheSize() +
           userCdpCache.GetCacheSize() + ratioCDPIdCache.GetCacheSize();
}

bool CCdpDBCache::Flush() {
    globalStakedBcoinsCache.Flush();
    globalOwedScoinsCache.Flush();
    cdpCache.Flush();
    userCdpCache.Flush();
    ratioCDPIdCache.Flush();

    return true;
}

string GetCdpCloseTypeName(const CDPCloseType type) {
    switch (type) {
        case CDPCloseType:: BY_REDEEM:
            return "redeem" ;
        case CDPCloseType:: BY_FORCE_LIQUIDATE :
            return "force_liquidate" ;
        case CDPCloseType ::BY_MANUAL_LIQUIDATE:
            return "manual_liquidate" ;
    }
    return "undefined" ;
}