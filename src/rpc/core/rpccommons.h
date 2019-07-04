// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef RPC_CORE_COMMONS_H
#define RPC_CORE_COMMONS_H

#include <string>
#include "accounts/id.h"

using namespace std;

string RegIDToAddress(CUserID &userId);
static bool GetKeyId(string const &addr, CKeyID &KeyId);
Object GetTxDetailJSON(const uint256& txhash);
Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx);

#endif //RPC_CORE_COMMONS_H