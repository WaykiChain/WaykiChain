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
    CSysGovernDBCache(CDBAccess *pDbAccess) : governors_cache(pDbAccess),
                                              proposals_cache(pDbAccess),
                                              approvals_cache(pDbAccess) {}
    CSysGovernDBCache(CSysGovernDBCache *pBaseIn) : governors_cache(pBaseIn->governors_cache),
                                                    proposals_cache(pBaseIn->proposals_cache),
                                                    approvals_cache(pBaseIn->approvals_cache) {};

    bool Flush() {
        governors_cache.Flush();
        proposals_cache.Flush();
        approvals_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return governors_cache.GetCacheSize()
               + proposals_cache.GetCacheSize()
               + approvals_cache.GetCacheSize();
    }

    void SetBaseViewPtr(CSysGovernDBCache *pBaseIn) {
        governors_cache.SetBase(&pBaseIn->governors_cache);
        proposals_cache.SetBase(&pBaseIn->proposals_cache);
        approvals_cache.SetBase(&pBaseIn->approvals_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        governors_cache.SetDbOpLogMap(pDbOpLogMapIn);
        proposals_cache.SetDbOpLogMap(pDbOpLogMapIn);
        approvals_cache.SetDbOpLogMap(pDbOpLogMapIn);
        approvals_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }


    uint8_t GetGovernorApprovalMinCount(){

        set<CRegID> regids;
        if (GetGovernors(regids)) {
            return regids.size() - (regids.size() / 3);
        }

        throw runtime_error("get governer list error");
    }

    bool SetProposal(const uint256& txid,  shared_ptr<CProposal>& proposal) {
        return proposals_cache.SetData(txid, CProposalStorageBean(proposal));
    }

    bool GetProposal(const uint256& txid, shared_ptr<CProposal>& proposal) {

        CProposalStorageBean bean;
        if(proposals_cache.GetData(txid, bean) ){
            proposal = bean.sp_proposal;
            return true;
        }

        return false;
    }

    int GetApprovalCount(const uint256 &proposalId){
        vector<CRegID> v;
        approvals_cache.GetData(proposalId, v);
        return static_cast<int>(v.size());
    }

    bool GetApprovalList(const uint256& proposalId, vector<CRegID>& v){
        return  approvals_cache.GetData(proposalId, v);
    }

    bool SetApproval(const uint256 &proposalId, const CRegID &governor){

        vector<CRegID> v;
        if(approvals_cache.GetData(proposalId, v)){
            if(find(v.begin(),v.end(),governor) != v.end()){
                return ERRORMSG("governor(regid= %s) had approvaled this proposal(proposalid=%s)",
                                governor.ToString(), proposalId.ToString());
            }
        }
        v.push_back(governor);
        return approvals_cache.SetData(proposalId,v);
    }

    bool CheckIsGovernor(const CRegID &candidateRegId) {
        set<CRegID> regids;
        if(GetGovernors(regids)){
           return regids.count(candidateRegId) > 0;
        }

        return false;
    }

    bool GetGovernors(set<CRegID>& governors) {
        governors_cache.GetData(governors);
        if(governors.empty())
            governors.insert(CRegID(SysCfg().GetVer2GenesisHeight(), 2));

        return true;
    }

    bool AddGovernor(const CRegID governor){
        set<CRegID> governors;
        governors_cache.GetData(governors);
        governors.insert(governor);
        return governors_cache.SetData(governors);
    }

    bool EraseGovernor(const CRegID governor){
        set<CRegID> governors;
        governors_cache.GetData(governors);
        governors.erase(governor);
        return governors_cache.SetData(governors);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        governors_cache.RegisterUndoFunc(undoDataFuncMap);
        proposals_cache.RegisterUndoFunc(undoDataFuncMap);
        approvals_cache.RegisterUndoFunc(undoDataFuncMap);
    }

public:
/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    CSimpleKVCache< dbk::SYS_GOVERN,            set<CRegID>>        governors_cache;    // list of governors

/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // pgvn{txid} -> proposal
    CCompositeKVCache< dbk::GOVN_PROP,             uint256,     CProposalStorageBean>          proposals_cache;
    // sgvn{txid}->vector(regid)
    CCompositeKVCache< dbk::GOVN_APPROVAL_LIST,    uint256,     vector<CRegID> >               approvals_cache;


};