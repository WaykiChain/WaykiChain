// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_SCOIN_H
#define CONFIG_SCOIN_H

#include <cstdint>

static const uint16_t kDayBlockTotalCount                   = 8640;     // = 24*3600/10
static const uint16_t kPriceFeedContinuousDeviateTimesLimit = 10;       // after 10 times continuous deviate limit penetration all deposit be deducted

static const uint16_t kPercentBoost                         = 10000;

static const uint16_t kDefaultDexDealFeeRatio               = 4;        // 0.04% * 10000

static const uint32_t kTotalFundCoinAmount                  = 21000000; // 21 million WGRT
static const uint32_t kDefaultPriceFeedStakedFcoinsMin      = 210000;   // 1%: min 210K fcoins deposited to be a price feeder

static const uint16_t kDefaultPriceFeedDeviateAcceptLimit   = 3000;     // 30% * 10000, above than that will be penalized
static const uint16_t kDefaultPriceFeedDeviatePenalty       = 1000;     // 1000 bcoins deduction as penalty

static const uint64_t kGlobalCollateralCeiling              = 21000000; // 10% * 210000000
static const uint16_t kGlobalCollateralRatioLimit           = 8000;     // 80% * 10000

static const uint16_t kStartingCdpCollateralRatio           = 19000;    // 190% * 10000: starting collateral ratio
static const uint16_t kStartingCdpLiquidateRatio            = 15000;    // 1.13 ~ 1.5  : common liquidation
static const uint16_t kNonReturnCdpLiquidateRatio           = 11300;    // 1.03 ~ 1.13 : Non-return to CDP owner
static const uint16_t kForcedCdpLiquidateRatio              = 10300;    // 0 ~ 1.03    : forced liquidation only

static const uint16_t kCdpLiquidateDiscountRate             = 9700;     // 97%

static const uint64_t kBcoinsToStakeAmountMin               = 10000000000;  //100 WICC, dust amount (<100) rejected

static const uint16_t kFcoinGenesisIssueTxIndex         = 1;
static const uint16_t kFcoinGenesisRegisterTxIndex      = 2;
static const uint16_t kSettleServiceRegisterTxIndex     = 3;

#endif
