// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexdb.h"
#include "accounts/account.h"
#include "main.h"

bool CDexCache::CreateBuyOrder(uint64_t buyAmount, CoinType targetCoinType) {
    //TODO
    CUserID fcoinGenesisUid(CRegID(kFcoinGenesisTxHeight, kFcoinGenesisIssueTxIndex));
    return true;
}
bool CDexCache::CreateSellOrder(uint64_t sellAmount, CoinType targetCoinType) {
    //TODO
    return true;
}