// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "contract.h"

static const std::string kLuaScriptHeadLine = "mylib = require";

///////////////////////////////////////////////////////////////////////////////
// class CLuaContract

bool CLuaContract::IsValid() {
    if (code.size() > kContractScriptMaxSize) return false;

    if (code.compare(0, kLuaScriptHeadLine.size(), kLuaScriptHeadLine))
        return false;  // lua script shebang existing verified

    if (memo.size() > kContractMemoMaxSize) return false;

    return true;
}

bool CLuaContract::IsCheckAccount(void) { 
    return false; 
}
