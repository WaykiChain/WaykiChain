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

// Proposal for DeGov
enum ProposalType: uint8_t {
    NULL_PROPOSAL       = 0 ,
    GOV_SYS_PARAM       = 1 , // basic parameters
    GOV_BPMC_LIST       = 2 , // update BP Mgmt Committee List
    GOV_BP_SIZE         = 3 , // update BP total number (11 -> 21 -> xxx)
    GOV_MINER_FEE       = 4 , // Miner fees for all Trx types
    GOV_CDP_COINPAIR    = 5 , // govern GOV_CDP_COINPAIR, set GOV_CDP_COINPAIR status
    GOV_CDP_PARAM       = 6 , // CDP parameters
    GOV_COIN_TRANSFER   = 7 , // for private-key loss, account robbery or for CFT/AML etc purposes
    GOV_DEX_OP          = 8 , // turn on/off DEX operator
    GOV_DEX_QUOTE       = 9 , // DEX quote coin
    GOV_FEED_COINPAIR   = 10, // BaseSymbol/QuoteSymbol
    GOV_AXC_IN          = 11, // atomic-cross-chain swap in
    GOV_AXC_OUT         = 12, // atomic-cross-chain swap out

};

enum ProposalOperateType: uint8_t {
    NULL_PROPOSAL_OP    = 0 ,
    ENABLE              = 1 ,
    DISABLE             = 2
};


class CProposal {
public:

    ProposalType proposal_type = NULL_PROPOSAL;
    int8_t approval_min_count = 0;
    int32_t expiry_block_height = 0;

public:

    CProposal() {}
    CProposal(ProposalType proposalTypeIn) : proposal_type(proposalTypeIn) {}

    virtual bool CheckProposal(CTxExecuteContext& context ) {return true ;};
    virtual bool ExecuteProposal(CTxExecuteContext& context) { return true ;};
    virtual string ToString(){
        return strprintf("proposal_type=%d,approval_min_count=%d,expiry_block_height=%d",
                        proposal_type, approval_min_count, expiry_block_height) ;
    }
    virtual Object ToJson(){
        Object o ;
        o.push_back(Pair("proposal_type", proposal_type)) ;
        o.push_back(Pair("approval_min_count", approval_min_count)) ;
        o.push_back(Pair("expiry_block_height", expiry_block_height)) ;

        return o ;
    };

    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }
};

// base currency -> quote currency
class CFeedCoinPairProposal: public CProposal {
public:
    TokenSymbol  feed_symbol;
    TokenSymbol  quote_symbol = SYMB::USD;
    ProposalOperateType op_type = ProposalOperateType::NULL_PROPOSAL_OP;

    CFeedCoinPairProposal(): CProposal(ProposalType::GOV_FEED_COINPAIR) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(feed_symbol);
        READWRITE(quote_symbol);
        READWRITE((uint8_t&)op_type);
    )

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("feed_symbol", feed_symbol));
        o.push_back(Pair("quote_symbol", quote_symbol));

        o.push_back(Pair("op_type", op_type)) ;
        return o ;
    }

    string ToString() override {
        return  strprintf("feed_symbol=%s,quote_symbol=%s",feed_symbol, quote_symbol ) + ", " +
                strprintf("op_type=%d", op_type);
    }

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CDexQuoteCoinProposal: public CProposal {
public:
    TokenSymbol  coin_symbol ;
    ProposalOperateType op_type  = ProposalOperateType::NULL_PROPOSAL_OP;

    CDexQuoteCoinProposal(): CProposal(ProposalType::GOV_DEX_QUOTE) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height)) ;
        READWRITE(approval_min_count) ;
        READWRITE(coin_symbol) ;
        READWRITE((uint8_t&)op_type);
    )

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("coin_symbol", coin_symbol));

        o.push_back(Pair("op_type", op_type)) ;
        return o ;
    }

    string ToString() override {
        return  strprintf("coin_symbol=%s",coin_symbol ) + ", " +
                strprintf("op_type=%d", op_type);
    }

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CParamsGovernProposal: public CProposal {
public:
    vector<std::pair<uint8_t, uint64_t>> param_values;

    CParamsGovernProposal(): CProposal(ProposalType::GOV_SYS_PARAM){}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
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
                param_name = std::get<1>(itr->second);

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

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CGovernorUpdateProposal: public CProposal{
public:
    CRegID governor_regid ;
    ProposalOperateType op_type  = ProposalOperateType::NULL_PROPOSAL_OP;

    CGovernorUpdateProposal(): CProposal(ProposalType::GOV_BPMC_LIST){}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(governor_regid);
        READWRITE((uint8_t&)op_type);
    );

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("governor_regid",governor_regid.ToString())) ;
        o.push_back(Pair("operate_type", op_type));
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString() ;
        return strprintf("%s, governor_regid=%s, operate_type=%d", baseString,
                governor_regid.ToString(),op_type) ;

    }

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CDexSwitchProposal: public CProposal{
public:
    uint32_t dexid;
    ProposalOperateType operate_type = ProposalOperateType::ENABLE;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(VARINT(dexid));
        READWRITE((uint8_t&)operate_type);
    );

    CDexSwitchProposal(): CProposal(ProposalType::GOV_DEX_OP){}

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context ) override;

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

    CMinerFeeProposal() : CProposal(ProposalType::GOV_MINER_FEE) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE((uint8_t&)tx_type);
        READWRITE(fee_symbol);
        READWRITE(VARINT(fee_sawi_amount));
    )

    bool CheckProposal(CTxExecuteContext& context ) override;
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
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(VARINT(amount));
        READWRITE(token) ;
        READWRITE(from_uid) ;
        READWRITE(to_uid) ;
    );


    CCoinTransferProposal(): CProposal(ProposalType::GOV_COIN_TRANSFER) {}

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

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
    CCdpCoinPair coin_pair ;

    CCdpParamGovernProposal(): CProposal(ProposalType::GOV_CDP_PARAM) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(param_values);
        READWRITE(coin_pair);
    );

    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("asset_pair", coin_pair.ToString()));
        Array arrayItems;
        for (const auto &item : param_values) {
            Object subItem;
            subItem.push_back(Pair("param_code", item.first));

            string param_name = "" ;
            auto itr = SysParamTable.find(SysParamType(item.first)) ;
            if(itr != SysParamTable.end())
                param_name = std::get<1>(itr->second);

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

    bool CheckProposal(CTxExecuteContext& context ) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CBPCountUpdateProposal: public CProposal {
public:
    uint8_t bp_count ;
    uint32_t effective_height ;

    CBPCountUpdateProposal(): CProposal(GOV_BP_SIZE) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(bp_count);
        READWRITE(VARINT(effective_height));
    );


    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("bp_count", (uint64_t)bp_count));
        o.push_back(Pair("effective_height",(uint64_t)effective_height));
        return o ;
    }

    string ToString() override {
        string baseString = CProposal::ToString();
        return baseString ;
    }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CCdpCoinPairProposal: public CProposal {
public:
    CCdpCoinPair GOV_CDP_COINPAIR;
    CdpCoinPairStatus status; // cdp coin pair status, can not be NONE

    CCdpCoinPairProposal(): CProposal(ProposalType::GOV_CDP_COINPAIR){}

    IMPLEMENT_SERIALIZE(
        READWRITE(GOV_CDP_COINPAIR);
        READWRITE(VARINT((uint8_t&)status));
    );


    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("GOV_CDP_COINPAIR", GOV_CDP_COINPAIR.ToString()));

        o.push_back(Pair("status", GetCdpCoinPairStatusName(status))) ;
        return o ;
    }

    string ToString() override {
        return  strprintf("GOV_CDP_COINPAIR=%s", GOV_CDP_COINPAIR.ToString()) + ", " +
                strprintf("status=%s", GetCdpCoinPairStatusName(status));
    }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CAssetProposal: public CProposal {

};

struct CAccountProposal: public CProposal {
    uint64_t account_perms_sums;

    CAccountProposal(): CProposal(ProposalType::GOV_CDP_COINPAIR){}

    IMPLEMENT_SERIALIZE(
        READWRITE(GOV_CDP_COINPAIR);
        READWRITE(VARINT((uint8_t&)status));
    );


    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("GOV_CDP_COINPAIR", GOV_CDP_COINPAIR.ToString()));

        o.push_back(Pair("status", GetCdpCoinPairStatusName(status))) ;
        return o ;
    }

    string ToString() override {
        return  strprintf("GOV_CDP_COINPAIR=%s", GOV_CDP_COINPAIR.ToString()) + ", " +
                strprintf("status=%s", GetCdpCoinPairStatusName(status));
    }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;
}

enum ChainType: uint8_t {
    NULL_CHAIN_TYPE = 0,
    BITCOIN         = 1,
    ETHEREUM        = 2,
    EOS             = 3
};

// default setting before deGov
static const std::map<TokenSymbol, TokenSymbol> kXChainSwapTokenMap =  {
    { SYMB::BTC,     SYMB::WBTC  },
    { SYMB::ETH,     SYMB::WETH  },
    { SYMB::WBTC,    SYMB::BTC   },
    { SYMB::WETH,    SYMB::ETH   },   
    // {SYMB::ETH_USDT, SYMB::WETH_USDT},
};

class CXChainSwapInProposal: public CProposal {
public:
    ChainType   peer_chain_type = ChainType::BITCOIN;  //redudant, reference only
    TokenSymbol peer_chain_token_symbol; // from kXChainSwapTokenMap to get the target token symbol
    string      peer_chain_uid;  // initiator's address at peer chain
    string      peer_chain_txid; // a proof from the peer chain (non-HTLC version)
    
    CUserID     self_chain_uid;
    uint64_t    swap_amount;

    CXChainSwapInProposal(): CProposal(ProposalType::GOV_AXC_IN) {}
    CXChainSwapInProposal(ChainType peerChainType, TokenSymbol peerChainTokenSymbol, string &peerChainUid, string &peerChainTxid,
                        CUserID &selfChainUid, uint64_t &swapAmount): CProposal(ProposalType::GOV_AXC_IN), 
                        peer_chain_type(peerChainType), 
                        peer_chain_token_symbol(peerChainTokenSymbol), 
                        peer_chain_uid(peerChainUid),
                        peer_chain_txid(peerChainTxid),
                        self_chain_uid(selfChainUid), 
                        swap_amount(swapAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE((uint8_t &)peer_chain_type);
        READWRITE(peer_chain_token_symbol);
        READWRITE(peer_chain_uid);
        READWRITE(peer_chain_txid);
        READWRITE(self_chain_uid);
        READWRITE(VARINT(swap_amount));
    );

    Object ToJson() override {
        Object obj = CProposal::ToJson();
        obj.push_back(Pair("peer_chain_type", peer_chain_type));
        obj.push_back(Pair("peer_chain_token_symbol", peer_chain_token_symbol));
        obj.push_back(Pair("peer_chain_uid", peer_chain_uid));
        obj.push_back(Pair("peer_chain_txid", peer_chain_txid));
        obj.push_back(Pair("self_chain_uid", self_chain_uid.ToString()));
        obj.push_back(Pair("swap_amount", ValueFromAmount(swap_amount)));
        return obj;
    }

    string ToString() override {
        return  strprintf("peer_chain_type=%d, peer_chain_token_symbol=%s, peer_chain_uid=%, peer_chain_txid=%, self_chain_uid=%s, swap_amount=%llu",
                        peer_chain_type, peer_chain_token_symbol, peer_chain_uid, peer_chain_txid, self_chain_uid.ToString(), swap_amount);
    }
    
    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CXChainSwapOutProposal: public CProposal {
public:
    CUserID     self_chain_uid;  // swap-out initiator's address 
    TokenSymbol self_chain_token_symbol; // from kXChainSwapTokenMap to get the target token symbol

    ChainType   peer_chain_type = ChainType::BITCOIN; //redudant, reference only
    string      peer_chain_uid;  // swap-out peer-chain address
    uint64_t    swap_amount;
    
    
    CXChainSwapOutProposal(): CProposal(ProposalType::GOV_AXC_OUT) {}
    CXChainSwapOutProposal(CUserID &uid, TokenSymbol selfChainTokenSymbol, ChainType peerChainType, string &peerChainUid,
                        uint64_t &swapAmount): CProposal(ProposalType::GOV_AXC_OUT), 
                        self_chain_uid(uid), 
                        self_chain_token_symbol(selfChainTokenSymbol), 
                        peer_chain_type(peerChainType),
                        peer_chain_uid(peerChainUid),
                        swap_amount(swapAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE(self_chain_uid);
        READWRITE(self_chain_token_symbol);
        READWRITE((uint8_t &)peer_chain_type);
        READWRITE(peer_chain_uid);
        READWRITE(VARINT(swap_amount));
    );

     Object ToJson() override {
        Object obj = CProposal::ToJson();
        obj.push_back(Pair("self_chain_uid", self_chain_uid.ToString()));
        obj.push_back(Pair("self_chain_token_symbol", self_chain_token_symbol));
        obj.push_back(Pair("peer_chain_type", peer_chain_type));
        obj.push_back(Pair("peer_chain_uid", peer_chain_uid));
        obj.push_back(Pair("swap_amount", ValueFromAmount(swap_amount)));
        return obj;
    }

    string ToString() override {
        return  strprintf("self_chain_uid=%s, self_chain_token_symbol=%s, peer_chain_type=%d, peer_chain_uid=%, swap_amount=%llu",
                        self_chain_uid.ToString(), self_chain_token_symbol, peer_chain_type, peer_chain_uid, swap_amount);
    }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context) override;

};

class CProposalStorageBean {

public:
    shared_ptr<CProposal> sp_proposal ;

    CProposalStorageBean() {}

    CProposalStorageBean( shared_ptr<CProposal> ptr): sp_proposal(ptr) {}

    bool IsEmpty() const { return sp_proposal == nullptr; }
    void SetEmpty() { sp_proposal = nullptr; }

    string ToString() const {
        return sp_proposal->ToString();
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        if(IsEmpty())
            return 1 ;
  
        return (*sp_proposal).GetSerializeSize(nType, nVersion) + 1 ;
    }

    template <typename Stream>
    void Serialize(Stream &os, int nType, int nVersion) const {
        uint8_t proposalType = ProposalType ::NULL_PROPOSAL ;

        if (!IsEmpty())
            proposalType = sp_proposal->proposal_type ;

        ::Serialize(os, (uint8_t&) proposalType, nType, nVersion);

        if (IsEmpty())
            return ;

        switch (sp_proposal->proposal_type) {

            case GOV_SYS_PARAM:
                ::Serialize(os, *((CParamsGovernProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_CDP_COINPAIR:
                ::Serialize(os, *((CCdpCoinPairProposal    *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_CDP_PARAM:
                ::Serialize(os, *((CCdpParamGovernProposal *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_BPMC_LIST:
                ::Serialize(os, *((CGovernorUpdateProposal *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_DEX_OP:
                ::Serialize(os, *((CDexSwitchProposal      *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_MINER_FEE:
                ::Serialize(os, *((CMinerFeeProposal       *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_COIN_TRANSFER:
                ::Serialize(os, *((CCoinTransferProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_BP_SIZE:
                ::Serialize(os, *((CBPCountUpdateProposal  *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_DEX_QUOTE:
                ::Serialize(os, *((CDexQuoteCoinProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_FEED_COINPAIR:
                ::Serialize(os, *((CFeedCoinPairProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_AXC_IN:
                ::Serialize(os, *((CXChainSwapInProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_AXC_OUT:
                ::Serialize(os, *((CXChainSwapOutProposal  *) (sp_proposal.get())), nType, nVersion);
                break;

            default:
                throw ios_base::failure(strprintf("Serialize: proposalType(%d) error.",
                                                  sp_proposal->proposal_type));
        }
    }

    template <typename Stream>
    void Unserialize(Stream &is, int nType, int nVersion) {

        uint8_t nProposalTye;
        is.read((char *)&(nProposalTye), sizeof(nProposalTye));
        ProposalType proposalType = (ProposalType)nProposalTye ;
        if (proposalType == ProposalType:: NULL_PROPOSAL)
            return ;

        switch(proposalType) {
            case GOV_SYS_PARAM: {
                sp_proposal = std::make_shared<CParamsGovernProposal>();
                ::Unserialize(is, *((CParamsGovernProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_CDP_COINPAIR: {
                sp_proposal = std::make_shared<CCdpCoinPairProposal>();
                ::Unserialize(is, *((CCdpCoinPairProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_CDP_PARAM: {
                sp_proposal = std::make_shared<CCdpParamGovernProposal>();
                ::Unserialize(is, *((CCdpParamGovernProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_BPMC_LIST: {
                sp_proposal = std::make_shared<CGovernorUpdateProposal>();
                ::Unserialize(is, *((CGovernorUpdateProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_DEX_OP: {
                sp_proposal = std::make_shared<CDexSwitchProposal>();
                ::Unserialize(is, *((CDexSwitchProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_MINER_FEE: {
                sp_proposal = std::make_shared<CMinerFeeProposal>();
                ::Unserialize(is, *((CMinerFeeProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_COIN_TRANSFER: {
                sp_proposal = std:: make_shared<CCoinTransferProposal>();
                ::Unserialize(is,  *((CCoinTransferProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_BP_SIZE: {
                sp_proposal = std:: make_shared<CBPCountUpdateProposal>();
                ::Unserialize(is,  *((CBPCountUpdateProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_DEX_QUOTE: {
                sp_proposal = std:: make_shared<CDexQuoteCoinProposal>();
                ::Unserialize(is,  *((CDexQuoteCoinProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_FEED_COINPAIR: {
                sp_proposal = std:: make_shared<CFeedCoinPairProposal>();
                ::Unserialize(is,  *((CFeedCoinPairProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_AXC_IN: {
                sp_proposal = std:: make_shared<CXChainSwapInProposal>();
                ::Unserialize(is,  *((CXChainSwapInProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_AXC_OUT: {
                sp_proposal = std:: make_shared<CXChainSwapOutProposal>();
                ::Unserialize(is,  *((CXChainSwapOutProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            default:
                throw ios_base::failure(strprintf("Unserialize: nTxType(%d) error.",
                                                  nProposalTye));
        }

        sp_proposal->proposal_type = proposalType;
    }
};


#endif //ENTITIES_PROPOSAL_H