// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

#include "accounts/id.h"
#include "main.h"

#include <cstdint>

bool CCdpMemCache::LoadCdps() {
    assert(pAccess != nullptr);
    map<std::pair<string, string>, CUserCdp> rawCdps;

    if (!pAccess->GetAllElements(dbk::CDP, rawCdps)) {
        // TODO: log
        return false;
    }

    CRegID regId;
    CTxCord txCord;
    CUserCdp userCdp;
    uint8_t valid = 1;
    for (const auto & cdp: rawCdps) {
        regId   = std::get<0>(cdp.first);
        txCord  = std::get<1>(cdp.first);
        userCdp = cdp.second;

        userCdp.UpdateUserCdp(regId, txCord);

        cdps.emplace(userCdp, valid);
    }

    return true;
}

void CCdpMemCache::Flush() {
    assert(pBase != nullptr);

    pBase->BatchWrite(cdps);
    cdps.clear();
}

bool CCdpMemCache::GetUnderLiquidityCdps(const uint16_t openLiquidateRatio, const uint64_t bcoinMedianPrice,
                                         set<CUserCdp> &userCdps) {
    return GetCdps(openLiquidateRatio * bcoinMedianPrice, userCdps);
}

bool CCdpMemCache::GetForceSettleCdps(const uint16_t forceLiquidateRatio, const uint64_t bcoinMedianPrice,
                                      set<CUserCdp> &userCdps) {
    return GetCdps(forceLiquidateRatio * bcoinMedianPrice, userCdps);
}

bool CCdpMemCache::GetCdps(const double ratio, set<CUserCdp> &expiredCdps, set<CUserCdp> &userCdps) {
    static CUserCdp cdp;
    static CRegID regId(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint16_t>::max());
    cdp.collateralRatio = ratio;
    cdp.ownerRegId      = regId;

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
        return pBase->GetCdps(ratio, expiredCdps, userCdps);
    }

    return true;
}

bool CCdpMemCache::GetCdps(const double ratio, set<CUserCdp> &userCdps) {
    set<CUserCdp> expiredCdps;
    if (!GetCdps(ratio, expiredCdps, userCdps)) {
        // TODO: log
        return false;
    }

    return true;
}

void CCdpMemCache::BatchWrite(const map<CUserCdp, uint8_t> &cdpsIn) {
    // If the value is empty, delete it from cache.
    for (const auto &item : cdpsIn) {
        if (db_util::IsEmpty(item.second)) {
            cdps.erase(item.first);
        } else {
            cdps[item.first] = item.second;
        }
    }
}

bool CCdpCacheManager::StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                                        const int blockHeight, const int txIndex, CUserCdp &cdp, CDbOpLog &cdpDbOpLog) {
    cdpDbOpLog = CDbOpLog(cdpCache.GetPrefixType(), regId.ToRawString(), cdp);

    cdp.lastBlockHeight = blockHeight;
    cdp.totalStakedBcoins += bcoinsToStake;
    cdp.totalOwedScoins += mintedScoins;

    if (!SaveCdp(regId, CTxCord(blockHeight, txIndex), cdp)) {
        return ERRORMSG("CCdpCacheManager::StakeBcoinsToCdp : SetData failed.");
    }

    return true;
}

bool CCdpCacheManager::GetCdp(const CRegID &regId, const CTxCord &cdpTxCord, CUserCdp &cdp) {
    if (!cdpCache.GetData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()), cdp))
        return false;

    cdp.UpdateUserCdp(regId, cdpTxCord);

    return true;
}

bool CCdpCacheManager::SaveCdp(const CRegID &regId, const CTxCord &cdpTxCord, CUserCdp &cdp) {
    if (!cdpCache.SetData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()), cdp))
        return false;

    cdp.UpdateUserCdp(regId, cdpTxCord);

    return true;
}

bool CCdpCacheManager::EraseCdp(const CRegID &regId, const CTxCord &cdpTxCord) {
    return cdpCache.EraseData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()));
}

bool CCdpCacheManager::AddCdpOpLog(const CRegID &regId, const CTxCord &cdpTxCord,
                                   const CUserCdp &cdp, CDBOpLogsMap &dbOpLogsMap) {
    return false; // TODO:...                                       
}
/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *  a = 1, b = 1
 *
 *  ==> ratio = 1/Log10(1+N)
 */
uint64_t CCdpCacheManager::ComputeInterest(int blockHeight, const CUserCdp &cdp) {
    assert(uint64_t(blockHeight) > cdp.lastBlockHeight);

    int interval = blockHeight - cdp.lastBlockHeight;
    double interest = ((double) GetInterestParamA() * cdp.totalOwedScoins / kYearBlockCount)
                    * log10(GetInterestParamB() + cdp.totalOwedScoins) * interval;

    return (uint64_t) interest;
}