// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <tuple>
#include <algorithm>

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
#include "entities/proposal.h"


using namespace std;

class CSysGovernDBCache {
public:
    CSysGovernDBCache() {}
    CSysGovernDBCache(CDBAccess *pDbAccess) : governersCache(pDbAccess),
                                              proposalsCache(pDbAccess),
                                              secondsCache(pDbAccess) {}
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

    bool CheckIsGoverner(const CRegID &candidateRegId) {
        if (!governersCache.HaveData()) {
            return candidateRegId == CRegID(SysCfg().GetStableCoinGenesisHeight(), 2);
        } else {
            vector<CRegID> regids;
            if(governersCache.GetData(regids)){
                auto itr = find(regids.begin(), regids.end(), candidateRegId);
                return ( itr != regids.end());
            }
            else
                return false ;
        }
    }


    uint8_t GetNeedGovernerCount(){
        if (!governersCache.HaveData()) {
            return 1 ;
        } else {
            vector<CRegID> regids;
            if(governersCache.GetData(regids)){
                uint8_t cnt = (regids.size()/3)*2 +2 ;
                return cnt>regids.size()?regids.size():cnt;
            } else
                return 1 ;
        }
    }

    bool SetProposal(const uint256& txid, const CProposal& proposal ){
        return proposalsCache.SetData(txid, proposal) ;
    }

    bool GetProposal(const uint256& txid, CProposal& proposal) {
        return proposalsCache.GetData(txid, proposal) ;
    }

    int GetAssentionCount(const uint256& proposalId){
        vector<CRegID> v ;
        secondsCache.GetData(proposalId, v) ;
        return  v.size();
    }
    bool SetAssention(const uint256 &proposalId, const CRegID &governer){

        vector<CRegID> v  ;
        if(secondsCache.GetData(proposalId, v)){
            if(find(v.begin(),v.end(),governer) != v.end()){
                return ERRORMSG("governer(regid= %s) had assented this proposal(proposalid=%s)", governer.ToString(), proposalId.ToString());
            }
        }
        v.push_back(governer) ;
        return secondsCache.SetData(proposalId,v) ;
    }

private:
/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    CSimpleKVCache< dbk::SYS_GOVERN,            vector<CRegID>>        governersCache;    // list of governers

/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // pgvn{txid} -> proposal
    CCompositeKVCache< dbk::GOVN_PROP,             uint256,                    CProposal>          proposalsCache;
    // sgvn{txid}{regid} -> 1
    CCompositeKVCache< dbk::GOVN_SECOND,           uint256,     vector<CRegID> >            secondsCache;
};