// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDPDB_H
#define PERSIST_CDPDB_H

#include "accounts/id.h"
#include "dbaccess.h"
#include "tx/tx.h"  // TODO: constant definitions

#include <map>
#include <string>

using namespace std;

/**
 * CDP Cache Item: stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.1a/Log10(1+b*TNj)
 *
 * Persisted in LDB as:
 *      cdp{$RegID}{$CTxCord} --> { blockHeight, totalStakedBcoins, totalOwedScoins }
 *
 */
struct CUserCDP {
    mutable double collateralRatioBase;  // ratioBase = totalStakedBcoins / totalOwedScoins, mem-only

    CRegID ownerRegId;              // CDP Owner RegId
    uint256 cdpTxId;                // CDP TxID
    uint64_t blockHeight;           // persisted: Hj (Hj+1 refer to current height) - last op block height
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCDP() : blockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    CUserCDP(const CRegID &regId, const uint256 &cdpTxIdIn)
        : ownerRegId(regId), cdpTxId(cdpTxIdIn), blockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    bool operator<(const CUserCDP &cdp) const {
        if (collateralRatioBase == cdp.collateralRatioBase) {
            if (ownerRegId == cdp.ownerRegId)
                return cdpTxId < cdp.cdpTxId;
            else
                return ownerRegId < cdp.ownerRegId;
        } else {
            return collateralRatioBase < cdp.collateralRatioBase;
        }
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(ownerRegId);
        READWRITE(cdpTxId);
        READWRITE(VARINT(blockHeight));
        READWRITE(VARINT(totalStakedBcoins));
        READWRITE(VARINT(totalOwedScoins));
        if (fRead) {
            collateralRatioBase = double(totalStakedBcoins) / totalOwedScoins;
        }
    )

    string ToString() {
        return strprintf(
            "ownerRegId=%s, cdpTxId=%s, blockHeight=%d, totalStakedBcoins=%d, tatalOwedScoins=%d, "
            "collateralRatioBase=%f",
            ownerRegId.ToString(), cdpTxId.ToString(), blockHeight, totalStakedBcoins, totalOwedScoins,
            collateralRatioBase);
    }

    bool IsEmpty() const {
        return cdpTxId.IsEmpty();
    }

    void SetEmpty() {
        cdpTxId             = uint256();
        blockHeight         = 0;
        totalStakedBcoins   = 0;
        totalOwedScoins     = 0;
    }
};

class CCdpMemCache {
public:
    CCdpMemCache() {}
    CCdpMemCache(CCdpMemCache *pBaseIn) : pBase(pBaseIn) {}
    // Only apply to construct the global mem-cache.
    CCdpMemCache(CDBAccess *pAccessIn) : pAccess(pAccessIn) {}

    uint16_t GetGlobalCollateralRatio(const uint64_t bcoinMedianPrice) const;
    uint64_t GetGlobalCollateral() const;

    bool LoadCdps();
    void Flush();

    // Usage: before modification, erase the old cdp; after modification, save the new cdp.
    bool SaveCdp(const CUserCDP &userCdp);
    bool EraseCdp(const CUserCDP &userCdp);

    bool GetUnderLiquidityCdps(const uint16_t openLiquidateRatio, const uint64_t bcoinMedianPrice,
                               set<CUserCDP> &userCdps);
    bool GetForceSettleCdps(const uint16_t forceLiquidateRatio, const uint64_t bcoinMedianPrice,
                            set<CUserCDP> &userCdps);

private:
    bool GetCdps(const double ratio, set<CUserCDP> &expiredCdps, set<CUserCDP> &userCdps);
    bool GetCdps(const double ratio, set<CUserCDP> &userCdps);

    void BatchWrite(const map<CUserCDP, uint8_t> &cdpsIn);

private:
    map<CUserCDP, uint8_t> cdps;  // map: CUserCDP -> flag(0: valid; 1: invalid)
    uint64_t totalStakedBcoins = 0;
    uint64_t totalOwedScoins   = 0;
    CCdpMemCache *pBase        = nullptr;
    CDBAccess *pAccess         = nullptr;
};

class CCdpDBCache {
public:
    CCdpDBCache() {}
    CCdpDBCache(CDBAccess *pDbAccess): cdpCache(pDbAccess) {}

    bool StakeBcoinsToCdp(const int32_t blockHeight, const uint64_t bcoinsToStake, const uint64_t mintedScoins, CUserCDP &cdp);
    bool StakeBcoinsToCdp(const int32_t blockHeight, const uint64_t bcoinsToStake, const uint64_t mintedScoins, CUserCDP &cdp, 
                        CDBOpLogMap &dbOpLogMap);

    bool GetCdp(CUserCDP &cdp);
    bool SaveCdp(CUserCDP &cdp); //first-time cdp creation
    bool SaveCdp(CUserCDP &cdp, CDBOpLogMap &dbOpLogMap);
    bool EraseCdp(const CUserCDP &cdp);
    bool UndoCdp(CDBOpLogMap &dbOpLogMap) { return cdpCache.UndoData(dbOpLogMap);  }

    uint64_t ComputeInterest(int32_t blockHeight, const CUserCDP &cdp);

    bool CheckGlobalCollateralFloorReached(const uint64_t bcoinMedianPrice);
    bool CheckGlobalCollateralCeilingReached(const uint64_t newBcoinsToStake);

    uint16_t GetStartingCollateralRatio() {
        uint16_t ratio = 0;
        return collateralRatio.GetData(ratio) ? ratio : kStartingCdpCollateralRatio;
    }
    uint16_t GetStartingLiquidateRatio() {
        uint16_t ratio = 0;
        return openLiquidateRatio.GetData(ratio) ? ratio : kStartingCdpLiquidateRatio;
    }
    uint16_t GetDefaultForceLiquidateRatio() {
        uint16_t ratio = 0;
        return forceLiquidateRatio.GetData(ratio) ? ratio : kForcedCdpLiquidateRatio;
    }
    uint16_t GetDefaultInterestParamA() {
        uint16_t paramA = 0;
        return interestParamA.GetData(paramA) ? paramA : 1;
    }
    uint16_t GetDefaultInterestParamB() {
        uint16_t paramB = 0;
        return interestParamB.GetData(paramB) ? paramB : 1;
    }

public:
    CCdpMemCache cdpMemCache;

private:

/*   CDBScalarValueCache   prefixType                 value                 variable               */
/*  -------------------- --------------------------  ------------------   ------------------------ */
    // collateralRatio
    CDBScalarValueCache< dbk::CDP_COLLATERAL_RATIO,  uint16_t>           collateralRatio;
    // openLiquidateRatio
    CDBScalarValueCache< dbk::CDP_OPEN_LIQUIDATE_RATIO, uint16_t>        openLiquidateRatio;
    // forceLiquidateRatio
    CDBScalarValueCache< dbk::CDP_FORCE_LIQUIDATE_RATIO, uint16_t>       forceLiquidateRatio;
    // interestParamA
    CDBScalarValueCache< dbk::CDP_IR_PARAM_A,        uint16_t>           interestParamA;
    // interestParamB
    CDBScalarValueCache< dbk::CDP_IR_PARAM_B,        uint16_t>           interestParamB;

/*  CDBMultiValueCache     prefixType     key                               value        variable  */
/*  ----------------   --------------   ---------------------------   ---------------    --------- */
    // <CRegID, CTxCord> -> CUserCDP
    CDBMultiValueCache< dbk::CDP,         std::pair<string, uint256>,   CUserCDP >       cdpCache;
};

#endif  // PERSIST_CDPDB_H
