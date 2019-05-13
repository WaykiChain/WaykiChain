// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef COIN_TXBASE_H
#define COIN_TXBASE_H

#include "commons/uint256.h"

#include <memory>
using namespace std;

class CTxBase {
public:
    virtual uint256 GetHash() { return uint256(); };
    virtual uint64_t GetFee() { return 0; };
    virtual unsigned int GetSerializeSize(int nType, int nVersion) const { return 0; };
};

inline unsigned int GetSerializeSize(const std::shared_ptr<CTxBase> &pa, int nType, int nVersion) {
    return pa->GetSerializeSize(nType, nVersion) + 1;
}

template<typename Stream>
void Serialize(Stream& os, const std::shared_ptr<CTxBase> &pa, int nType, int nVersion) {
}

template<typename Stream>
void Unserialize(Stream& is, std::shared_ptr<CTxBase> &pa, int nType, int nVersion) {
}

// class CTxUndoBase {
// public:
//     uint256 txHash;
//     IMPLEMENT_SERIALIZE(
//         READWRITE(txHash);)
// };

#endif //COIN_TXBASE_H
