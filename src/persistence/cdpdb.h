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
 *      cdp{$RegID}{$CTxCord} --> { lastBlockHeight, totalStakedBcoins, totalOwedScoins }
 *
 */
struct CUserCdp {
    mutable double collateralRatioBase;  // ratio = totalStakedBcoins * price / totalOwedScoins, must be >= 200%, mem-only

    CRegID ownerRegId;              // CDP Owner RegId
    CTxCord cdpTxCord;              // Transaction coordinate
    uint64_t lastBlockHeight;       // persisted: Hj (Hj+1 refer to current height)
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCdp() : lastBlockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    CUserCdp(const CRegID &regId, const CTxCord &txCord)
        : ownerRegId(regId), cdpTxCord(txCord), lastBlockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    bool operator<(const CUserCdp &cdp) const {
        if (collateralRatioBase == cdp.collateralRatioBase) {
            if (ownerRegId == cdp.ownerRegId)
                return cdpTxCord < cdp.cdpTxCord;
            else
                return ownerRegId < cdp.ownerRegId;
        } else {
            return collateralRatioBase < cdp.collateralRatioBase;
        }
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(ownerRegId);
        READWRITE(cdpTxCord);
        READWRITE(VARINT(lastBlockHeight));
        READWRITE(VARINT(totalStakedBcoins));
        READWRITE(VARINT(totalOwedScoins));
        if (fRead) {
            collateralRatioBase = double(totalStakedBcoins) / totalOwedScoins;
        }
    )

    string ToString() {
        return strprintf(
            "ownerRegId=%s, cdpTxCord=%s, lastBlockHeight=%d, totalStakedBcoins=%d, tatalOwedScoins=%d, "
            "collateralRatioBase=%f",
            ownerRegId.ToString(), cdpTxCord.ToString(), lastBlockHeight, totalStakedBcoins, totalOwedScoins,
            collateralRatioBase);
    }

    bool IsEmpty() const {
        // FIXME: ownerRegID/cdpTxCord set empty?
        return lastBlockHeight == 0 && totalStakedBcoins == 0 && totalOwedScoins == 0;
    }

    void SetEmpty() {
        // FIXME: ownerRegID/cdpTxCord set empty?
        lastBlockHeight   = 0;
        totalStakedBcoins = 0;
        totalOwedScoins   = 0;
    }
};

class CCdpMemCache {
public:
    CCdpMemCache() {}
    CCdpMemCache(CCdpMemCache *pBaseIn) : pBase(pBaseIn) {}
    // Only apply to construct the global mem-cache.
    CCdpMemCache(CDBAccess *pAccessIn) : pAccess(pAccessIn) {}

    uint16_t GetGlobalCollateralRatio(const uint64_t price) const;
    uint64_t GetGlobalDebt() const;

    bool LoadCdps();
    void Flush();

    // Usage: before modification, erase the old cdp; after modification, save the new cdp.
    bool SaveCdp(const CUserCdp &userCdp);
    bool EraseCdp(const CUserCdp &userCdp);

    bool GetUnderLiquidityCdps(const uint16_t openLiquidateRatio, const uint64_t bcoinMedianPrice,
                               set<CUserCdp> &userCdps);
    bool GetForceSettleCdps(const uint16_t forceLiquidateRatio, const uint64_t bcoinMedianPrice,
                            set<CUserCdp> &userCdps);

private:
    bool GetCdps(const double ratio, set<CUserCdp> &expiredCdps, set<CUserCdp> &userCdps);
    bool GetCdps(const double ratio, set<CUserCdp> &userCdps);

    void BatchWrite(const map<CUserCdp, uint8_t> &cdpsIn);

private:
    map<CUserCdp, uint8_t> cdps;  // map: CUserCdp -> flag(0: valid; 1: invalid)
    uint64_t totalStakedBcoins = 0;
    uint64_t totalOwedScoins   = 0;
    CCdpMemCache *pBase        = nullptr;
    CDBAccess *pAccess         = nullptr;
};

class CCdpDBCache {
public:
    CCdpDBCache() {}
    CCdpDBCache(CDBAccess *pDbAccess): cdpCache(pDbAccess) {}

    bool StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                          const int blockHeight, CUserCdp &cdp, CDBOpLogMap &dbOpLogMap);

    bool GetCdp(CUserCdp &cdp);
    bool SaveCdp(CUserCdp &cdp, CDBOpLogMap &dbOpLogMap);
    bool EraseCdp(const CUserCdp &cdp);
    bool UndoCdp(CDBOpLogMap &dbOpLogMap) { /*return cdpCache.UndoData(opLog);*/ return false;  } // TODO:

    uint64_t ComputeInterest(int blockHeight, const CUserCdp &cdp);

    // When true, CDP cannot be further operated
    bool GetGlobalCDPLock(const uint64_t price) ;
    // When true, WUSD cannot be owed.
    bool ExceedGlobalDebtCeiling() const;

    uint16_t GetDefaultCollateralRatio() {
        uint16_t ratio = 0;
        return collateralRatio.GetData(ratio) ? ratio : kDefaultCollateralRatio;
    }
    uint16_t GetDefaultOpenLiquidateRatio() {
        uint16_t ratio = 0;
        return openLiquidateRatio.GetData(ratio) ? ratio : kDefaultOpenLiquidateRatio;
    }
    uint16_t GetDefaultForceLiquidateRatio() {
        uint16_t ratio = 0;
        return forceLiquidateRatio.GetData(ratio) ? ratio : kDefaultForcedLiquidateRatio;
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
    // globalCDPHaltState
    CDBScalarValueCache< dbk::CDP_GLOBAL_HALT,  bool>                    cdpGlobalHalt;
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
    // <CRegID, CTxCord> -> CUserCdp
    CDBMultiValueCache< dbk::CDP,         std::pair<string, string>,   CUserCdp >       cdpCache;
};

#endif  // PERSIST_CDPDB_H
