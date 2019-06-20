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
 *      cdp$RegId --> { blockHeight, mintedScoins, totalOwedScoins }
 *
 */
struct CUserCdp {
    TxCord cdpTxCord;               // persisted: transaction coordinate
    uint64_t lastBlockHeight;       // persisted: Hj (Hj+1 refer to current height)
    uint64_t mintedScoins;          // persisted: mintedScoins = bcoins/rate
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCdp() : lastBlockHeight(0), mintedScoins(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(cdpTxCord);
        READWRITE(lastBlockHeight);
        READWRITE(mintedScoins);
        READWRITE(totalStakedBcoins);
        READWRITE(totalOwedScoins);
    )

    string ToString() {
        return strprintf("cdpTxCord=%s,lastBlockHeight=%d, mintedScoins=%d, totalStakedBcoins=%d, tatalOwedScoins=%d",
                         cdpTxCord.ToString(), lastBlockHeight, mintedScoins, totalStakedBcoins, totalOwedScoins);
    }

    bool IsEmpty() const {
        return cdpTxCord.IsEmpty() && lastBlockHeight == 0 && mintedScoins == 0 && totalStakedBcoins == 0 &&
               totalOwedScoins == 0;
    }

    void SetEmpty() {
        cdpTxCord.Clean();
        lastBlockHeight   = 0;
        mintedScoins      = 0;
        totalStakedBcoins = 0;
        totalOwedScoins   = 0;
    }
};


class CCdpCacheManager {
public:
    CCdpCacheManager() {}
    CCdpCacheManager(CDBAccess *pDbAccess): cdpCache(pDbAccess) {}

    bool StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                          const int blockHeight, CUserCdp &cdp, CDbOpLog &cdpDbOpLog);

    bool GetUnderLiquidityCdps(vector<CUserCdp> & userCdps);

    bool GetCdp(const CRegID &regId, CUserCdp &cdp) { return cdpCache.GetData(regId.ToRawString(), cdp); }
    bool SaveCdp(const CRegID &regId, CUserCdp &cdp) { return cdpCache.SetData(regId.ToRawString(), cdp); }
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

/*  CDBMultiValueCache     prefixType     key                 value            variable            */
/*  ----------------   ----------------- -------------     ---------------   --------------------- */
    // <KeyID -> Account>
    CDBMultiValueCache< dbk::CDP,          string,             CUserCdp >      cdpCache;
};

#endif  // PERSIST_CDPDB_H