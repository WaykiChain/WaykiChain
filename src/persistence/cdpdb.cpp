// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"
#include "persistence/dbiterator.h"

CCdpDBCache::CCdpDBCache(CDBAccess *pDbAccess)
    : cdp_global_data_cache(pDbAccess),
      cdp_cache(pDbAccess),
      cdp_bcoin_cache(pDbAccess),
      user_cdp_cache(pDbAccess),
      cdp_ratio_index_cache(pDbAccess),
      cdp_height_index_cache(pDbAccess) {}

CCdpDBCache::CCdpDBCache(CCdpDBCache *pBaseIn)
    : cdp_global_data_cache(pBaseIn->cdp_global_data_cache),
      cdp_cache(pBaseIn->cdp_cache),
      cdp_bcoin_cache(pBaseIn->cdp_bcoin_cache),
      user_cdp_cache(pBaseIn->user_cdp_cache),
      cdp_ratio_index_cache(pBaseIn->cdp_ratio_index_cache),
      cdp_height_index_cache(pBaseIn->cdp_height_index_cache) {}

bool CCdpDBCache::NewCDP(const int32_t blockHeight, CUserCDP &cdp) {
    return cdp_cache.SetData(cdp.cdpid, cdp) &&
        user_cdp_cache.SetData(make_pair(CRegIDKey(cdp.owner_regid), cdp.GetCoinPair()), cdp.cdpid) &&
        SaveCdpIndexData(cdp);
}

bool CCdpDBCache::EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp) {
    return cdp_cache.EraseData(cdp.cdpid) &&
        user_cdp_cache.EraseData(make_pair(CRegIDKey(cdp.owner_regid), cdp.GetCoinPair())) &&
        EraseCdpIndexData(oldCDP);
}

// Need to delete the old cdp(before updating cdp), then save the new cdp if necessary.
bool CCdpDBCache::UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP) {
    assert(!newCDP.IsEmpty());
    return cdp_cache.SetData(newCDP.cdpid, newCDP) && EraseCdpIndexData(oldCDP) && SaveCdpIndexData(newCDP);
}

bool CCdpDBCache::UserHaveCdp(const CRegID &regid, const TokenSymbol &assetSymbol, const TokenSymbol &scoinSymbol) {
    return user_cdp_cache.HasData(make_pair(CRegIDKey(regid), CCdpCoinPair(assetSymbol, scoinSymbol)));
}

bool CCdpDBCache::GetCDPList(const CRegID &regid, vector<CUserCDP> &cdpList) {

    CRegIDKey prefixKey(regid);
    CDBPrefixIterator<decltype(user_cdp_cache), CRegIDKey> dbIt(user_cdp_cache, prefixKey);
    dbIt.First();
    for(dbIt.First(); dbIt.IsValid(); dbIt.Next()) {
        CUserCDP userCdp;
        if (!cdp_cache.GetData(dbIt.GetValue().value(), userCdp)) {
            return false; // has invalid data
        }

        cdpList.push_back(userCdp);
    }

    return true;
}

bool CCdpDBCache::GetCDP(const uint256 cdpid, CUserCDP &cdp) {
    return cdp_cache.GetData(cdpid, cdp);
}

// Attention: update cdp_cache and user_cdp_cache synchronously.
bool CCdpDBCache::SaveCDPToDB(const CUserCDP &cdp) {
    return cdp_cache.SetData(cdp.cdpid, cdp);
}

bool CCdpDBCache::EraseCDPFromDB(const CUserCDP &cdp) {
    return cdp_cache.EraseData(cdp.cdpid);
}

bool CCdpDBCache::SaveCdpIndexData(const CUserCDP &userCdp) {
    CCdpCoinPair cdpCoinPair = userCdp.GetCoinPair();
    CCdpGlobalData cdpGlobalData = GetCdpGlobalData(cdpCoinPair);

    cdpGlobalData.total_staked_assets += userCdp.total_staked_bcoins;
    cdpGlobalData.total_owed_scoins += userCdp.total_owed_scoins;

    if (!cdp_global_data_cache.SetData(cdpCoinPair, cdpGlobalData))
        return false;

    if (!cdp_ratio_index_cache.SetData(MakeCdpRatioIndexKey(userCdp), userCdp))
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

    cdp_global_data_cache.SetData(cdpCoinPair, cdpGlobalData);

    if (!cdp_ratio_index_cache.EraseData(MakeCdpRatioIndexKey(userCdp)))
        return false;

    if (!cdp_height_index_cache.EraseData(MakeCdpHeightIndexKey(userCdp)))
        return false;
    return true;
}

list<CUserCDP> CCdpDBCache::GetCdpListByCollateralRatio(const CCdpCoinPair &cdpCoinPair,
        const uint64_t collateralRatio, const uint64_t bcoinMedianPrice) {

    double ratio = (double(collateralRatio) / RATIO_BOOST) / (double(bcoinMedianPrice) / PRICE_BOOST);
    assert(uint64_t(ratio * CDP_BASE_RATIO_BOOST) < UINT64_MAX);
    uint64_t ratioBoost = uint64_t(ratio * CDP_BASE_RATIO_BOOST);
    list<CUserCDP> cdpList;
    auto dbIt = MakeDbIterator(cdp_ratio_index_cache);
    for (dbIt->First(); dbIt->IsValid(); dbIt->Next()) {
        if (std::get<1>(dbIt->GetKey()).value >= ratioBoost) {
            break;
        }
        cdpList.push_back(dbIt->GetValue());
    }
    return std::move(cdpList);
}

CCdpGlobalData CCdpDBCache::GetCdpGlobalData(const CCdpCoinPair &cdpCoinPair) const {
    CCdpGlobalData ret;
    cdp_global_data_cache.GetData(cdpCoinPair, ret);
    return ret;
}

bool CCdpDBCache::GetCdpBcoin(const TokenSymbol &bcoinSymbol, CCdpBcoinDetail &cdpBcoin) {
    auto it = kCdpBcoinSymbolSet.find(bcoinSymbol);
    if (it != kCdpBcoinSymbolSet.end()) {
        cdpBcoin.status = CdpBcoinStatus::STAKE_ON;
        cdpBcoin.init_tx_cord = CRegID(0, 1);
        return true;
    }
    if (bcoinSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(bcoinSymbol) > 0) {
        return false;
    }
    return cdp_bcoin_cache.GetData(bcoinSymbol, cdpBcoin);
}

bool CCdpDBCache::IsCdpBcoinActivated(const TokenSymbol &bcoinSymbol) {
    if (kCdpBcoinSymbolSet.count(bcoinSymbol) > 0) return true;
    if (bcoinSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(bcoinSymbol) > 0) return false;
    return cdp_bcoin_cache.HasData(bcoinSymbol);
}

bool CCdpDBCache::SetCdpBcoin(const TokenSymbol &bcoinSymbol, const CCdpBcoinDetail &cdpBcoin) {
    if (kCdpBcoinSymbolSet.count(bcoinSymbol) > 0) return false;
    if (bcoinSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(bcoinSymbol) > 0) return false;
    CCdpBcoinDetail oldCdpBcoin;
    if (cdp_bcoin_cache.GetData(bcoinSymbol, oldCdpBcoin)) {
        oldCdpBcoin.status = cdpBcoin.status; // just update the status
        return cdp_bcoin_cache.SetData(bcoinSymbol, oldCdpBcoin);
    } else {
        return cdp_bcoin_cache.SetData(bcoinSymbol, cdpBcoin);
    }
}

void CCdpDBCache::SetBaseViewPtr(CCdpDBCache *pBaseIn) {
    cdp_global_data_cache.SetBase(&pBaseIn->cdp_global_data_cache);
    cdp_cache.SetBase(&pBaseIn->cdp_cache);
    cdp_bcoin_cache.SetBase(&pBaseIn->cdp_bcoin_cache);
    user_cdp_cache.SetBase(&pBaseIn->user_cdp_cache);
    cdp_ratio_index_cache.SetBase(&pBaseIn->cdp_ratio_index_cache);
    cdp_height_index_cache.SetBase(&pBaseIn->cdp_height_index_cache);
}

void CCdpDBCache::SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
    cdp_global_data_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_bcoin_cache.SetDbOpLogMap(pDbOpLogMapIn);
    user_cdp_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_ratio_index_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_height_index_cache.SetDbOpLogMap(pDbOpLogMapIn);
}

uint32_t CCdpDBCache::GetCacheSize() const {
    return cdp_global_data_cache.GetCacheSize() + cdp_cache.GetCacheSize() + cdp_bcoin_cache.GetCacheSize() +
            user_cdp_cache.GetCacheSize() + cdp_ratio_index_cache.GetCacheSize() + cdp_height_index_cache.GetCacheSize();
}

bool CCdpDBCache::Flush() {
    cdp_global_data_cache.Flush();
    cdp_cache.Flush();
    cdp_bcoin_cache.Flush();
    user_cdp_cache.Flush();
    cdp_ratio_index_cache.Flush();
    cdp_ratio_index_cache.Flush();

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
            return "redeem";
        case CDPCloseType:: BY_FORCE_LIQUIDATE :
            return "force_liquidate";
        case CDPCloseType ::BY_MANUAL_LIQUIDATE:
            return "manual_liquidate";
    }
    return "undefined";
}