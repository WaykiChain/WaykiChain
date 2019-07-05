// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef RPC_CORE_COMMONS_H
#define RPC_CORE_COMMONS_H

#include <string>
#include "accounts/id.h"
#include "json/json_spirit.h"

using namespace std;
using namespace json_spirit;

const int MAX_RPC_SIG_STR_LEN = 65 * 1024; // 65K

string RegIDToAddress(CUserID &userId);
bool GetKeyId(const string &addr, CKeyID &KeyId);
Object GetTxDetailJSON(const uint256& txhash);
Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx);

Object SubmitTx(CKeyID &userKeyId, CBaseTx &tx);

#endif //RPC_CORE_COMMONS_H