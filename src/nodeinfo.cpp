// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nodeinfo.h"
#include "main.h"
#include "commons/util.h"

extern CCacheDBManager *pCdMan;
extern int32_t nSyncTipHeight;
extern CChain chainActive;

extern bool mining;
extern CKeyID minerKeyId;
extern CKeyID nodeKeyId;

bool nodeinfo(NodeInfo &nodeinfo) {
    static const string fullVersion = strprintf("%s (%s)", FormatFullVersion().c_str(), CLIENT_DATE.c_str());
    nodeinfo.nv = fullVersion;
    nodeinfo.bp = mining;
    nodeinfo.nfp = mining ? minerKeyId.ToString() : nodeKeyId.ToString();
    nodeinfo.synh = nSyncTipHeight;
    nodeinfo.tiph = chainActive.Height();
    nodeinfo.finh = 0; // TODO: placeholder here

}