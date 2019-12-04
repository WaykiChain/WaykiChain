// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef RPC_WASM_H
#define RPC_WASM_H

#include "json/json_spirit_value.h"
using namespace std;
using namespace json_spirit;

extern Value submitwasmcontractdeploytx(const Array& params, bool fHelp);
extern Value submitwasmcontractcalltx(const Array& params, bool fHelp);
extern Value gettablewasm(const Array& params, bool fHelp);

#endif //RPC_VM_H