// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.'

#ifndef ENTITIES_PROPOSAL_H
#define ENTITIES_PROPOSAL_H

#include <string>
#include <vector>
#include <unordered_map>
#include "entities/id.h"
#include "commons/json/json_spirit.h"
#include "config/const.h"
#include "config/txbase.h"
#include "config/scoin.h"
#include "entities/cdp.h"
#include "config/sysparams.h"

class CCacheWrapper ;
class CValidationState ;
class CTxExecuteContext ;
using namespace json_spirit;

enum ProposalType: uint8_t{
    NULL_PROPOSAL     = 0 ,
    PARAM_GOVERN      = 1 ,
    GOVERNER_UPDATE   = 2 ,
    DEX_SWITCH        = 3 ,
    MINER_FEE_UPDATE  = 4 ,
    CDP_COIN_PAIR     = 5, // govern cdpCoinPair, set the cdpCoinPair status
    CDP_PARAM_GOVERN  = 6 ,
    COIN_TRANSFER     = 7 ,
    BP_COUNT_UPDATE   = 8 ,

};

enum ProposalOperateType: uint8_t {
    NULL_OPT = 0,
    ENABLE   = 1 ,
    DISABLE  = 2
};


class CProposal {
public:

    ProposalType proposal_type = NULL_PROPOSAL;
    int8_t need_governer_count = 0;
    int32_t expire_block_height = 0;

public:

    CProposal() {}
    CProposal(uint8_t proposalTypeIn):proposal_type(ProposalType(proposalTypeIn)) {}

    virtual shared_ptr<CProposal> GetNewInstance(){ return nullptr; } ;
    virtual bool ExecuteProposal(CTxExecuteContext& context) { return true ;};
    virtual bool CheckProposal(CCacheWrapper &cw, CValidationState& state) {return true ;};
    virtual string ToString(){
        return strprintf("proposaltype=%d,needgoverneramount=%d,expire_height=%d",
                proposal_type, need_governer_count, expire_block_height) ;
    }
    virtual Object ToJson(){
        Object o ;
        o.push_back(Pair("proposal_type", proposal_type)) ;
        o.push_back(Pair("need_governer_count", need_governer_count)) ;
        o.push_back(Pair("expire_block_height", expire_block_height)) ;

        return o ;

    };

    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }
};


class CParamsGovernProposal: public CProposal {
public:
    vector<std::pair<uint8_t, uint64_t>> param_values;

    CParamsGovernProposal(): CProposal(ProposalType::PARAM_GOVERN){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(param_values);
    );

    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        Array arrayItems;
        for (const auto &item : param_values) {
            Object subItem;
            subItem.push_back(Pair("param_code", item.first));

            string param_name = "" ;
            auto itr = SysParamTable.find(SysParamType(item.first)) ;
            if(itr != SysParamTable.end())
                param_name = std::get<2>(itr->second);

            subItem.push_back(Pair("param_name", param_name));
            subItem.push_back(Pair("param_value", item.second));
            arrayItems.push_back(subItem);
        }

        o.push_back(Pair("params",arrayItems)) ;
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString();
        for(auto itr: param_values){
            baseString = strprintf("%s, %s:%d", baseString,itr.first, itr.second ) ;
        }
        return baseString ;
    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CParamsGovernProposal>(*this); } ;

    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
};



class CGovernerUpdateProposal: public CProposal{
public:
    CRegID governer_regid ;
     ProposalOperateType operate_type  = ProposalOperateType::NULL_OPT;

    CGovernerUpdateProposal(): CProposal(ProposalType::GOVERNER_UPDATE){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(governer_regid);
            READWRITE((uint8_t&)operate_type);
    );

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("governer_regid",governer_regid.ToString())) ;
        o.push_back(Pair("operate_type", operate_type));
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString() ;
        return strprintf("%s, governer_regid=%s, operate_type=%d", baseString,
                governer_regid.ToString(),operate_type) ;

    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovernerUpdateProposal>(*this); }
    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
};

class CDexSwitchProposal: public CProposal{
public:
    uint32_t dexid;
    ProposalOperateType operate_type = ProposalOperateType ::ENABLE;
    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(VARINT(dexid));
            READWRITE((uint8_t&)operate_type);
    );

    CDexSwitchProposal(): CProposal(ProposalType::DEX_SWITCH){}

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CDexSwitchProposal>(*this); }

    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("dexid",(uint64_t)dexid)) ;
        o.push_back(Pair("operate_type", operate_type));
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString() ;
        return strprintf("%s, dexid=%d, operate_type=%d", baseString, dexid, operate_type) ;
    }

};

class CMinerFeeProposal: public CProposal {
public:
    TxType tx_type = TxType::NULL_TX ;
    string  fee_symbol = "" ;
    uint64_t  fee_sawi_amount = 0 ;

    CMinerFeeProposal():CProposal(ProposalType::MINER_FEE_UPDATE){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE((uint8_t&)tx_type);
            READWRITE(fee_symbol);
            READWRITE(VARINT(fee_sawi_amount));
            )

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CMinerFeeProposal>(*this); }

    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("tx_type", tx_type));
        o.push_back(Pair("fee_symbol", fee_symbol)) ;
        o.push_back(Pair("fee_sawi_amount", fee_sawi_amount));
        return o ;

    }

    string ToString() override {
        string baseString = CProposal::ToString() ;
        return strprintf("%s, tx_type=%d, fee_symbo=%s ,fee_sawi_amount=%d",
                baseString, tx_type, fee_symbol,fee_sawi_amount ) ;

    }
};

class CCoinTransferProposal: public CProposal {

public:
    uint64_t amount ;
    TokenSymbol token ;
    CUserID from_uid ;
    CUserID to_uid ;

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(VARINT(amount));
            READWRITE(token) ;
            READWRITE(from_uid) ;
            READWRITE(to_uid) ;
    );


    CCoinTransferProposal(): CProposal(ProposalType::COIN_TRANSFER){}

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CCoinTransferProposal>(*this); } ;

    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;

    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("coin_symbol", token)) ;
        o.push_back(Pair("amount", amount)) ;
        o.push_back(Pair("from_uid", from_uid.ToString())) ;
        o.push_back(Pair("to_uid", to_uid.ToString())) ;
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString();
        return baseString ;
    }


};

class CCdpParamGovernProposal: public CProposal {

public:
    vector<std::pair<uint8_t, uint64_t>> param_values;
    CCdpCoinPair coinPair ;

    CCdpParamGovernProposal(): CProposal(ProposalType::CDP_PARAM_GOVERN){}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(param_values);
            READWRITE(coinPair);
    );

    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("asset_pair", coinPair.ToString()));
        Array arrayItems;
        for (const auto &item : param_values) {
            Object subItem;
            subItem.push_back(Pair("param_code", item.first));

            string param_name = "" ;
            auto itr = SysParamTable.find(SysParamType(item.first)) ;
            if(itr != SysParamTable.end())
                param_name = std::get<2>(itr->second);

            subItem.push_back(Pair("param_name", param_name));
            subItem.push_back(Pair("param_value", item.second));
            arrayItems.push_back(subItem);
        }

        o.push_back(Pair("params",arrayItems)) ;
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString();
        for(auto itr: param_values){
            baseString = strprintf("%s, %s:%d", baseString,itr.first, itr.second ) ;
        }
        return baseString ;
    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CCdpParamGovernProposal>(*this); } ;

    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;

};

class CBPCountUpdateProposal: public CProposal {
public:
    uint8_t bp_count ;

    CBPCountUpdateProposal(): CProposal(BP_COUNT_UPDATE) {}
    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expire_block_height));
            READWRITE(need_governer_count);
            READWRITE(bp_count);
    );


    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("bp_count", (uint64_t)bp_count));
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString();
        return baseString ;
    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CBPCountUpdateProposal>(*this); }

    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;


};

class CCdpCoinPairProposal: public CProposal {
public:
    CCdpCoinPair cdpCoinPair;
    CdpCoinPairStatus status; // cdp coin pair status, can not be NONE

    CCdpCoinPairProposal(): CProposal(ProposalType::CDP_PARAM_GOVERN){}

    IMPLEMENT_SERIALIZE(
            READWRITE(cdpCoinPair);
            READWRITE(VARINT((uint8_t&)status));
    );

    virtual Object ToJson() override;

    string ToString() override;

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CCdpCoinPairProposal>(*this); } ;

    bool ExecuteProposal(CTxExecuteContext& context) override;
    bool CheckProposal(CCacheWrapper &cw, CValidationState& state) override;

};

class CProposalStorageBean {

public:
    shared_ptr<CProposal> proposalPtr ;

    CProposalStorageBean() {}

    CProposalStorageBean( shared_ptr<CProposal> ptr): proposalPtr(ptr) {}

    bool IsEmpty() const { return proposalPtr == nullptr; }
    void SetEmpty() { proposalPtr = nullptr; }



    unsigned int GetSerializeSize(int nType, int nVersion) const {

        if(IsEmpty())
            return 1 ;
        else
            return (*proposalPtr).GetSerializeSize(nType, nVersion) + 1 ;
    }

    template <typename Stream>
    void Serialize(Stream &os, int nType, int nVersion) const {

        uint8_t proposalType = ProposalType ::NULL_PROPOSAL ;

        if(!IsEmpty())
            proposalType = proposalPtr->proposal_type ;
        uint8_t pt = (uint8_t&)proposalType;

        ::Serialize(os, pt, nType, nVersion);

        if(IsEmpty())
            return ;

        switch (proposalPtr->proposal_type) {
            case PARAM_GOVERN:
                ::Serialize(os, *((CParamsGovernProposal   *) (proposalPtr.get())), nType, nVersion);
                break;

            case CDP_COIN_PAIR:
                ::Serialize(os, *((CCdpCoinPairProposal *) (proposalPtr.get())), nType, nVersion);
                break;
            case CDP_PARAM_GOVERN:
                ::Serialize(os, *((CCdpParamGovernProposal *) (proposalPtr.get())), nType, nVersion);
                break;
            case GOVERNER_UPDATE:
                ::Serialize(os, *((CGovernerUpdateProposal *) (proposalPtr.get())), nType, nVersion);
                break;
            case DEX_SWITCH:
                ::Serialize(os, *((CDexSwitchProposal      *) (proposalPtr.get())), nType, nVersion);
                break;
            case MINER_FEE_UPDATE:
                ::Serialize(os, *((CMinerFeeProposal       *) (proposalPtr.get())), nType, nVersion);
                break;
            case COIN_TRANSFER:
                ::Serialize(os, *((CCoinTransferProposal   *) (proposalPtr.get())), nType, nVersion);
                break;
            case BP_COUNT_UPDATE:
                ::Serialize(os, *((CBPCountUpdateProposal   *) (proposalPtr.get())), nType, nVersion);
                break;
            default:
                throw ios_base::failure(strprintf("Serialize: proposalType(%d) error.",
                                                  proposalPtr->proposal_type));
        }


    }

    template <typename Stream>
    void Unserialize(Stream &is, int nType, int nVersion) {

        uint8_t nProposalTye;
        is.read((char *)&(nProposalTye), sizeof(nProposalTye));
        ProposalType proposalType = (ProposalType)nProposalTye ;
        if(proposalType == ProposalType:: NULL_PROPOSAL)
            return ;

        switch(proposalType) {

            case PARAM_GOVERN: {
                proposalPtr = std::make_shared<CParamsGovernProposal>();
                ::Unserialize(is, *((CParamsGovernProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case CDP_COIN_PAIR: {
                proposalPtr = std::make_shared<CCdpCoinPairProposal>();
                ::Unserialize(is, *((CCdpCoinPairProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case CDP_PARAM_GOVERN: {
                proposalPtr = std::make_shared<CCdpParamGovernProposal>();
                ::Unserialize(is, *((CCdpParamGovernProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case GOVERNER_UPDATE: {
                proposalPtr = std::make_shared<CGovernerUpdateProposal>();
                ::Unserialize(is, *((CGovernerUpdateProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case DEX_SWITCH: {
                proposalPtr = std::make_shared<CDexSwitchProposal>();
                ::Unserialize(is, *((CDexSwitchProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case MINER_FEE_UPDATE: {
                proposalPtr = std::make_shared<CMinerFeeProposal>();
                ::Unserialize(is, *((CMinerFeeProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case COIN_TRANSFER: {
                proposalPtr = std:: make_shared<CCoinTransferProposal>();
                ::Unserialize(is,  *((CCoinTransferProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            case BP_COUNT_UPDATE: {
                proposalPtr = std:: make_shared<CBPCountUpdateProposal>();
                ::Unserialize(is,  *((CBPCountUpdateProposal *)(proposalPtr.get())), nType, nVersion);
                break;
            }

            default:
                throw ios_base::failure(strprintf("Unserialize: nTxType(%d) error.",
                                                  nProposalTye));
        }
        proposalPtr->proposal_type = proposalType;
    }

};


#endif //ENTITIES_PROPOSAL_H