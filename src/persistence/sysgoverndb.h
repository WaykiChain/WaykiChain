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
    CSysGovernDBCache(CDBAccess *pDbAccess) : governorsCache(pDbAccess),
                                              proposalsCache(pDbAccess),
                                              approvalListCache(pDbAccess) {}
    CSysGovernDBCache(CSysGovernDBCache *pBaseIn) : governorsCache(pBaseIn->governorsCache),
                                                    proposalsCache(pBaseIn->proposalsCache),
                                                    approvalListCache(pBaseIn->approvalListCache) {};

    bool Flush() {
        governorsCache.Flush();
        proposalsCache.Flush();
        approvalListCache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return governorsCache.GetCacheSize()
               + proposalsCache.GetCacheSize()
               + approvalListCache.GetCacheSize();
    }

    void SetBaseViewPtr(CSysGovernDBCache *pBaseIn) { 
        governorsCache.SetBase(&pBaseIn->governorsCache);
        proposalsCache.SetBase(&pBaseIn->proposalsCache);
        approvalListCache.SetBase(&pBaseIn->approvalListCache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { 
        governorsCache.SetDbOpLogMap(pDbOpLogMapIn);
        proposalsCache.SetDbOpLogMap(pDbOpLogMapIn);
        approvalListCache.SetDbOpLogMap(pDbOpLogMapIn);
    }





    uint8_t GetGovernorApprovalMinCount(){

        vector<CRegID> regids;
        if (governorsCache.GetData(regids)) {
            return regids.size() * 2 / 3 + regids.size()%3;
        } 

        return 1 ;
    }

    bool SetProposal(const uint256& txid,  shared_ptr<CProposal>& proposal ){
        return proposalsCache.SetData(txid, CProposalStorageBean(proposal)) ;
    }

    bool GetProposal(const uint256& txid, shared_ptr<CProposal>& proposal) {

        CProposalStorageBean bean ;
        if(proposalsCache.GetData(txid, bean) ){
            proposal = bean.sp_proposal ;
            return true;
        }

        return false ;
    }

    int GetApprovalCount(const uint256 &proposalId){
        vector<CRegID> v ;
        approvalListCache.GetData(proposalId, v) ;
        return  v.size();
    }

    bool GetApprovalList(const uint256& proposalId, vector<CRegID>& v){
        return  approvalListCache.GetData(proposalId, v) ;
    }
    
    bool SetApproval(const uint256 &proposalId, const CRegID &governor){

        vector<CRegID> v  ;
        if(approvalListCache.GetData(proposalId, v)){
            if(find(v.begin(),v.end(),governor) != v.end()){
                return ERRORMSG("governor(regid= %s) had approvaled this proposal(proposalid=%s)",
                                governor.ToString(), proposalId.ToString());
            }
        }
        v.push_back(governor) ;
        return approvalListCache.SetData(proposalId,v) ;
    }

    bool CheckIsGovernor(const CRegID &candidateRegId) {
        if (!governorsCache.HaveData())
            return (candidateRegId == CRegID(SysCfg().GetStableCoinGenesisHeight(), 2));

        vector<CRegID> regids;
        if(governorsCache.GetData(regids)){
            auto itr = find(regids.begin(), regids.end(), candidateRegId);
            return ( itr != regids.end() );
        }

        return false ;
    }

    bool SetGovernors(const vector<CRegID> &governors){
        return governorsCache.SetData(governors) ;
    }
    bool GetGovernors(vector<CRegID>& governors){

        governorsCache.GetData(governors);
        if(governors.empty())
            governors.emplace_back(CRegID(SysCfg().GetStableCoinGenesisHeight(), 2));
        return true;
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        governorsCache.RegisterUndoFunc(undoDataFuncMap);
        proposalsCache.RegisterUndoFunc(undoDataFuncMap);
        approvalListCache.RegisterUndoFunc(undoDataFuncMap);
    }

private:
/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    CSimpleKVCache< dbk::SYS_GOVERN,            vector<CRegID>>        governorsCache;    // list of governors

/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // pgvn{txid} -> proposal
    CCompositeKVCache< dbk::GOVN_PROP,             uint256,     CProposalStorageBean>          proposalsCache;
    // sgvn{txid}->vector(regid)
    CCompositeKVCache< dbk::GOVN_APPROVAL_LIST,    uint256,     vector<CRegID> >               approvalListCache;

};