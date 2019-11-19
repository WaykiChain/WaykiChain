// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pbftcontext.h"
#include "p2p/protocol.h"

CPBFTContext pbftContext ;


void CPBFTContext::PutConfirmMessage(CBlockConfirmMessage msg){

    auto it = confirmMessages.find(msg.blockHash) ;
    if( it != confirmMessages.end()){
        it->second.insert(msg.miner);
    }else{
        set<CRegID> ids ;
        ids.insert(msg.miner) ;
    }
}

void CPBFTContext::PutConfirmedBlockHash(uint256 hash) {
    confirmedBlockHashSet.insert(hash);
}