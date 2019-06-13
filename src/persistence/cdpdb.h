// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDP_CACHE_H
#define PERSIST_CDP_CACHE_H

#include <map>
#include <string>
#include "leveldbwrapper.h"
#include "accounts/id.h"
#include "dbaccess.h"

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
    CRegID ownerRegId;              // CDP Owner RegId

    uint64_t lastBlockHeight;       // Hj, mem-only
    uint64_t lastOwedScoins;        // TNj, mem-only
    uint64_t collateralRatio;       // ratio = bcoins / mintedScoins, must be >= 200%

    uint64_t blockHeight;           // persisted: Hj+1
    uint64_t mintedScoins;          // persisted: mintedScoins = bcoins/rate
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    CUserCdp(): lastOwedScoins(0), collateralRatio(200), mintedScoins(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(blockHeight);
        READWRITE(collateralRatio);
        READWRITE(mintedScoins);
        READWRITE(totalStakedBcoins);
        READWRITE(totalOwedScoins);
    )

    string ToString() {
        return strprintf("%d%d%d%d", blockHeight, mintedScoins, totalStakedBcoins, totalOwedScoins);
    }

    bool IsEmpty() const {
        return ownerRegId.IsEmpty(); // TODO: need to check empty for other fields?
    }
};


class CCdpCacheDBManager {

public:
    CCdpCacheDBManager() {}
    CCdpCacheDBManager(CDBAccess *pDbAccess): cdpCache(pDbAccess, dbk::CDP) {}

    bool StakeBcoinsToCdp(CUserID txUid, uint64_t bcoinsToStake, uint64_t collateralRatio,
                    uint64_t mintedScoins, int blockHeight, CDbOpLog &cdpDbOpLog);

    bool GetUnderLiquidityCdps(vector<CUserCdp> & userCdps);

    bool GetCdp(string userRegId, CUserCdp &cdp) { return cdpCache.GetData(userRegId, cdp); }
    bool SaveCdp(string userRegId, CUserCdp &cdp) { return cdpCache.SetData(userRegId, cdp); }
    bool UndoCdp(CDbOpLog &opLog) { return cdpCache.UndoData(opLog); }

    uint64_t ComputeInterest(int blockHeight, const CUserCdp &cdp);

    uint64_t GetCollateralRatio() {
        uint64_t ratio;
        return collateralRatio.GetData(ratio) ? ratio : 200;
    }
    uint64_t GetInterestParamA() {
        uint64_t paramA;
        return interestParamA.GetData(paramA) ? paramA : 1;
    }
    uint64_t GetInterestParamB() {
        uint64_t paramB;
        return interestParamB.GetData(paramB) ? paramB : 1;
    }

private:
    CDBScalarValueCache<uint64_t> collateralRatio;
    CDBScalarValueCache<uint64_t> interestParamA;
    CDBScalarValueCache<uint64_t> interestParamB;

    CDBMultiValueCache<string, CUserCdp> cdpCache;      // CdpOwnerRegId -> CUserCdp

};

enum OrderType {
    BUY = 1,
    SELL = 2,
    NULL_TYPE = 0
};

enum CoinType {
    WICC = 1,
    WUSD = 2,
    MICC = 3,
};

struct CDexOrder {
    uint64_t amount;
    uint64_t discount; // *1000 E.g. 97% * 1000 = 970
}

class CDexCacheDBManager {
public:
    CDexCacheDBManager() {}
    CDexCacheDBManager(CDBAccess *pDbAccess): cdpCache(pDbAccess, dbk::CDP) {}

private:
    //key: dex{wicc2wusd|wusd2wicc|wusd2micc|micc2wusd}{RegID}
    CDBMultiValueCache<string, CDexOrder> dexManualBuyMiccOrderCache;  // buy micc with wusd
    CDBMultiValueCache<string, CDexOrder> dexManualSellMiccOrderCache; // sell micc for wusd
    CDBMultiValueCache<string, CDexOrder> dexManualBuyWiccOrderCache;  // buy wicc with wusd
    CDBMultiValueCache<string, CDexOrder> dexManualSellWiccOrderCache; // sell wicc for wusd

    CDBMultiValueCache<string, CUserCdp> dexCdpBuyMiccOrderCache;     //with wusd
}

#endif //PERSIST_CDP_CACHE_H