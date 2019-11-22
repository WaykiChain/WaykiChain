// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CDP_H
#define ENTITIES_CDP_H

#include "config/scoin.h"
#include "asset.h"
#include "id.h"

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

#include <cmath>

using namespace std;
using namespace json_spirit;

/**
 * CDP Cache Item: stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.2a/Log10(1+b*TNj)
 *
 * Persisted in LDB as:
 *      cdp{$RegID}{$CTxCord} --> { blockHeight, total_staked_bcoins, total_owed_scoins }
 *
 */
struct CUserCDP {
    uint256 cdpid;                      // CDP TxID
    CRegID owner_regid;                 // CDP Owner RegId
    int32_t block_height;               // persisted: Hj (Hj+1 refer to current height) - last op block height
    TokenSymbol bcoin_symbol;           // persisted
    TokenSymbol scoin_symbol;           // persisted
    uint64_t total_staked_bcoins;       // persisted: total staked bcoins
    uint64_t total_owed_scoins;         // persisted: TNj = last + minted = total minted - total redempted

    mutable double collateral_ratio_base; // ratioBase = total_staked_bcoins / total_owed_scoins, mem-only

    CUserCDP() : block_height(0), total_staked_bcoins(0), total_owed_scoins(0), collateral_ratio_base(0) {}

    CUserCDP(const CRegID &regId, const uint256 &cdpidIn, int32_t blockHeight,
             TokenSymbol bcoinSymbol, TokenSymbol scoinSymbol, uint64_t totalStakedBcoins,
             uint64_t totalOwedScoins)
        : cdpid(cdpidIn),
          owner_regid(regId),
          block_height(blockHeight),
          bcoin_symbol(bcoinSymbol),
          scoin_symbol(scoinSymbol),
          total_staked_bcoins(totalStakedBcoins),
          total_owed_scoins(totalOwedScoins) {
              ComputeCollateralRatioBase();
          }

    // Attention: NEVER use block_height as a comparative factor, as block_height may not change, i.e. liquidating
    // partially.
    bool operator<(const CUserCDP &cdp) const {


        if(fabs(this->collateral_ratio_base - cdp.collateral_ratio_base) > 1e-8)
            return this->collateral_ratio_base < cdp.collateral_ratio_base;

        if(this->owner_regid != cdp.owner_regid)
            return this->owner_regid < cdp.owner_regid;

        if(this->cdpid != cdp.cdpid)
            return this->cdpid < cdp.cdpid;

        if(this->total_staked_bcoins != cdp.total_staked_bcoins)
            return this->total_staked_bcoins < cdp.total_staked_bcoins;

        return this->total_owed_scoins < cdp.total_owed_scoins;

    }

    IMPLEMENT_SERIALIZE(
        READWRITE(cdpid);
        READWRITE(owner_regid);
        READWRITE(VARINT(block_height));
        READWRITE(bcoin_symbol);
        READWRITE(scoin_symbol);
        READWRITE(VARINT(total_staked_bcoins));
        READWRITE(VARINT(total_owed_scoins));
        if (fRead) {
            ComputeCollateralRatioBase();
        }
    )

    string ToString() const;

    Object ToJson(uint64_t bcoinMedianPrice) const;

    inline void ComputeCollateralRatioBase() const {
        if (total_staked_bcoins != 0 && total_owed_scoins == 0) {
            collateral_ratio_base = UINT64_MAX;  // big safe percent
        } else if (total_staked_bcoins == 0 || total_owed_scoins == 0) {
            collateral_ratio_base = 0;
        } else {
            collateral_ratio_base = double(total_staked_bcoins) / total_owed_scoins;
        }
    }

    uint64_t GetCollateralRatio(uint64_t bcoinPrice) {
        ComputeCollateralRatioBase();
        if(collateral_ratio_base == UINT64_MAX)
            return UINT64_MAX;

        return  (double(bcoinPrice) / PRICE_BOOST) * collateral_ratio_base * RATIO_BOOST;
    }

    void Redeem(int32_t blockHeight, uint64_t bcoinsToRedeem, uint64_t scoinsToRepay);

    void AddStake(int32_t blockHeight, uint64_t bcoinsToStake, uint64_t mintedScoins);

    void LiquidatePartial(int32_t blockHeight, uint64_t bcoinsToLiquidate, uint64_t scoinsToLiquidate);

    bool IsFinished() const { return total_owed_scoins == 0 && total_staked_bcoins == 0; }

    bool IsEmpty() const { return cdpid.IsEmpty(); }

    void SetEmpty() {
        cdpid.SetEmpty();
        owner_regid.SetEmpty();
        block_height = 0;
        bcoin_symbol.clear();
        scoin_symbol.clear();
        total_staked_bcoins = 0;
        total_owed_scoins = 0;
    }
};

#endif //ENTITIES_CDP_H