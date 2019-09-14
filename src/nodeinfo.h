// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COIN_NODEINFO_H
#define COIN_NODEINFO_H

#include <string>

using namespace std;

struct NodeInfo {
    bool bp;        //is a current block producer or not
    string nv;      //node program version
    string nfp;     //node fingerprint
    uint32_t synh;  //sync block height
    uint32_t tiph;  //tip block height
    uint32_t finh;  //finalized block height
};

bool nodeinfo(NodeInfo &nodeinfo);

#endif