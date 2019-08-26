// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contract.h"
#include "config/const.h"

///////////////////////////////////////////////////////////////////////////////
// class CLuaContract

bool CLuaContract::IsValid() {
    if (code.size() > MAX_CONTRACT_CODE_SIZE)
        return false;

    if (code.compare(0, LUA_CONTRACT_HEADLINE.size(), LUA_CONTRACT_HEADLINE))
        return false;  // lua script shebang existing verified

    if (memo.size() > MAX_CONTRACT_MEMO_SIZE)
        return false;

    return true;
}

bool CUniversalContract::IsValid() {
    if (vm_type == VMType::LUA_VM) {
        if (code.compare(0, LUA_CONTRACT_HEADLINE.size(), LUA_CONTRACT_HEADLINE))
            return false;  // lua script shebang existing verified
    }

    if (code.size() > MAX_CONTRACT_CODE_SIZE)
        return false;

    if (memo.size() > MAX_CONTRACT_MEMO_SIZE)
        return false;

    return true;
}