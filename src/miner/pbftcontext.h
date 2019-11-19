// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef MINER_PBFTCONTEXT_H
#define MINER_PBFTCONTEXT_H

#include <map>
#include <set>
#include "sync.h"
#include "commons/uint256.h"

class CRegID ;
class CBlockConfirmMessage ;

class CPBFTContext {

public:
    map<uint256, set<CRegID>> confirmMessages;
    map<uint256, set<CRegID>> blockMiners ;
    set<uint256> confirmedBlockHashSet ;
    void PutConfirmMessage(CBlockConfirmMessage msg) ;

    void PutConfirmedBlockHash(uint256 hash) ;
};


#endif //MINER_PBFTCONTEXT_H
