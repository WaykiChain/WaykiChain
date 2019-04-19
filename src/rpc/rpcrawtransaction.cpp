// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "core.h"
#include "init.h"
#include "accounts/keystore.h"
#include "main.h"
#include "net.h"
#include "rpcserver.h"
#include "commons/uint256.h"
#include "../wallet/wallet.h"


#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;


extern void SyncTransaction(const uint256 &hash, const CBaseTx *pTx, const CBlock *pblock);
