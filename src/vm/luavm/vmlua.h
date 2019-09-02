// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VMLUA_H
#define VMLUA_H

#include "main.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace std;

class CVmRunEnv;

class CVmlua {
public:
    CVmlua(const std::string &code, const std::string &arguments);
    ~CVmlua();

    std::tuple<uint64_t, string> Run(uint64_t fuelLimit, CVmRunEnv *pVmRunEnv);
    static std::tuple<bool, string> CheckScriptSyntax(const char *filePath);

private:
    // to hold contract call arguments
    std::string code;
    std::string arguments;
};

#endif  // VMLUA_H
