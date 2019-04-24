// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef CDP_H
#define CDP_H

// #include <memory>
#include <string>
// #include <vector>
// #include <unordered_map>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "commons/serialize.h"
// #include "key.h"
// #include "chainparams.h"
// #include "crypto/hash.h"
// #include "vote.h"

class CCdp {
private:
    uint64_t bcoinAmount; //collatorized basecoin amount
    uint64_t scoinAmount; //minted stablecoin amount

public:
    CCdp() {}
    CCdp(uint64_t bcoinAmountIn, uint64_t scoinAmountIn): bcoinAmount(bcoinAmountIn), scoinAmount(scoinAmountIn) {}

    uint64_t GetBcoindAmount() const { return bcoinAmount; }
    uint64_t GetScoinAmount() const { return scoinAmount; }

    IMPLEMENT_SERIALIZE(
        READWRITE(bcoinAmount);
        READWRITE(scoinAmount);
    )
};

class CCdpCacheView {

};

#endif//CDP_H
