// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_VM_H
#define RPC_VM_H

#include "commons/json/json_spirit_value.h"

json_spirit::Value vmexecutescript(const json_spirit::Array& params, bool fHelp);

#endif //RPC_VM_H
