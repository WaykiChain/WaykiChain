// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_ERRORCODE_H
#define CONFIG_ERRORCODE_H

#include <string>

/** "reject" message codes **/
static const uint8_t REJECT_MALFORMED           = 0x01;

static const uint8_t REJECT_INVALID             = 0x10;
static const uint8_t REJECT_OBSOLETE            = 0x11;
static const uint8_t REJECT_DUPLICATE           = 0x12;

static const uint8_t REJECT_NONSTANDARD         = 0x20;
static const uint8_t REJECT_DUST                = 0x21;
static const uint8_t REJECT_INSUFFICIENTFEE     = 0x22;

static const uint8_t READ_ACCOUNT_FAIL          = 0X30;
static const uint8_t WRITE_ACCOUNT_FAIL         = 0X31;
static const uint8_t UPDATE_ACCOUNT_FAIL        = 0X32;

static const uint8_t PRICE_FEED_FAIL            = 0X40;
static const uint8_t FCOIN_STAKE_FAIL           = 0X41;
static const uint8_t CDP_LIQUIDATE_FAIL         = 0X41;

static const uint8_t READ_SCRIPT_FAIL           = 0X50;
static const uint8_t WRITE_SCRIPT_FAIL          = 0X51;

static const uint8_t STAKE_CDP_FAIL             = 0X60;
static const uint8_t REDEEM_CDP_FAIL            = 0X61;
static const uint8_t WRITE_CDP_FAIL             = 0X62;
static const uint8_t INTEREST_INSUFFICIENT      = 0x63;
static const uint8_t UPDATE_CDP_FAIL            = 0X64;

static const uint8_t WRITE_CANDIDATE_VOTES_FAIL = 0X70;

static const uint8_t CREATE_SYS_ORDER_FAILED    = 0x80;
static const uint8_t UNDO_SYS_ORDER_FAILED      = 0x81;

static const uint8_t READ_SYS_PARAM_FAIL        = 0x90;
static const uint8_t WRITE_SYS_PARAM_FAIL       = 0x91;

static const uint8_t READ_PRICE_POINT_FAIL      = 0xa0;
static const uint8_t WRITE_PRICE_POINT_FAIL     = 0xa1;

#endif //CONFIG_ERRORCODE_H