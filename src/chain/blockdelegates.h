// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef CHAIN_BLOCK_DELEGATES_H
#define CHAIN_BLOCK_DELEGATES_H

#include "persistence/cachewrapper.h"

namespace chain {

    // process block delegates, call in the tail of block executing
    bool ProcessBlockDelegates(CBlock &block, CCacheWrapper &cw, CValidationState &state);

    bool IsBlockInflatedRewardStarted(CCacheWrapper &cw, HeightType height);
};


#endif //CHAIN_BLOCK_DELEGATES_H