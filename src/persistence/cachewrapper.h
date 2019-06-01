// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "contractdb.h"
#include "txdb.h"
#include "stakedb.h"
#include "cdpdb.h"

class CCacheWrapper {
public:
    CAccountCache accountCache;
    CTransactionCache txCache;
    CContractCache contractCache;
    CPricePointCache pricePointCache;
    CStakeCache stakeCache;
    CCdpCache cdpCache;
    CTxUndo txUndo;

};

#endif //PERSIST_CACHEWRAPPER_H