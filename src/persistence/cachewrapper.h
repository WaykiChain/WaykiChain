// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "contractdb.h"
#include "txdb.h"
#include "cdpdb.h"
#include "dex.h"

class CCacheWrapper {
public:
    CAccountCache accountCache;
    CContractCache contractCache;
    CDelegateCache delegateCache;
    CTransactionCache txCache;
    CPricePointCache pricePointCache;
    CCdpCacheManager cdpCache;
    CDexCache dexCache;
    CTxUndo txUndo;

};

#endif //PERSIST_CACHEWRAPPER_H