// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "asset.h"
#include "config/configuration.h"

bool CheckCoinRange(const TokenSymbol &symbol, const int64_t amount) {
    if (symbol == SYMB::WICC) {
        return CheckBaseCoinRange(amount);
    } else if (symbol == SYMB::WGRT) {
        return CheckFundCoinRange(amount);
    } else if (symbol == SYMB::WUSD) {
        return CheckStableCoinRange(amount);
    } else {
        // TODO: need to check other token range
        return amount >= 0;
    }
}
