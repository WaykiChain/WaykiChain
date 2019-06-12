// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"
#include "dbconf.h"
#include "leveldbwrapper.h"

bool CCdpCacheDBManager::StakeBcoins(
    CUserID txUid,
    uint64_t bcoinsToStake,
    uint64_t collateralRatio,
    uint64_t mintedScoins,
    int blockHeight,
    CDbOpLog &cdpDbOpLog) {

    CUserCdp lastCdp;
    if (!cdpCache.GetData(txUid.ToString(), lastCdp)) {
        return ERRORMSG("CCdpCache::StakeBcoins : GetData failed.");
    }

    CUserCdp cdp        = lastCdp;

    cdp.lastBlockHeight = cdp.blockHeight;
    cdp.lastOwedScoins  += cdp.totalOwedScoins;
    cdp.blockHeight     = blockHeight;
    cdp.collateralRatio = collateralRatio;
    cdp.mintedScoins    = mintedScoins;
    cdp.totalOwedScoins += cdp.mintedScoins;

    string cdpKey = txUid.ToString();
    if (!cdpCache.SetData(cdpKey, cdp)) {
        return ERRORMSG("CCdpCache::StakeBcoins : SetData failed.");
    }

    cdpDbOpLog = CDbOpLog(cdpCache.GetPrefixType(), cdpKey, lastCdp);
    return true;
}

bool CCdpCacheDBManager::PayInterest(uint64_t scoinInterest, uint64_t fcoinsInterest) {
    // TODO:
    return true;
}

bool CCdpCacheDBManager::GetUnderLiquidityCdps(vector<CUserCdp> & userCdps) {
    //TODO
    return true;
}

/**
 *  Interest Ratio Formula: ( a/collateralRatioFloat/Log10(b+4*N) )
 *  a = 1, b = 1, collateralRatioFloat = 4
 *
 *  ==> ratio = 1/4/Log10(1+4*N)
 */
uint64_t CCdpCacheDBManager::ComputeInterest(int blockHeight) {
    // TODO:
    // assert(blockHeight > cdp.lastBlockHeight);

    // int interval = blockHeight - cdp.lastBlockHeight;
    // double collateralRatioFloat = collateralRatio / 100.0;
    // double interest = ((double) (interestParamA / collateralRatioFloat) * totalOwedScoins / kYearBlockCount)
    //                 * log10(interestParamB + collateralRatioFloat * totalOwedScoins) * interval;

    // return (uint64_t) interest;
    return 0;
}