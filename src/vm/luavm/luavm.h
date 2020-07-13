// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LUA_VM_H
#define LUA_VM_H

#include "commons/types.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace std;

class CLuaVMRunEnv;

class CLuaVM {
public:
    CLuaVM(const std::string &code, const std::string &arguments);
    ~CLuaVM();

    std::tuple<uint64_t, string> Run(uint64_t fuelLimit, CLuaVMRunEnv *pVmRunEnv);
    static std::tuple<bool, string> CheckScriptSyntax(const char *filePath, HeightType height);

private:
    // to hold contract call arguments
    std::string code;
    std::string arguments;
};

#endif  // LUA_VM_H
