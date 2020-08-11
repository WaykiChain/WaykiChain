// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COIN_NODEINFO_H
#define COIN_NODEINFO_H

#include "main.h"
#include "commons/util/util.h"

#include <string>

using namespace std;

extern CCacheDBManager *pCdMan;
extern int32_t nSyncTipHeight;

extern bool mining;
extern CKeyID minerKeyId;
extern CKeyID nodeKeyId;

struct NodeInfo {
    bool bp;        //is a current block producer or not
    string nv;      //node program version
    string nfp;     //node fingerprint
    uint32_t synh;  //sync block height
    uint32_t tiph;  //tip block height
    uint32_t finh;  //finalized block height
};

void getnodeinfo(NodeInfo *pNodeInfo) {
    static const string fullVersion = strprintf("%s (%s)", FormatFullVersion().c_str(), CLIENT_DATE.c_str());
    pNodeInfo->nv = fullVersion;
    pNodeInfo->bp = mining;
    pNodeInfo->nfp = mining ? minerKeyId.ToString() : nodeKeyId.ToString();
    pNodeInfo->synh = nSyncTipHeight;
    pNodeInfo->tiph = chainActive.Height();
    std::pair<int32_t ,uint256> globalfinblock = std::make_pair(0,uint256());
    pCdMan->pBlockCache->ReadGlobalFinBlock(globalfinblock);
    pNodeInfo->finh = globalfinblock.first; 
}

#endif