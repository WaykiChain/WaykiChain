// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_ASSET_H
#define TX_ASSET_H

#include "tx.h"

/**
 * Issue a new asset onto Chain
 */
class CAssetIssueTx: public CBaseTx {
public:
    CRegID      owner_regid;    // owner RegID, can be transferred though
    TokenSymbol symbol;         // asset symbol, E.g WICC | WUSD
    TokenName   name;           // asset long name, E.g WaykiChain coin
    bool        mintable;       // whether this token can be minted in the future.
    uint64_t    total_supply;   // boosted by 1e8 for the decimal part, max is 90 billion.

public:
    CAssetTx() : CBaseTx(ASSET_ISSUE_TX) {};



}
#endif //TX_ASSET_H