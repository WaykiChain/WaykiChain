// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDPDB_H
#define PERSIST_CDPDB_H

#include "accounts/id.h"
#include "dbaccess.h"

#include <map>
#include <string>

using namespace std;

/**
 * CDP Cache Item: stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.1a/Log10(1+b*TNj)
 *
 * Persisted in LDB as:
 *      cdp{$RegID}{$TxCord} --> { lastBlockHeight, mintedScoins, totalStakedBcoins, totalOwedScoins }
 *
 */
struct CUserCdp {
    CRegID ownerRegId;              // CDP Owner RegId, mem-only
    TxCord cdpTxCord;               // Transaction coordinate, mem-only
    double collateralRatio;         // ratio = bcoins / mintedScoins, must be >= 200%, mem-only

    uint64_t lastBlockHeight;       // persisted: Hj (Hj+1 refer to current height)
    uint64_t mintedScoins;          // persisted: mintedScoins = bcoins/rate
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCdp() : lastBlockHeight(0), mintedScoins(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    bool operator()(const CUserCdp &a, const CUserCdp &b) {
        if (a.collateralRatio == b.collateralRatio) {
            if (a.ownerRegId == b.ownerRegId)
                return a.cdpTxCord < b.cdpTxCord;
            else
                return a.ownerRegId < b.ownerRegId;
        } else {
            return a.collateralRatio < b.collateralRatio;
        }
    }

    void UpdateUserCdp(const CRegID &ownerRegIdIn, const TxCord &cdpTxCordIn) {
        ownerRegId      = ownerRegIdIn;
        cdpTxCord       = cdpTxCordIn;
        collateralRatio = double(totalStakedBcoins) / totalOwedScoins;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(lastBlockHeight);
        READWRITE(mintedScoins);
        READWRITE(totalStakedBcoins);
        READWRITE(totalOwedScoins);
    )

    string ToString() {
        return strprintf("lastBlockHeight=%d, mintedScoins=%d, totalStakedBcoins=%d, tatalOwedScoins=%d",
                         lastBlockHeight, mintedScoins, totalStakedBcoins, totalOwedScoins);
    }

    bool IsEmpty() const {
        return lastBlockHeight == 0 && mintedScoins == 0 && totalStakedBcoins == 0 && totalOwedScoins == 0;
    }

    void SetEmpty() {
        lastBlockHeight   = 0;
        mintedScoins      = 0;
        totalStakedBcoins = 0;
        totalOwedScoins   = 0;
    }
};

class CCdpMemCache {
private:
    map<CUserCdp, uint8_t> cdps;  // map: CUserCdp -> flag(0: valid; 1: invalid)
    CCdpMemCache *pBase;

public:
    CCdpMemCache() : pBase(nullptr) {}
    CCdpMemCache(CCdpMemCache *pBaseIn) : pBase(pBaseIn) {}
};

class CCdpCacheManager {
public:
    CCdpCacheManager() {}
    CCdpCacheManager(CDBAccess *pDbAccess): cdpCache(pDbAccess) {}

    bool StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                          const int blockHeight, const int txIndex, CUserCdp &cdp, CDbOpLog &cdpDbOpLog);

    bool GetUnderLiquidityCdps(vector<CUserCdp> & userCdps);

    bool GetCdp(const CRegID &regId, const TxCord &cdpTxCord, CUserCdp &cdp);
    bool SaveCdp(const CRegID &regId, const TxCord &cdpTxCord, CUserCdp &cdp);
    bool UndoCdp(CDbOpLog &opLog) { return cdpCache.UndoData(opLog); }

    uint64_t ComputeInterest(int blockHeight, const CUserCdp &cdp);

    uint64_t GetCollateralRatio() {
        uint64_t ratio = 0;
        return collateralRatio.GetData(ratio) ? ratio : 200;
    }
    uint64_t GetInterestParamA() {
        uint64_t paramA = 0;
        return interestParamA.GetData(paramA) ? paramA : 1;
    }
    uint64_t GetInterestParamB() {
        uint64_t paramB = 0;
        return interestParamB.GetData(paramB) ? paramB : 1;
    }

private:

/*   CDBScalarValueCache   prefixType                 value                 variable               */
/*  -------------------- --------------------------  ------------------   ------------------------ */
    // collateralRatio
    CDBScalarValueCache< dbk::CDP_COLLATERAL_RATIO,  uint64_t>           collateralRatio;
    // interestParamA
    CDBScalarValueCache< dbk::CDP_IR_PARAM_A,        uint64_t>           interestParamA;
    // interestParamB
    CDBScalarValueCache< dbk::CDP_IR_PARAM_B,        uint64_t>           interestParamB;

/*  CDBMultiValueCache     prefixType     key                               value        variable  */
/*  ----------------   --------------   ---------------------------   ---------------   --------- */
    // <CRegID, TxCord> -> CUserCdp
    CDBMultiValueCache< dbk::CDP,         std::pair<string, string>,   CUserCdp >       cdpCache;
};

#endif  // PERSIST_CDPDB_H