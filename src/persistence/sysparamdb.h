// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/serialize.h"
#include "persistence/dbaccess.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <tuple>

using namespace std;

class CSysParamDBCache {
public:
    CSysParamDBCache() {}
    CSysParamDBCache(CDBAccess *pDbAccess) : sysParamCache(pDbAccess) {}
    CSysParamDBCache(CSysParamDBCache *pBaseIn) : sysParamCache(pBaseIn->sysParamCache) {}

    bool GetParam(const SysParamType &paramType, uint64_t& paramValue) {
        if (SysParamTable.count(paramType) == 0)
            return false;

        auto iter = SysParamTable.find(paramType);
        string keyPostfix = std::get<0>(iter->second);
        if (!sysParamCache.GetData(keyPostfix, paramValue)) {
            paramValue = std::get<1>(iter->second);
        }

        return true;
    }

    bool Flush() {
        sysParamCache.Flush();
        return true;
    }
    uint32_t GetCacheSize() const { return sysParamCache.GetCacheSize(); }

    void SetBaseViewPtr(CSysParamDBCache *pBaseIn) {
        sysParamCache.SetBase(&pBaseIn->sysParamCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        sysParamCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    bool UndoDatas() {
        return sysParamCache.UndoDatas();
    }
private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // order tx id -> active order
    CCompositeKVCache< dbk::SYS_PARAM,             string,                 uint64_t >              sysParamCache;
};