// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"
#include "persistence/dbiterator.h"

CCdpDBCache::CCdpDBCache(CDBAccess *pDbAccess)
    : cdpGlobalDataCache(pDbAccess),
      cdpCache(pDbAccess),
      bcoinStatusCache(pDbAccess),
      userCdpCache(pDbAccess),
      cdpRatioIndexCache(pDbAccess),
      cdp_height_index_cache(pDbAccess) {}

CCdpDBCache::CCdpDBCache(CCdpDBCache *pBaseIn)
    : cdpGlobalDataCache(pBaseIn->cdpGlobalDataCache),
      cdpCache(pBaseIn->cdpCache),
      bcoinStatusCache(pBaseIn->bcoinStatusCache),
      userCdpCache(pBaseIn->userCdpCache),
      cdpRatioIndexCache(pBaseIn->cdpRatioIndexCache),
      cdp_height_index_cache(pBaseIn->cdp_height_index_cache) {}

bool CCdpDBCache::NewCDP(const int32_t blockHeight, CUserCDP &cdp) {
    assert(!cdpCache.HasData(cdp.cdpid));
    assert(!userCdpCache.HasData(make_pair(CRegIDKey(cdp.owner_regid), cdp.GetCoinPair())));

    return cdpCache.SetData(cdp.cdpid, cdp) &&
        userCdpCache.SetData(make_pair(CRegIDKey(cdp.owner_regid), cdp.GetCoinPair()), cdp.cdpid) &&
        SaveCdpIndexData(cdp);
}

bool CCdpDBCache::EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp) {
    return cdpCache.SetData(cdp.cdpid, cdp) &&
        userCdpCache.EraseData(make_pair(CRegIDKey(cdp.owner_regid), cdp.GetCoinPair())) &&
        EraseCdpIndexData(oldCDP);
}

// Need to delete the old cdp(before updating cdp), then save the new cdp if necessary.
bool CCdpDBCache::UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP) {
    assert(!newCDP.IsEmpty());
    return cdpCache.SetData(newCDP.cdpid, newCDP) && EraseCdpIndexData(oldCDP) && SaveCdpIndexData(newCDP);
}

bool CCdpDBCache::UserHaveCdp(const CRegID &regid, const TokenSymbol &assetSymbol, const TokenSymbol &scoinSymbol) {
    return userCdpCache.HasData(make_pair(CRegIDKey(regid), CCdpCoinPair(assetSymbol, scoinSymbol)));
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

bool CCdpDBCache::SaveCdpIndexData(const CUserCDP &userCdp) {
    CCdpCoinPair cdpCoinPair = userCdp.GetCoinPair();
    CCdpGlobalData cdpGlobalData = GetCdpGlobalData(cdpCoinPair);

    cdpGlobalData.total_staked_assets += userCdp.total_staked_bcoins;
    cdpGlobalData.total_owed_scoins += userCdp.total_owed_scoins;

    if (!cdpGlobalDataCache.SetData(cdpCoinPair, cdpGlobalData))
        return false;

    if (!cdpRatioIndexCache.SetData(MakeCdpRatioIndexKey(userCdp), userCdp))
        return false;

    if (!cdp_height_index_cache.SetData(MakeCdpHeightIndexKey(userCdp), userCdp))
        return false;
    return true;
}

bool CCdpDBCache::EraseCdpIndexData(const CUserCDP &userCdp) {
    CCdpCoinPair cdpCoinPair = userCdp.GetCoinPair();
    CCdpGlobalData cdpGlobalData = GetCdpGlobalData(cdpCoinPair);

    cdpGlobalData.total_staked_assets -= userCdp.total_staked_bcoins;
    cdpGlobalData.total_owed_scoins -= userCdp.total_owed_scoins;

    cdpGlobalDataCache.SetData(cdpCoinPair, cdpGlobalData);

    if (!cdpRatioIndexCache.EraseData(MakeCdpRatioIndexKey(userCdp)))
        return false;

    if (!cdp_height_index_cache.EraseData(MakeCdpHeightIndexKey(userCdp)))
        return false;
    return true;
}

bool CCdpDBCache::GetCdpListByCollateralRatio(const CCdpCoinPair &cdpCoinPair,
        const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
        CCdpRatioIndexCache::Map &userCdps) {
    double ratio = (double(collateralRatio) / RATIO_BOOST) / (double(bcoinMedianPrice) / PRICE_BOOST);
    assert(uint64_t(ratio * CDP_BASE_RATIO_BOOST) < UINT64_MAX);
    uint64_t ratioBoost = uint64_t(ratio * CDP_BASE_RATIO_BOOST) + 1;
    CCdpRatioIndexCache::KeyType endKey(cdpCoinPair, ratioBoost, 0, uint256());

    return cdpRatioIndexCache.GetAllElements(endKey, userCdps);
}

CCdpGlobalData CCdpDBCache::GetCdpGlobalData(const CCdpCoinPair &cdpCoinPair) const {
    CCdpGlobalData ret;
    cdpGlobalDataCache.GetData(cdpCoinPair, ret);
    return ret;
}

bool CCdpDBCache::GetBcoinStatus(const TokenSymbol &bcoinSymbol, CdpBcoinStatus &activation) {
    if (kCdpBcoinSymbolSet.count(bcoinSymbol) > 0) {
        activation = CdpBcoinStatus::STAKE_ON;
        return true;
    }
    if (bcoinSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(bcoinSymbol) > 0) {
        activation = CdpBcoinStatus::NONE;
        return false;
    }
    uint8_t act;
    if (!bcoinStatusCache.GetData(bcoinSymbol, act)) return false;
    activation = CdpBcoinStatus(act);
    return true;
}

bool CCdpDBCache::IsBcoinActivated(const TokenSymbol &bcoinSymbol) {
    if (kCdpBcoinSymbolSet.count(bcoinSymbol) > 0) return true;
    if (bcoinSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(bcoinSymbol) > 0) return false;
    return bcoinStatusCache.HasData(bcoinSymbol);
}

bool CCdpDBCache::SetBcoinStatus(const TokenSymbol &bcoinSymbol, const CdpBcoinStatus &activation) {
    return bcoinStatusCache.SetData(bcoinSymbol, (uint8_t)activation);
}

void CCdpDBCache::SetBaseViewPtr(CCdpDBCache *pBaseIn) {
    cdpGlobalDataCache.SetBase(&pBaseIn->cdpGlobalDataCache);
    cdpCache.SetBase(&pBaseIn->cdpCache);
    bcoinStatusCache.SetBase(&pBaseIn->bcoinStatusCache);
    userCdpCache.SetBase(&pBaseIn->userCdpCache);
    cdpRatioIndexCache.SetBase(&pBaseIn->cdpRatioIndexCache);
    cdp_height_index_cache.SetBase(&pBaseIn->cdp_height_index_cache);
}

void CCdpDBCache::SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
    cdpGlobalDataCache.SetDbOpLogMap(pDbOpLogMapIn);
    cdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    bcoinStatusCache.SetDbOpLogMap(pDbOpLogMapIn);
    userCdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    cdpRatioIndexCache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_height_index_cache.SetDbOpLogMap(pDbOpLogMapIn);
}

uint32_t CCdpDBCache::GetCacheSize() const {
    return cdpGlobalDataCache.GetCacheSize() + cdpCache.GetCacheSize() + bcoinStatusCache.GetCacheSize() +
            userCdpCache.GetCacheSize() + cdpRatioIndexCache.GetCacheSize() + cdp_height_index_cache.GetCacheSize();
}

bool CCdpDBCache::Flush() {
    cdpGlobalDataCache.Flush();
    cdpCache.Flush();
    bcoinStatusCache.Flush();
    userCdpCache.Flush();
    cdpRatioIndexCache.Flush();
    cdpRatioIndexCache.Flush();

    return true;
}

CCdpRatioIndexCache::KeyType CCdpDBCache::MakeCdpRatioIndexKey(const CUserCDP &cdp) {

    uint64_t boostedRatio = cdp.collateral_ratio_base * CDP_BASE_RATIO_BOOST;
    uint64_t ratio        = (boostedRatio < cdp.collateral_ratio_base /* overflown */) ? UINT64_MAX : boostedRatio;
    return { cdp.GetCoinPair(), CFixedUInt64(ratio), CFixedUInt64(cdp.block_height), cdp.cdpid };
}

CCdpHeightIndexCache::KeyType CCdpDBCache::MakeCdpHeightIndexKey(const CUserCDP &cdp) {

    return {cdp.GetCoinPair(), CFixedUInt64(cdp.block_height), cdp.cdpid};
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