// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

#include "accounts/id.h"
#include "main.h"

bool CCdpMemCache::LoadCdps() {
    assert(pAccess != nullptr);
    map<std::pair<string, string>, CUserCdp> rawCdps;

    if (!pAccess->GetAllElements(dbk::CDP, rawCdps)) {
        // TODO: log
        return false;
    }

    CRegID regId;
    TxCord txCord;
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

bool CCdpMemCache::GetUnderLiquidityCdps(const uint16_t openLiquidateRatio, const uint64_t bcoinMedianPrice,
                                         vector<CUserCdp> &userCdps) {
    return GetCdps(openLiquidateRatio * bcoinMedianPrice, userCdps);
}

bool CCdpMemCache::GetForceSettleCdps(const uint16_t forceLiquidateRatio, const uint64_t bcoinMedianPrice,
                                      vector<CUserCdp> &userCdps) {
    return GetCdps(forceLiquidateRatio * bcoinMedianPrice, userCdps);
}

bool CCdpMemCache::GetCdps(const double ratio, vector<CUserCdp> &userCdps) {
    // TODO:
    return true;
}

bool CCdpCacheManager::StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                                        const int blockHeight, const int txIndex, CUserCdp &cdp, CDbOpLog &cdpDbOpLog) {
    cdpDbOpLog = CDbOpLog(cdpCache.GetPrefixType(), regId.ToRawString(), cdp);

    cdp.lastBlockHeight = blockHeight;
    cdp.mintedScoins    = mintedScoins;
    cdp.totalStakedBcoins += bcoinsToStake;
    cdp.totalOwedScoins += cdp.mintedScoins;

    if (!SaveCdp(regId, TxCord(blockHeight, txIndex), cdp)) {
        return ERRORMSG("CCdpCacheManager::StakeBcoinsToCdp : SetData failed.");
    }

    return true;
}

bool CCdpCacheManager::GetCdp(const CRegID &regId, const TxCord &cdpTxCord, CUserCdp &cdp) {
    if (!cdpCache.GetData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()), cdp))
        return false;

    cdp.UpdateUserCdp(regId, cdpTxCord);

    return true;
}

bool CCdpCacheManager::SaveCdp(const CRegID &regId, const TxCord &cdpTxCord, CUserCdp &cdp) {
    if (!cdpCache.SetData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()), cdp))
        return false;

    cdp.UpdateUserCdp(regId, cdpTxCord);

    return true;
}

bool CCdpCacheManager::EraseCdp(const CRegID &regId, const TxCord &cdpTxCord) {
    return cdpCache.EraseData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()));
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