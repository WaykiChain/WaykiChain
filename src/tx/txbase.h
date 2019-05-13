// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef COIN_TXBASE_H
#define COIN_TXBASE_H

#include "commons/uint256.h"

class CTxBase {
public:
    virtual uint256 GetHash() { return uint256(); };
    virtual uint64_t GetFee() { return 0; };
    virtual unsigned int GetSerializeSize(int nType, int nVersion) const { return 0; };
};

// class CTxUndoBase {
// public:
//     uint256 txHash;
//     IMPLEMENT_SERIALIZE(
//         READWRITE(txHash);)
// };

#endif //COIN_TXBASE_H