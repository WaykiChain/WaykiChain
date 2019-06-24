// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_VERSION_H
#define COIN_VERSION_H

#include "clientversion.h"

#include <string>

//
// client versioning
//

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

//
// network protocol versioning
//

static const int PROTOCOL_VERSION = 10001;

// initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 10001;

// disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 10001;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
//static const int CADDR_TIME_VERSION = 31402;

// only request blocks from nodes outside this range of versions
//static const int NOBLKS_VERSION_START = 32000;
//static const int NOBLKS_VERSION_END = 32400;

// BIP 0031, pong message, is enabled for all versions AFTER this one
//static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
//static const int MEMPOOL_GD_VERSION = 60002;

enum FeatureForkVersionEnum {
    MAJOR_VER_R1 = 10001, // Release 1.0
    MAJOR_VER_R2 = 10002, // Release 2.0: StableCoin Release (2019-06-30)
};

#endif // COIN_VERSION_H
