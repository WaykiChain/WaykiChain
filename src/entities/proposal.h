// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_PROPOSAL_H
#define ENTITIES_PROPOSAL_H

#include <string>
#include <vector>
#include <unordered_map>
#include "commons/serialize.h"
#include "entities/id.h"
#include "commons/json/json_spirit.h"

class CCacheWrapper ;
class CValidationState ;
using namespace json_spirit;

enum ProposalType: uint8_t{
    NULL_PROPOSAL     = 0 ,
    PARAM_GOVERN      = 1 ,
    GOVERNER_UPDATE   = 2 ,
    DEX_SWITCH        = 3
};

enum OperateType: uint8_t {
    NULL_OPT = 0,
    ENABLE  = 1 ,
    DISABLE = 2
};


class CProposal {
public:

    uint8_t proposal_type = 0 ;
    int8_t need_governer_count = 0;
    int32_t expire_block_height = 0;

public:

    CProposal() {}
    CProposal(uint8_t proposalTypeIn):proposal_type(proposalTypeIn) {}

    virtual shared_ptr<CProposal> GetNewInstance(){ return nullptr; } ;
    virtual bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) { return true ;};
    virtual bool CheckProposal(CCacheWrapper &cw, CValidationState& state) {return true ;};
    virtual bool IsEmpty() const { return  expire_block_height == 0
                                            && need_governer_count == 0
                                            && proposal_type == ProposalType ::NULL_PROPOSAL; };
    virtual Object ToJson(){
        Object o ;
        o.push_back(Pair("proposal_type", proposal_type)) ;
        o.push_back(Pair("need_governer_count", need_governer_count)) ;
        o.push_back(Pair("expire_block_height", expire_block_height)) ;

        return o ;

    };

    static unsigned int GetSerializePtrSize(const std::shared_ptr<CProposal> &pProposal, int nType, int nVersion){
        return pProposal->GetSerializeSize(nType, nVersion) + 1;
    }

    template<typename Stream>
    static void SerializePtr(Stream& os, const std::shared_ptr<CProposal> &pProposal, int nType, int nVersion);

    template<typename Stream>
    static void UnserializePtr(Stream& is, std::shared_ptr<CProposal> &pProposal, int nType, int nVersion);

    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }
    virtual void SetEmpty() {
        expire_block_height = 0 ;
        need_governer_count = 0 ;
        proposal_type = ProposalType ::NULL_PROPOSAL ;
    }

};

class CNullProposal: public CProposal {
public:
    CNullProposal(): CProposal(ProposalType::NULL_PROPOSAL){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
    );
    bool IsEmpty() const override { return proposal_type == NULL_PROPOSAL || CProposal::IsEmpty(); };
    void SetEmpty() override {
        CProposal::SetEmpty();
    }

    shared_ptr<CProposal> GetNewInstance(){ return make_shared<CNullProposal>(*this);} ;

    bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) override {return true ;};

    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override {return true ;};
};

class CParamsGovernProposal: public CProposal {
public:
    vector<std::pair<string, uint64_t>> param_values;

    CParamsGovernProposal(): CProposal(ProposalType::PARAM_GOVERN){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(param_values);
    );
    bool IsEmpty() const override { return param_values.size()== 0 && CProposal::IsEmpty(); };
    void SetEmpty() override {
        CProposal::SetEmpty();
        param_values.clear();
    }

    virtual Object ToJson(){
        Object o = CProposal::ToJson();


        Array arrayItems;
        for (const auto &item : param_values) {
            Object subItem;
            subItem.push_back(Pair("param_name", item.first));
            subItem.push_back(Pair("param_value", item.second));
            arrayItems.push_back(subItem);
        }

        o.push_back(Pair("params",arrayItems)) ;
        return o ;
    }

    shared_ptr<CProposal> GetNewInstance(){ return make_shared<CParamsGovernProposal>(*this);} ;

    bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) override;

    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
};



class CGovernerUpdateProposal: public CProposal{
public:
    CRegID governer_regid ;
    uint8_t  operate_type  = 0;

    CGovernerUpdateProposal(): CProposal(ProposalType::GOVERNER_UPDATE){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(governer_regid);
            READWRITE(proposal_type);
            READWRITE(operate_type);
    );

    bool IsEmpty() const override { return operate_type == NULL_OPT && governer_regid.IsEmpty() && CProposal::IsEmpty(); };
    void SetEmpty() override {
        CProposal::SetEmpty() ;
        operate_type = NULL_OPT;
        governer_regid.SetEmpty();
    }

    shared_ptr<CProposal> GetNewInstance(){ return make_shared<CGovernerUpdateProposal>(*this);} ;
    bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) override;

    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
};



#endif //ENTITIES_PROPOSAL_H