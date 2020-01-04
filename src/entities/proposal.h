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

class CCacheWrapper ;
class CValidationState ;

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

    uint8_t proposalType = 0 ;
    int8_t need_governer_count = 0;
    int32_t expire_block_height = 0;

public:

    CProposal() {}
    CProposal(uint8_t proposalTypeIn):proposalType(proposalTypeIn) {}


    virtual bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) = 0 ;
    virtual bool CheckProposal(CCacheWrapper &cw, CValidationState& state) = 0;
    virtual bool IsEmpty() const { return  expire_block_height == 0
                                            && need_governer_count == 0
                                            && proposalType == ProposalType ::NULL_PROPOSAL; };
    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }
    virtual void SetEmpty() {
        expire_block_height = 0 ;
        need_governer_count = 0 ;
        proposalType = ProposalType ::NULL_PROPOSAL ;
    }

};

class CParamsGovernProposal: public CProposal{
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

    bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) override;

    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
};



class CGovernerUpdateProposal: public CProposal{
public:
    CRegID governerRegId ;
    uint8_t  operateType  = 0;

    CGovernerUpdateProposal(): CProposal(ProposalType::GOVERNER_UPDATE){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(governerRegId);
            READWRITE(proposalType);
            READWRITE(operateType);
    );

    bool IsEmpty() const override { return operateType == NULL_OPT && governerRegId.IsEmpty() && CProposal::IsEmpty(); };
    void SetEmpty() override {
        CProposal::SetEmpty() ;
        operateType = NULL_OPT;
        governerRegId.SetEmpty();
    }

    bool ExecuteProposal(CCacheWrapper &cw, CValidationState& state) override;

    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
};



#endif //ENTITIES_PROPOSAL_H