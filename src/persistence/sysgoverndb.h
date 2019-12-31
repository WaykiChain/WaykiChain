// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
#include "entities/proposal.hpp"

#include <cstdint>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <tuple>

using namespace std;

class CSysGovernDBCache {
public:
    CSysGovernDBCache() {}
    CSysGovernDBCache(CDBAccess *pDbAccess) : governersCache(pDbAccess), proposalsCache(pDbAccess), secondsCache(pDbAccess) {}
    CSysGovernDBCache(CSysGovernDBCache *pBaseIn) : governersCache(pBaseIn->governersCache), 
                                                    proposalsCache(pBaseIn->proposalsCache),
                                                    secondsCache(pBaseIn->secondsCache) {};

    bool Flush() {
        governersCache.Flush();
        proposalsCache.Flush();
        secondsCache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const { return governersCache.GetCacheSize() 
                                            + proposalsCache.GetCacheSize()
                                            + secondsCache.GetCacheSize(); }

    void SetBaseViewPtr(CSysGovernDBCache *pBaseIn) { 
        governersCache.SetBase(&pBaseIn->governersCache);
        proposalsCache.SetBase(&pBaseIn->proposalsCache);
        secondsCache.SetBase(&pBaseIn->secondsCache); }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { 
        governersCache.SetDbOpLogMap(pDbOpLogMapIn);
        proposalsCache.SetDbOpLogMap(pDbOpLogMapIn);
        secondsCache.SetDbOpLogMap(pDbOpLogMapIn); }

    bool UndoData() { return 
        governersCache.UndoData() && 
        proposalsCache.UndoData() &&
        secondsCache.UndoData(); }

    bool addGoverner(CRegID &governerRegId) {
        //check if existing
        return true;
    }

    bool checkIsGoverner(CRegID &candidateRegId) {
        if (!governersCache.HaveData()) {
            return candidateRegId == CRegID(4109388, 2);
        } else {
            unordered_set<CRegID> regids;
            governersCache.GetData(regids);
            return (regids.find(candidateRegId) != regids.end());
        }
    }


private:
/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    CSimpleKVCache< dbk::SYS_GOVERN,            unordered_set<CRegID>>        governersCache;    // list of governers

/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // pgvn{txid} -> proposal
    CCompositeKVCache< dbk::GOVN_PROP,             uint256,                    CProposal >          proposalsCache;
    // sgvn{txid}{regid} -> 1
    CCompositeKVCache< dbk::GOVN_SECOND,           tuple<uint256, CRegID>,     uint8_t >            secondsCache;
};