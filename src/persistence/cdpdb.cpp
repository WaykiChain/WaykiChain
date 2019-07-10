// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

#include "accounts/id.h"
#include "main.h"

#include <cstdint>

bool CCdpMemCache::LoadAllCdpFromDB() {
    assert(pAccess != nullptr);
    map<std::pair<string, string>, CUserCDP> rawCdps;

    if (!pAccess->GetAllElements(dbk::CDP, rawCdps)) {
        // TODO: log
        return false;
    }

    for (const auto & item: rawCdps) {
        static uint8_t valid = 1; // 0: invalid; 1: valid

        cdps.emplace(item.second, valid);
        totalStakedBcoins += item.second.totalStakedBcoins;
        totalOwedScoins += item.second.totalOwedScoins;
    }

    return true;
}

uint16_t CCdpMemCache::GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const {
    return totalStakedBcoins * bcoinMedianPrice * kPercentBoost / totalOwedScoins;
}

uint64_t CCdpMemCache::GetGlobalCollateral() const {
    return totalStakedBcoins;
}

void CCdpMemCache::Flush() {
    assert(pBase != nullptr);

    pBase->BatchWrite(cdps);
    cdps.clear();
}

bool CCdpMemCache::SaveCdp(const CUserCDP &userCdp) {
    static uint8_t valid = 1;  // 0: invalid; 1: valid
    cdps.emplace(userCdp, valid);

    totalStakedBcoins += userCdp.totalStakedBcoins;
    totalOwedScoins += userCdp.totalOwedScoins;

    return true;
}

bool CCdpMemCache::EraseCdp(const CUserCDP &userCdp) {
    static uint8_t invalid = 0;  // 0: invalid; 1: valid

    cdps[userCdp] = invalid;
    totalStakedBcoins -= userCdp.totalStakedBcoins;
    totalOwedScoins -= userCdp.totalOwedScoins;

    return true;
}

bool CCdpMemCache::GetCdpListByCollateralRatio(const uint16_t collateralRatio, const uint64_t bcoinMedianPrice,
                                         set<CUserCDP> &userCdps) {
    return GetCdpList(collateralRatio * bcoinMedianPrice, userCdps);
}

bool CCdpMemCache::GetCdpList(const double ratio, set<CUserCDP> &expiredCdps, set<CUserCDP> &userCdps) {
    static CRegID regId(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint16_t>::max());
    static uint256 txid;
    static CUserCDP cdp(regId, txid);
    cdp.collateralRatioBase = ratio;
    cdp.ownerRegId          = regId;

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
        return pBase->GetCdpList(ratio, expiredCdps, userCdps);
    }

    return true;
}

bool CCdpMemCache::GetCdpList(const double ratio, set<CUserCDP> &userCdps) {
    set<CUserCDP> expiredCdps;
    if (!GetCdpList(ratio, expiredCdps, userCdps)) {
        // TODO: log
        return false;
    }

    return true;
}

void CCdpMemCache::BatchWrite(const map<CUserCDP, uint8_t> &cdpsIn) {
    // If the value is empty, delete it from cache.
    for (const auto &item : cdpsIn) {
        if (db_util::IsEmpty(item.second)) {
            cdps.erase(item.first);
        } else {
            cdps[item.first] = item.second;
        }
    }
}

bool CCdpDBCache::StakeBcoinsToCdp(const int32_t blockHeight, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                                    CUserCDP &cdp, CDBOpLogMap &dbOpLogMap) {
    // update cdp's properties before saving
    cdp.blockHeight = blockHeight;
    cdp.totalStakedBcoins += bcoinsToStake;
    cdp.totalOwedScoins += mintedScoins;
    cdp.collateralRatioBase = double(cdp.totalStakedBcoins) / cdp.totalOwedScoins;

    if (!SaveCdp(cdp, dbOpLogMap)) {
        return ERRORMSG("CCdpDBCache::StakeBcoinsToCdp : SetData failed.");
    }

    return true;
}

bool CCdpDBCache::GetCdpList(const CRegID &regId, vector<CUserCDP> &cdps) {
    map<std::pair<string, uint256>, CUserCDP> elements;
    if (!cdpCache.GetAllElements(regId.ToRawString(), elements)) {
        return false;
    }

    for (const auto &item : elements) {
        cdps.push_back(item.second);
    }

    return true;
}

bool CCdpDBCache::GetCdp(CUserCDP &cdp) {
    if (!cdpCache.GetData(std::make_pair(cdp.ownerRegId.ToRawString(), cdp.cdpTxId), cdp))
        return false;

    return true;
}

bool CCdpDBCache::SaveCdp(CUserCDP &cdp) {
    return cdpCache.SetData(std::make_pair(cdp.ownerRegId.ToRawString(), cdp.cdpTxId), cdp);
}

bool CCdpDBCache::SaveCdp(CUserCDP &cdp, CDBOpLogMap &dbOpLogMap) {
    return cdpCache.SetData(std::make_pair(cdp.ownerRegId.ToRawString(), cdp.cdpTxId), cdp, dbOpLogMap);
}

bool CCdpDBCache::EraseCdp(const CUserCDP &cdp) {
    return cdpCache.EraseData(std::make_pair(cdp.ownerRegId.ToRawString(), cdp.cdpTxId));
}

bool CCdpDBCache::EraseCdp(const CUserCDP &cdp, CDBOpLogMap &dbOpLogMap) {
    return cdpCache.EraseData(std::make_pair(cdp.ownerRegId.ToRawString(), cdp.cdpTxId), dbOpLogMap);
}

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *
 *  ==> ratio = a / Log10(b+N)
 */
uint64_t CCdpDBCache::ComputeInterest(const int32_t blockHeight, const CUserCDP &cdp) {
    assert(blockHeight > cdp.blockHeight);

    int32_t blockInterval = blockHeight - cdp.blockHeight;
    int32_t loanedDays = ceil( (double) blockInterval / kDayBlockTotalCount );
    uint16_t A = GetDefaultInterestParamA();
    uint16_t B = GetDefaultInterestParamB();
    uint64_t N = cdp.totalOwedScoins;
    double annualInterestRate = 0.1 * (double) A / log10( 1 + B * N);
    double interest = ((double) N / 365) * loanedDays * annualInterestRate;

    return (uint64_t) interest;
}

// global collateral ratio floor check
bool CCdpDBCache::CheckGlobalCollateralFloorReached(const uint64_t bcoinMedianPrice) {
    bool floorRatioReached = cdpMemCache.GetGlobalCollateralRatio(bcoinMedianPrice) < kGlobalCollateralRatioLimit;
    return floorRatioReached;
}

// global collateral amount ceiling check
bool CCdpDBCache::CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake) {
    bool ceilingAmountReached = (newBcoinsToStake + cdpMemCache.GetGlobalCollateral()) > kGlobalCollateralCeiling;
    return ceilingAmountReached;
}