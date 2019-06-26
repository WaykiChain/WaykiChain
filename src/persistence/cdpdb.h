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
    mutable double collateralRatio; // ratio = totalStakedBcoins / totalOwedScoins, must be >= 200%, mem-only

    CRegID ownerRegId;              // CDP Owner RegId
    CTxCord cdpTxCord;              // Transaction coordinate
    uint64_t lastBlockHeight;       // persisted: Hj (Hj+1 refer to current height)
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCdp() : lastBlockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    CUserCdp(const CRegID &regId, const CTxCord &txCord)
        : ownerRegId(regId), cdpTxCord(txCord), lastBlockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    bool operator<(const CUserCdp &cdp) const {
        if (collateralRatio == cdp.collateralRatio) {
            if (ownerRegId == cdp.ownerRegId)
                return cdpTxCord < cdp.cdpTxCord;
            else
                return ownerRegId < cdp.ownerRegId;
        } else {
            return collateralRatio < cdp.collateralRatio;
        }
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(ownerRegId);
        READWRITE(cdpTxCord);
        READWRITE(VARINT(lastBlockHeight));
        READWRITE(VARINT(totalStakedBcoins));
        READWRITE(VARINT(totalOwedScoins));
        if (fRead) {
            collateralRatio = double(totalStakedBcoins) / totalOwedScoins;
        }
    )

    string ToString() {
        return strprintf(
            "ownerRegId=%s, cdpTxCord=%s, lastBlockHeight=%d, totalStakedBcoins=%d, tatalOwedScoins=%d, "
            "collateralRatio=%f",
            ownerRegId.ToString(), cdpTxCord.ToString(), lastBlockHeight, totalStakedBcoins, totalOwedScoins,
            collateralRatio);
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
    CCdpMemCache() : pBase(nullptr), pAccess(nullptr) {}
    CCdpMemCache(CCdpMemCache *pBaseIn) : pBase(pBaseIn), pAccess(nullptr) {}
    // Only apply to construct the global mem-cache.
    CCdpMemCache(CDBAccess *pAccessIn) : pBase(nullptr), pAccess(pAccessIn) {}

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
    CCdpMemCache *pBase;
    CDBAccess *pAccess;
};

class CCdpCacheManager {
public:
    CCdpCacheManager() {}
    CCdpCacheManager(CDBAccess *pDbAccess): cdpCache(pDbAccess) {}

    bool StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                          const int blockHeight, CUserCdp &cdp, CDBOpLogMap &dbOpLogMap);

    bool GetCdp(CUserCdp &cdp);
    bool SaveCdp(CUserCdp &cdp, CDBOpLogMap &dbOpLogMap);
    bool EraseCdp(const CUserCdp &cdp);
    bool UndoCdp(CDBOpLogMap &dbOpLogMap) { /*return cdpCache.UndoData(opLog);*/ return false;  } // TODO:

    uint64_t ComputeInterest(int blockHeight, const CUserCdp &cdp);

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
