// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"
#include "dbconf.h"
#include "main.h"

bool CCdpCacheManager::StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake,
                                        const uint64_t collateralRatio, const uint64_t mintedScoins,
                                        const int blockHeight, CUserCdp &cdp, CDbOpLog &cdpDbOpLog) {
    string cdpKey = regId.ToRawString();
    cdpDbOpLog = CDbOpLog(cdpCache.GetPrefixType(), cdpKey, cdp);

    cdp.lastBlockHeight = blockHeight;
    cdp.collateralRatio = collateralRatio;
    cdp.mintedScoins    = mintedScoins;
    cdp.lastOwedScoins += cdp.totalOwedScoins;
    cdp.totalOwedScoins += cdp.mintedScoins;

    if (!cdpCache.SetData(cdpKey, cdp)) {
        return ERRORMSG("CCdpCache::StakeBcoins : SetData failed.");
    }

    return true;
}

bool CCdpCacheManager::GetUnderLiquidityCdps(vector<CUserCdp> & userCdps) {
    //TODO
    return true;
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