// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdp.h"

string CUserCDP::ToString() const {
    return strprintf(
        "cdpid=%s, owner_regid=%s, block_height=%d, bcoin_symbol=%s, total_staked_bcoins=%d, "
        "scoin_symbol=%s, tatal_owed_scoins=%d, collateral_ratio_base=%f",
        cdpid.ToString(), owner_regid.ToString(), block_height, bcoin_symbol, total_staked_bcoins,
        scoin_symbol, total_owed_scoins, collateral_ratio_base);
}

Object CUserCDP::ToJson(uint64_t bcoinMedianPrice) const {
    double collateralRatio = collateral_ratio_base * bcoinMedianPrice * 100 / kPercentBoost;

    Object result;
    result.push_back(Pair("cdpid",              cdpid.GetHex()));
    result.push_back(Pair("regid",              owner_regid.ToString()));
    result.push_back(Pair("last_height",        block_height));
    result.push_back(Pair("bcoin_symbol",       bcoin_symbol));
    result.push_back(Pair("total_bcoin",        total_staked_bcoins));
    result.push_back(Pair("scoin_symbol",       scoin_symbol));
    result.push_back(Pair("total_scoin",        total_owed_scoins));
    result.push_back(Pair("collateral_ratio",   strprintf("%.2f%%", collateralRatio)));
    return result;
}

void CUserCDP::Redeem(int32_t blockHeight, uint64_t bcoinsToRedeem, uint64_t scoinsToRepay) {
    assert(total_staked_bcoins >= bcoinsToRedeem);
    assert(total_owed_scoins >= scoinsToRepay);
    block_height = blockHeight;
    total_staked_bcoins -= bcoinsToRedeem;
    total_owed_scoins -= scoinsToRepay;
    Update();
}


void CUserCDP::AddStake(int32_t blockHeight, uint64_t bcoinsToStake, uint64_t mintedScoins) {
    assert(total_staked_bcoins + bcoinsToStake >= total_staked_bcoins);
    assert(total_owed_scoins + mintedScoins >= total_owed_scoins);
    block_height = blockHeight;
    total_staked_bcoins += bcoinsToStake;
    total_owed_scoins += mintedScoins;
    Update();
}

void CUserCDP::LiquidatePartial(int32_t blockHeight, uint64_t bcoinsToLiquidate, uint64_t scoinsToLiquidate) {
    assert(total_staked_bcoins >= bcoinsToLiquidate);
    assert(total_owed_scoins >= scoinsToLiquidate);
    block_height = blockHeight;
    total_staked_bcoins -= bcoinsToLiquidate;
    total_owed_scoins -= scoinsToLiquidate;
    Update();
}

uint64_t CUserCDP::ComputeCollateralRatio(uint64_t bcoinPrice) {
    if (total_staked_bcoins != 0 && total_owed_scoins == 0) {
        return UINT64_MAX; // big safe percent
    } else if (total_staked_bcoins == 0 || total_owed_scoins == 0) {
        return 0;
    } else {
        return  double(bcoinPrice) * total_staked_bcoins / total_owed_scoins;
    }
}
