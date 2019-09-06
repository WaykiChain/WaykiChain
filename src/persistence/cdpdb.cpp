// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

bool CCDPMemCache::LoadAllCDPFromDB() {
    assert(pAccess != nullptr);
    map<uint256, CUserCDP> rawCdps;
    if (!pAccess->GetAllElements(dbk::CDP, rawCdps)) {
        LogPrint("CDP", "CCDPMemCache::LoadAllCDPFromDB, GetAllElements failed\n");
        return false;
    }

    for (const auto & item: rawCdps) {
        cdps.emplace(item.second, CDPState::CDP_VALID);
        global_staked_bcoins += item.second.total_staked_bcoins;
        global_owed_scoins += item.second.total_owed_scoins;
    }

    LogPrint("CDP", "CCDPMemCache::LoadAllCDPFromDB, rawCdps: %llu, global_staked_bcoins: %llu, global_owed_scoins: %llu\n",
             rawCdps.size(), global_staked_bcoins, global_owed_scoins);

    return true;
}

void CCDPMemCache::SetBase(CCDPMemCache *pBaseIn) {
    assert(pBaseIn != nullptr);
    pBase = pBaseIn;

    SetGlobalItem(pBaseIn->global_staked_bcoins, pBaseIn->global_owed_scoins);
}

void CCDPMemCache::Flush() {
    if (pBase != nullptr) {
        pBase->BatchWrite(cdps);
        cdps.clear();

        pBase->SetGlobalItem(global_staked_bcoins, global_owed_scoins);
    }
}

bool CCDPMemCache::SaveCDP(const CUserCDP &userCdp) {
    cdps.emplace(userCdp, CDPState::CDP_VALID);

    global_staked_bcoins += userCdp.total_staked_bcoins;
    global_owed_scoins += userCdp.total_owed_scoins;

    return true;
}

bool CCDPMemCache::EraseCDP(const CUserCDP &userCdp) {
    cdps[userCdp] = CDPState::CDP_EXPIRED;
    global_staked_bcoins -= userCdp.total_staked_bcoins;
    global_owed_scoins -= userCdp.total_owed_scoins;

    return true;
}

bool CCDPMemCache::HaveCDP(const CUserCDP &userCdp) {
    return cdps.count(userCdp) > 0;
}

uint64_t CCDPMemCache::GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const {
    // If total owed scoins equal to zero, the global collateral ratio becomes infinite.
    return (global_owed_scoins == 0) ? UINT64_MAX : uint64_t(double(global_staked_bcoins)
        * bcoinMedianPrice / global_owed_scoins);
}

uint64_t CCDPMemCache::GetGlobalCollateral() const {
    return global_staked_bcoins;
}

void CCDPMemCache::GetGlobalItem(uint64_t &globalStakedBcoins, uint64_t &globalOwedScoins) {
    globalStakedBcoins = global_staked_bcoins;
    globalOwedScoins   = global_owed_scoins;
}

bool CCDPMemCache::GetCdpListByCollateralRatio(const uint64_t collateralRatio, const uint64_t bcoinMedianPrice,
                                         set<CUserCDP> &userCdps) {
    return GetCDPList(double(collateralRatio) / bcoinMedianPrice, userCdps);
}

bool CCDPMemCache::GetCDPList(const double ratio, set<CUserCDP> &expiredCdps, set<CUserCDP> &userCdps) {
    for (const auto item : cdps) {
        LogPrint("CDP", "CCDPMemCache::GetCDPList, candidate cdp: %s, valid: %d, ratio: %f\n", item.first.ToString(),
                 item.second, ratio);
    }

    static CRegID regId(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint16_t>::max());
    static CUserCDP cdp;
    cdp.collateral_ratio_base = ratio;
    cdp.owner_regid           = regId;

    auto boundary = cdps.upper_bound(cdp);
    for (auto iter = cdps.begin(); iter != boundary; ++ iter) {
        if (db_util::IsEmpty(iter->second)) {
            expiredCdps.insert(iter->first);
        } else if (expiredCdps.count(iter->first) || userCdps.count(iter->first)) {
            LogPrint("CDP", "CCDPMemCache::GetCDPList, found an expired item, ignore\n");
            continue;
        } else {
            // Got a valid cdp
            LogPrint("CDP", "CCDPMemCache::GetCDPList, found an valid item, %s\n", iter->first.ToString());
            userCdps.insert(iter->first);
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
        LogPrint("CDP", "CCDPMemCache::GetCDPList, GetCDPList failed\n");
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

///////////////////////////////////////////////////////////////////////////////
// class CCDPDBCache

void CCDPMemCache::SetGlobalItem(const uint64_t globalStakedBcoins, const uint64_t globalOwedScoins) {
    global_staked_bcoins = globalStakedBcoins;
    global_owed_scoins   = globalOwedScoins;

    LogPrint("CDP", "CCDPMemCache::SetGlobalItem, global_staked_bcoins: %llu, global_owed_scoins: %llu\n",
             global_staked_bcoins, global_owed_scoins);
}

// Need to delete the old cdp(before updating cdp), then save the new cdp if necessary.
bool CCDPDBCache::UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP) {
    cdpMemCache.EraseCDP(oldCDP);
    if (newCDP.IsFinished()) {
        return EraseCDP(oldCDP, newCDP);
    } else {
        return SaveCDPToDB(newCDP) && cdpMemCache.SaveCDP(newCDP);
    }
}

bool CCDPDBCache::NewCDP(const int32_t blockHeight, CUserCDP &cdp) {
    assert(!cdpMemCache.HaveCDP(cdp));
    return SaveCDPToDB(cdp) && cdpMemCache.SaveCDP(cdp);
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

bool CCDPDBCache::EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp) {
    return EraseCDPFromDB(cdp) && cdpMemCache.EraseCDP(oldCDP);
}

// Attention: update cdpCache and regId2CDPCache synchronously.
bool CCDPDBCache::SaveCDPToDB(const CUserCDP &cdp) {
    set<uint256> cdpTxids;
    regId2CDPCache.GetData(cdp.owner_regid.ToRawString(), cdpTxids);
    cdpTxids.insert(cdp.cdpid);   // failed to insert if txid existed.

    return cdpCache.SetData(cdp.cdpid, cdp) && regId2CDPCache.SetData(cdp.owner_regid.ToRawString(), cdpTxids);
}

bool CCDPDBCache::EraseCDPFromDB(const CUserCDP &cdp) {
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
    LogPrint("CDP", "CCDPDBCache::CheckGlobalCollateralCeilingReached, newBcoinsToStake: %llu, "
             "cdpMemCache.GetGlobalCollateral(): %llu, globalCollateralCeiling: %llu\n",
             newBcoinsToStake, cdpMemCache.GetGlobalCollateral(), globalCollateralCeiling * COIN);

    return (newBcoinsToStake + cdpMemCache.GetGlobalCollateral()) > globalCollateralCeiling * COIN;
}

bool CCDPDBCache::Flush() {
    cdpCache.Flush();
    regId2CDPCache.Flush();
    cdpMemCache.Flush();

    return true;
}

uint32_t CCDPDBCache::GetCacheSize() const { return cdpCache.GetCacheSize() + regId2CDPCache.GetCacheSize(); }
