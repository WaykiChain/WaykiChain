// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

bool CCDPMemCache::LoadAllCDPFromDB() {
    assert(pAccess != nullptr);
    map<std::pair<string, string>, CUserCDP> rawCdps;

    if (!pAccess->GetAllElements(dbk::CDP, rawCdps)) {
        // TODO: log
        return false;
    }

    for (const auto & item: rawCdps) {
        cdps.emplace(item.second, CDPState::CDP_VALID);
        total_staked_bcoins += item.second.total_staked_bcoins;
        total_owed_scoins += item.second.total_owed_scoins;
    }

    return true;
}

void CCDPMemCache::SetBase(CCDPMemCache *pBaseIn) {
    assert(pBaseIn != nullptr);
    pBase = pBaseIn;
}

void CCDPMemCache::Flush() {
    if (pBase != nullptr) {
        pBase->BatchWrite(cdps);
        cdps.clear();
    }
}

bool CCDPMemCache::SaveCDP(const CUserCDP &userCdp) {
    cdps.emplace(userCdp, CDPState::CDP_VALID);

    total_staked_bcoins += userCdp.total_staked_bcoins;
    total_owed_scoins += userCdp.total_owed_scoins;

    return true;
}

bool CCDPMemCache::EraseCDP(const CUserCDP &userCdp) {
    cdps[userCdp] = CDPState::CDP_EXPIRED;
    total_staked_bcoins -= userCdp.total_staked_bcoins;
    total_owed_scoins -= userCdp.total_owed_scoins;

    return true;
}

bool CCDPMemCache::HaveCDP(const CUserCDP &userCdp) {
    return cdps.count(userCdp) > 0;
}

uint64_t CCDPMemCache::GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const {
    // If total owed scoins equal to zero, the global collateral ratio becomes infinite.
    return (total_owed_scoins == 0) ? UINT64_MAX : total_staked_bcoins * bcoinMedianPrice * kPercentBoost / total_owed_scoins;
}

uint64_t CCDPMemCache::GetGlobalCollateral() const {
    return total_staked_bcoins;
}

bool CCDPMemCache::GetCdpListByCollateralRatio(const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
                                         set<CUserCDP> &userCdps) {
    return GetCDPList(collateralRatio * bcoinMedianPrice, userCdps);
}

bool CCDPMemCache::GetCDPList(const double ratio, set<CUserCDP> &expiredCdps, set<CUserCDP> &userCdps) {
    static CRegID regId(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint16_t>::max());
    static uint256 txid;
    static CUserCDP cdp;
    cdp.collateral_ratio_base = ratio;
    cdp.owner_regid           = regId;

    auto boundary = cdps.upper_bound(cdp);
    if (boundary != cdps.end()) {
        for (auto iter = cdps.begin(); iter != boundary; ++ iter) {
            if (db_util::IsEmpty(iter->second)) {
                expiredCdps.insert(iter->first);
            } else if (expiredCdps.count(iter->first) || userCdps.count(iter->first)) {
                // TODO: log
                continue;
            } else {
                // Got a valid cdp
                userCdps.insert(iter->first);
            }
        }
    }

    if (pBase != nullptr) {
        return pBase->GetCDPList(ratio, expiredCdps, userCdps);
    }

    return true;
}

bool CCDPMemCache::GetCDPList(const double ratio, set<CUserCDP> &userCdps) {
    set<CUserCDP> expiredCdps;
    if (!GetCDPList(ratio, expiredCdps, userCdps)) {
        // TODO: log
        return false;
    }

    return true;
}

void CCDPMemCache::BatchWrite(const map<CUserCDP, uint8_t> &cdpsIn) {
    // If the value is empty, delete it from cache.
    for (const auto &item : cdpsIn) {
        if (db_util::IsEmpty(item.second)) {
            cdps.erase(item.first);
        } else {
            cdps[item.first] = item.second;
        }
    }
}

bool CCDPDBCache::UpdateCdp(const int32_t blockHeight, int64_t changedBcoins, const int64_t changedScoins, CUserCDP &cdp) {
    // 1. erase the old cdp in memory cache
    cdpMemCache.EraseCDP(cdp);

    // 2. update cdp's properties before saving
    cdp.Update(blockHeight, changedBcoins, changedScoins);

    // 3. save or erase cdp in cache/memory cache
    // 3.1 erase cdp from cache if it's time to close cdp. Do not bother to erase cdp from
    //     memory cache as it had never existed.
    if (cdp.total_staked_bcoins == 0 && cdp.total_owed_scoins == 0) {
        return EraseCDP(cdp);

    } else {
    // 3.2 otherwise, save cdp to cache/memory cache.
        return SaveCDP(cdp) && cdpMemCache.SaveCDP(cdp);
    }
}

bool CCDPDBCache::NewCDP(const int32_t blockHeight, CUserCDP &cdp) {
    assert(!cdpMemCache.HaveCDP(cdp));
    return SaveCDP(cdp) && cdpMemCache.SaveCDP(cdp);
}

bool CCDPDBCache::StakeBcoinsToCDP(const int32_t blockHeight, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                                   CUserCDP &cdp) {
    return UpdateCdp(blockHeight, bcoinsToStake, mintedScoins, cdp);
}

bool CCDPDBCache::RedeemBcoinsFromCDP(const int32_t blockHeight, const uint64_t bcoinsToRedeem,
                                      const uint64_t scoinsToRepay, CUserCDP &cdp) {
    return UpdateCdp(blockHeight, -1 * bcoinsToRedeem, -1 * scoinsToRepay, cdp);
}

bool CCDPDBCache::GetCDPList(const CRegID &regId, vector<CUserCDP> &cdpList) {
    set<uint256> cdpTxids;
    if (!regId2CDPCache.GetData(regId.ToRawString(), cdpTxids)) {
        return false;
    }

    CUserCDP userCdp;
    for (const auto &item : cdpTxids) {
        if (!cdpCache.GetData(item, userCdp)) {
            return false;
        }

        cdpList.push_back(userCdp);
    }

    return true;
}

bool CCDPDBCache::GetCDP(const uint256 cdpid, CUserCDP &cdp) {
    if (!cdpCache.GetData(cdpid, cdp))
        return false;

    return true;
}

// Attention: update cdpCache and regId2CDPCache synchronously.
bool CCDPDBCache::SaveCDP(CUserCDP &cdp) {
    set<uint256> cdpTxids;
    regId2CDPCache.GetData(cdp.owner_regid.ToRawString(), cdpTxids);
    cdpTxids.insert(cdp.cdpid);   // failed to insert if txid existed.

    return cdpCache.SetData(cdp.cdpid, cdp) && regId2CDPCache.SetData(cdp.owner_regid.ToRawString(), cdpTxids);
}

bool CCDPDBCache::EraseCDP(const CUserCDP &cdp) {
    set<uint256> cdpTxids;
    regId2CDPCache.GetData(cdp.owner_regid.ToRawString(), cdpTxids);
    cdpTxids.erase(cdp.cdpid);

    // If cdpTxids is empty, regId2CDPCache will erase the key automatically.
    return cdpCache.EraseData(cdp.cdpid) && regId2CDPCache.SetData(cdp.owner_regid.ToRawString(), cdpTxids);
}

// global collateral ratio floor check
bool CCDPDBCache::CheckGlobalCollateralRatioFloorReached(const uint64_t bcoinMedianPrice,
                                                         const uint64_t globalCollateralRatioLimit) {
    return cdpMemCache.GetGlobalCollateralRatio(bcoinMedianPrice) < globalCollateralRatioLimit;
}

// global collateral amount ceiling check
bool CCDPDBCache::CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake,
                                                      const uint64_t globalCollateralCeiling) {
    return (newBcoinsToStake + cdpMemCache.GetGlobalCollateral()) > globalCollateralCeiling * COIN;
}

bool CCDPDBCache::Flush() {
    cdpCache.Flush();
    regId2CDPCache.Flush();
    cdpMemCache.Flush();

    return true;
}

uint32_t CCDPDBCache::GetCacheSize() const { return cdpCache.GetCacheSize() + regId2CDPCache.GetCacheSize(); }