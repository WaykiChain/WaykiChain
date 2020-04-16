// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CDP_H
#define ENTITIES_CDP_H

#include "config/scoin.h"
#include "asset.h"
#include "id.h"
#include "commons/util/enumhelper.hpp"

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

#include <cmath>

using namespace std;
using namespace json_spirit;

enum class CdpBcoinStatus: uint8_t {
    NONE = 0,
    STAKE_ON,
    STAKE_OFF,
};

static const EnumHelper<CdpBcoinStatus, uint8_t> kCdpBcoinStatusHelper = {{
    {CdpBcoinStatus::NONE, "NONE"},
    {CdpBcoinStatus::STAKE_ON, "STAKE_ON"},
    {CdpBcoinStatus::STAKE_OFF, "STAKE_OFF"}
}};

class CCdpBcoinDetail {
public:
    CdpBcoinStatus status = CdpBcoinStatus::NONE;
    CTxCord init_tx_cord; // the tx_cord of first init tx

    IMPLEMENT_SERIALIZE(
        READWRITE_ENUM(status, uint8_t);
        READWRITE(init_tx_cord);
    )

    string ToString() const {
        return strprintf("status=%s, init_tx_cord=%s", kCdpBcoinStatusHelper.GetName(status), init_tx_cord.ToString());
    }

    bool IsEmpty() const { return status == CdpBcoinStatus::NONE; }

    void SetEmpty() {
        status = CdpBcoinStatus::NONE;
        init_tx_cord.SetEmpty();
    }
};

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
    int32_t block_height = 0;           // persisted: Hj (Hj+1 refer to current height) - last op block height
    TokenSymbol bcoin_symbol;           // persisted
    TokenSymbol scoin_symbol;           // persisted
    uint64_t total_staked_bcoins = 0;   // persisted: total staked bcoins
    uint64_t total_owed_scoins = 0;     // persisted: TNj = last + minted = total minted - total redempted

    mutable double collateral_ratio_base = 0; // ratioBase = total_staked_bcoins / total_owed_scoins, mem-only

    CUserCDP() {}

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
              ComputeCollateralRatioBase(); // will init collateral_ratio_base
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

    void PartialLiquidate(int32_t blockHeight, uint64_t bcoinsToLiquidate, uint64_t scoinsToLiquidate);

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

    CCdpCoinPair GetCoinPair() const {
        return CCdpCoinPair(bcoin_symbol, scoin_symbol);
    }
};

class CCdpGlobalData {
public:
    uint64_t total_staked_assets = 0;
    uint64_t total_owed_scoins = 0;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(total_staked_assets));
        READWRITE(VARINT(total_owed_scoins));
    )

    bool IsEmpty() const {
        return total_staked_assets == 0 && total_owed_scoins == 0;
    }

    void SetEmpty() {
        total_staked_assets = 0;
        total_owed_scoins = 0;
    }

    string ToString() const {
        return strprintf("total_staked_assets=%llu", total_staked_assets) + ", " +
        strprintf("total_owed_scoins=%llu", total_owed_scoins);

    }

    uint64_t GetCollateralRatio(const uint64_t assetPrice) const {
        // If total owed scoins equal to zero, the global collateral ratio becomes infinite.
        if (total_owed_scoins == 0) {
            return UINT64_MAX;
        }

        return double(total_staked_assets) * assetPrice / PRICE_BOOST / total_owed_scoins * RATIO_BOOST;
    }

    // global collateral ratio floor check
    bool CheckGlobalCollateralRatioFloorReached(const uint64_t assetPrice,
                                                const uint64_t globalCollateralRatioLimit) const {
        return GetCollateralRatio(assetPrice) < globalCollateralRatioLimit;
    }

    // global collateral amount ceiling check
    bool CheckGlobalCollateralCeilingReached(const uint64_t newAssetsToStake,
                                             const uint64_t globalCollateralCeiling) const {
        return (newAssetsToStake + total_staked_assets) > globalCollateralCeiling * COIN;
    }
};

enum class CdpCoinPairStatus: uint8_t {
    NONE                = 0,  // none
    NORMAL              = 1,  // enable all operation (stake, redeem, liquidate, feed price)
    DISABLE_ALL         = 2,  // Disable all cdp related operation (stake, redeem, liquidate, feed price)
    DISABLE_STAKE_CDP   = 4,  // Disable staking cdp
};

static const EnumTypeMap<CdpCoinPairStatus, string, uint8_t> kCdpCoinPairStatusNames = {
    {CdpCoinPairStatus::NONE, "NONE"},
    {CdpCoinPairStatus::NORMAL, "NORMAL"},
    {CdpCoinPairStatus::DISABLE_ALL, "DISABLE_ALL"},
    {CdpCoinPairStatus::DISABLE_STAKE_CDP, "DISABLE_STAKE_CDP"},
};

inline const string& GetCdpCoinPairStatusName(const CdpCoinPairStatus &status) {
    auto it = kCdpCoinPairStatusNames.find(status);
    if (it != kCdpCoinPairStatusNames.end())
        return it->second;
    return EMPTY_STRING;
}

#endif //ENTITIES_CDP_H