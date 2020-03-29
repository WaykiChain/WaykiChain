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
#include "entities/receipt.h"
#include "commons/json/json_spirit.h"
#include "config/const.h"
#include "config/txbase.h"
#include "config/scoin.h"
#include "entities/cdp.h"
#include "config/sysparams.h"


class CCacheWrapper;
class CValidationState;
class CTxExecuteContext;

using namespace json_spirit;
using namespace std;

// Proposal for DeGov
enum ProposalType: uint8_t {
    NULL_PROPOSAL       = 0 ,
    GOV_SYS_PARAM       = 1 , // basic parameters
    GOV_BPMC_LIST       = 2 , // update BP Mgmt Committee List
    GOV_BP_SIZE         = 3 , // update BP total number (11 -> 21 -> xxx)
    GOV_MINER_FEE       = 4 , // Miner fees for all Trx types
    GOV_COIN_TRANSFER   = 5 , // for private-key loss, account robbery or for CFT/AML etc purposes
    GOV_ACCOUNT_PERM    = 6 , // update account perms
    GOV_ASSET_PERM      = 7 , // update asset perms
    GOV_CDP_PARAM       = 8 , // CDP parameters
    GOV_DEX_OP          = 9 , // turn on/off DEX operator

    GOV_FEED_COINPAIR   = 11, // BaseSymbol/QuoteSymbol
    GOV_AXC_IN          = 12, // atomic-cross-chain swap in
    GOV_AXC_OUT         = 13, // atomic-cross-chain swap out
    GOV_AXC_COIN        = 14

};

enum ProposalOperateType: uint8_t {
    NULL_PROPOSAL_OP    = 0 ,
    ENABLE              = 1 ,
    DISABLE             = 2
};


struct CProposal {
    ProposalType proposal_type = NULL_PROPOSAL;
    uint32_t expiry_block_height = 0;
    uint8_t approval_min_count = 0;

    CProposal() {}
    CProposal(ProposalType proposalTypeIn) : proposal_type(proposalTypeIn) {}
    virtual std::shared_ptr<CProposal> GetNewInstance() = 0;
    virtual bool CheckProposal(CTxExecuteContext& context) = 0;
    virtual bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) = 0;
    virtual std::string ToString() {
        return strprintf("proposal_type=%d,approval_min_count=%d,expiry_block_height=%d",
                        proposal_type, approval_min_count, expiry_block_height) ;
    }
    virtual Object ToJson(){
        Object o ;
        o.push_back(Pair("proposal_type", proposal_type)) ;
        if(approval_min_count > 0)
            o.push_back(Pair("approval_min_count", approval_min_count)) ;
        if(expiry_block_height > 0)
            o.push_back(Pair("expiry_block_height", expiry_block_height)) ;

        return o ;
    };

    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }
};



struct CGovSysParamProposal: CProposal {
    vector<std::pair<uint8_t, uint64_t>> param_values;

    CGovSysParamProposal(): CProposal(ProposalType::GOV_SYS_PARAM) {}

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

            std::string param_name = "" ;
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

    std::string ToString() override {
        std::string baseString = CProposal::ToString();
        for(auto itr: param_values){
            baseString = strprintf("%s, %s:%d", baseString,itr.first, itr.second ) ;
        }
        return baseString ;
    }

    std::shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovSysParamProposal>(*this); } ;

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};


struct CGovBpMcListProposal: CProposal{
    CRegID gov_bp_regid ;
    ProposalOperateType op_type  = ProposalOperateType::NULL_PROPOSAL_OP;

    CGovBpMcListProposal(): CProposal(ProposalType::GOV_BPMC_LIST){}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE(gov_bp_regid);
        READWRITE((uint8_t&)op_type);
    );

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("gov_bp_regid",gov_bp_regid.ToString())) ;
        o.push_back(Pair("operate_type", op_type));
        return o ;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString() ;
        return strprintf("%s, gov_bp_regid=%s, operate_type=%d", baseString,
                gov_bp_regid.ToString(),op_type) ;

    }
    shared_ptr<CProposal> GetNewInstance() override { return std::make_shared<CGovBpMcListProposal>(*this); }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CGovBpSizeProposal: CProposal {
    uint8_t total_bps_size ;
    uint32_t effective_height ;

    CGovBpSizeProposal(): CProposal(GOV_BP_SIZE) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE(total_bps_size);
        READWRITE(VARINT(effective_height));
    );

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovBpSizeProposal>(*this); }

    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("total_bps_size", (uint64_t)total_bps_size));
        o.push_back(Pair("effective_height",(uint64_t)effective_height));
        return o ;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString();
        return strprintf("%s, total_bps_size=%d, effective_height=%d ",
                         baseString, total_bps_size, effective_height) ;
    }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CGovMinerFeeProposal: CProposal {
    TxType tx_type = TxType::NULL_TX ;
    std::string  fee_symbol = "" ;
    uint64_t  fee_sawi_amount = 0 ;

    CGovMinerFeeProposal() : CProposal(ProposalType::GOV_MINER_FEE) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE((uint8_t&)tx_type);
        READWRITE(fee_symbol);
        READWRITE(VARINT(fee_sawi_amount));
    )
    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovMinerFeeProposal>(*this); }


    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("tx_type", tx_type));
        o.push_back(Pair("fee_symbol", fee_symbol)) ;
        o.push_back(Pair("fee_sawi_amount", fee_sawi_amount));
        return o ;

    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString() ;
        return strprintf("%s, tx_type=%d, fee_symbo=%s ,fee_sawi_amount=%d",
                        baseString, tx_type, fee_symbol,fee_sawi_amount ) ;

    }
};

struct CGovCoinTransferProposal: CProposal {
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


    CGovCoinTransferProposal(): CProposal(ProposalType::GOV_COIN_TRANSFER) {}

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovCoinTransferProposal>(*this); } ;

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

    virtual Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("coin_symbol", token)) ;
        o.push_back(Pair("amount", amount)) ;
        o.push_back(Pair("from_uid", from_uid.ToString())) ;
        o.push_back(Pair("to_uid", to_uid.ToString())) ;
        return o ;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString();
        return baseString ;
    }
};

struct CGovAccountPermProposal: CProposal {
    CUserID account_uid;
    uint64_t proposed_perms_sum;

    CGovAccountPermProposal(): CProposal(ProposalType::GOV_ACCOUNT_PERM){}
    CGovAccountPermProposal(const CUserID& accountUid, const uint64_t& permsSum): CProposal(ProposalType::GOV_ACCOUNT_PERM),
                                 account_uid(accountUid),
                                 proposed_perms_sum(permsSum){}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE(account_uid);
        READWRITE(VARINT((uint64_t&)proposed_perms_sum));
    );

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("account_uid", account_uid.ToString())) ;
        o.push_back(Pair("proposed_perms_sum", proposed_perms_sum)) ;
        return o ;
    }

    std::string ToString() override {

        return  strprintf("account_uid=%s, proposed_perms_sum=%llu",
                        account_uid.ToString(), proposed_perms_sum);
    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovAccountPermProposal>(*this); } ;

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CGovAssetPermProposal: CProposal {
    TokenSymbol asset_symbol;
    uint64_t proposed_perms_sum;

    CGovAssetPermProposal(): CProposal(ProposalType::GOV_ASSET_PERM){}
    CGovAssetPermProposal(const TokenSymbol& assetSymbol, const uint64_t& permsSum): CProposal(ProposalType::GOV_ASSET_PERM),
                                    asset_symbol(assetSymbol),
                                    proposed_perms_sum(permsSum){}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(asset_symbol);
        READWRITE(VARINT((uint64_t&)proposed_perms_sum));
    );

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("asset_symbol", asset_symbol));
        o.push_back(Pair("proposed_perms_sum", proposed_perms_sum)) ;
        return o ;
    }

    std::string ToString() override {
        string baseProposalString = CProposal::ToString();
        return  strprintf("%s, asset_symbol=%s, proposed_perms_sum=%llu",
                        baseProposalString,asset_symbol, proposed_perms_sum);
    }


    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovAssetPermProposal>(*this); };

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CGovCdpParamProposal: CProposal {
    vector<std::pair<uint8_t, uint64_t>> param_values;
    CCdpCoinPair coin_pair ;

    CGovCdpParamProposal(): CProposal(ProposalType::GOV_CDP_PARAM) {}

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

            std::string param_name = "" ;
            auto itr = kCdpParamTable.find(CdpParamType(item.first)) ;
            if(itr != kCdpParamTable.end())
                param_name = std::get<1>(itr->second);

            subItem.push_back(Pair("param_name", param_name));
            subItem.push_back(Pair("param_value", item.second));
            arrayItems.push_back(subItem);
        }

        o.push_back(Pair("params",arrayItems)) ;
        return o ;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString();
        for(auto itr: param_values){
            baseString = strprintf("%s, %s:%d", baseString,itr.first, itr.second ) ;
        }
        return baseString ;
    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovCdpParamProposal>(*this); }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CGovDexOpProposal: CProposal{
    uint32_t dexid;
    ProposalOperateType operate_type = ProposalOperateType::ENABLE;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE(VARINT(dexid));
        READWRITE((uint8_t&)operate_type);
    );

    CGovDexOpProposal(): CProposal(ProposalType::GOV_DEX_OP) {}

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

    shared_ptr<CProposal> GetNewInstance() { return make_shared<CGovDexOpProposal>(*this); } ;

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("dexid",(uint64_t)dexid)) ;
        o.push_back(Pair("operate_type", operate_type));
        return o ;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString() ;
        return strprintf("%s, dexid=%d, operate_type=%d", baseString, dexid, operate_type) ;
    }

};

// base currency : quote currency
struct CGovFeedCoinPairProposal: CProposal {
    TokenSymbol  base_symbol;
    TokenSymbol  quote_symbol = SYMB::USD;
    ProposalOperateType op_type = ProposalOperateType::NULL_PROPOSAL_OP;

    CGovFeedCoinPairProposal(): CProposal(ProposalType::GOV_FEED_COINPAIR) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(base_symbol);
        READWRITE(quote_symbol);
        READWRITE((uint8_t&)op_type);
    )

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("base_symbol", base_symbol));
        o.push_back(Pair("quote_symbol", quote_symbol));

        o.push_back(Pair("op_type", op_type)) ;
        return o ;
    }

    string ToString() override {
        std::string baseString = CProposal::ToString();
        return  strprintf("%s, base_symbol=%s,quote_symbol=%s",baseString, base_symbol, quote_symbol ) + ", " +
                strprintf("op_type=%d", op_type);
    }
    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovFeedCoinPairProposal>(*this); }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};


struct CGovAxcCoinProposal: CProposal {
    TokenSymbol  peer_chain_coin_symbol;
    ChainType  peer_chain_type;
    ProposalOperateType op_type = ProposalOperateType::NULL_PROPOSAL_OP;

    CGovAxcCoinProposal(): CProposal(ProposalType::GOV_AXC_COIN) {}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(expiry_block_height));
            READWRITE(approval_min_count);
            READWRITE(peer_chain_coin_symbol);
            READWRITE((uint8_t&)peer_chain_type);
            READWRITE((uint8_t&)op_type);
    )

    Object ToJson() override {
        Object o = CProposal::ToJson();
        o.push_back(Pair("peer_chain_coin_symbol", peer_chain_coin_symbol));
        o.push_back(Pair("peer_chain_type", peer_chain_type));
        o.push_back(Pair("op_type", op_type));
        return o;
    }

    string ToString() override {
        std::string baseString = CProposal::ToString();
        return  strprintf("%s, peer_chain_coin_symbol=%s,peer_chain_type=%d",baseString, peer_chain_coin_symbol, peer_chain_type ) + ", " +
                strprintf("op_type=%d", op_type);
    }
    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovAxcCoinProposal>(*this); }
    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

//cross-chain swap must be initiated by the peer chain asset holder
struct CGovAxcInProposal: CProposal {
    TokenSymbol peer_chain_token_symbol; // from kXChainSwapInTokenMap to get the target token symbol
    string      peer_chain_addr;  // initiator's address at peer chain
    string      peer_chain_txid; // a proof from the peer chain (non-HTLC version)

    CUserID     self_chain_uid; //must be verified by MC members offchain
    uint64_t    swap_amount;

    CGovAxcInProposal(): CProposal(ProposalType::GOV_AXC_IN) {}
    CGovAxcInProposal( TokenSymbol peerChainTokenSymbol,
                    string &peerChainAddr, string &peerChainTxid, CUserID &selfChainUid, uint64_t &swapAmount):
                    CProposal(ProposalType::GOV_AXC_IN),
                    peer_chain_token_symbol(peerChainTokenSymbol),
                    peer_chain_addr(peerChainAddr),
                    peer_chain_txid(peerChainTxid),
                    self_chain_uid(selfChainUid),
                    swap_amount(swapAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);

        READWRITE(peer_chain_token_symbol);
        READWRITE(peer_chain_addr);
        READWRITE(peer_chain_txid);
        READWRITE(self_chain_uid);
        READWRITE(VARINT(swap_amount));
    );

    Object ToJson() override {
        Object obj = CProposal::ToJson();
        obj.push_back(Pair("peer_chain_token_symbol", peer_chain_token_symbol));
        obj.push_back(Pair("peer_chain_addr", peer_chain_addr));
        obj.push_back(Pair("peer_chain_txid", peer_chain_txid));
        obj.push_back(Pair("self_chain_uid", self_chain_uid.ToString()));
        obj.push_back(Pair("swap_amount", swap_amount));
        return obj;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString();
        return  strprintf("%s, peer_chain_token_symbol=%s, "
                          "peer_chain_addr=%, peer_chain_txid=%, self_chain_uid=%s,swap_amount=%llu",
                         baseString,peer_chain_token_symbol,
                        peer_chain_addr, peer_chain_txid, self_chain_uid.ToString(), swap_amount);
    }
    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovAxcInProposal>(*this); }

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CGovAxcOutProposal: CProposal {
    CUserID     self_chain_uid;
    TokenSymbol self_chain_token_symbol; // from kXChainSwapOutTokenMap to get the target token symbol
    string      peer_chain_addr;  // swap-out peer-chain address (usually different from swap-in fromAddr for bitcoin)
    uint64_t    swap_amount;

    vector<UnsignedCharArray> peer_chain_tx_multisigs; //only filled by approver

    CGovAxcOutProposal(): CProposal(ProposalType::GOV_AXC_OUT) {}
    CGovAxcOutProposal( TokenSymbol selfChainTokenSymbol, string &peerChainAddr,
                        uint64_t &swapAmount): CProposal(ProposalType::GOV_AXC_OUT),
                        self_chain_token_symbol(selfChainTokenSymbol),
                        peer_chain_addr(peerChainAddr),
                        swap_amount(swapAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(expiry_block_height));
        READWRITE(approval_min_count);
        READWRITE(self_chain_uid);
        READWRITE(self_chain_token_symbol);
        READWRITE(peer_chain_addr);
        READWRITE(VARINT(swap_amount));
        READWRITE(peer_chain_tx_multisigs);
    );

    Object ToJson() override {
        Object obj = CProposal::ToJson();
        obj.push_back(Pair("self_chain_token_symbol", self_chain_token_symbol));
        if(!self_chain_uid.is<CNullID>())
            obj.push_back(Pair("self_chain_uid", self_chain_uid.ToString()));
        obj.push_back(Pair("peer_chain_addr", peer_chain_addr));
        obj.push_back(Pair("swap_amount", ValueFromAmount(swap_amount)));
        Array arr;
        for(auto sig: peer_chain_tx_multisigs)
            arr.push_back(HexStr(sig));
        obj.push_back(Pair("signs", arr));
        return obj;
    }

    std::string ToString() override {
        std::string baseString = CProposal::ToString();
        return  strprintf("%s, self_chain_token_symbol=%s, peer_chain_addr=%, swap_amount=%llu",
                        baseString, self_chain_token_symbol, peer_chain_addr, swap_amount);
    }

    shared_ptr<CProposal> GetNewInstance() override { return make_shared<CGovAxcOutProposal>(*this); } ;

    bool CheckProposal(CTxExecuteContext& context) override;
    bool ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) override;

};

struct CProposalStorageBean {
    shared_ptr<CProposal> sp_proposal = nullptr;

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
                ::Serialize(os, *((CGovSysParamProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_CDP_PARAM:
                ::Serialize(os, *((CGovCdpParamProposal *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_BPMC_LIST:
                ::Serialize(os, *((CGovBpMcListProposal *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_DEX_OP:
                ::Serialize(os, *((CGovDexOpProposal      *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_MINER_FEE:
                ::Serialize(os, *((CGovMinerFeeProposal       *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_COIN_TRANSFER:
                ::Serialize(os, *((CGovCoinTransferProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_BP_SIZE:
                ::Serialize(os, *((CGovBpSizeProposal  *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_FEED_COINPAIR:
                ::Serialize(os, *((CGovFeedCoinPairProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_AXC_IN:
                ::Serialize(os, *((CGovAxcInProposal   *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_AXC_OUT:
                ::Serialize(os, *((CGovAxcOutProposal  *) (sp_proposal.get())), nType, nVersion);
                break;
            case GOV_AXC_COIN:
                ::Serialize(os, *((CGovAxcCoinProposal  *) (sp_proposal.get())), nType, nVersion);
                break;

            case GOV_ACCOUNT_PERM:
                ::Serialize(os, *((CGovAccountPermProposal  *) (sp_proposal.get())), nType, nVersion);
                break;

            case GOV_ASSET_PERM:
                ::Serialize(os, *((CGovAssetPermProposal  *) (sp_proposal.get())), nType, nVersion);
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
                sp_proposal = std::make_shared<CGovSysParamProposal>();
                ::Unserialize(is, *((CGovSysParamProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_CDP_PARAM: {
                sp_proposal = std::make_shared<CGovCdpParamProposal>();
                ::Unserialize(is, *((CGovCdpParamProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_BPMC_LIST: {
                sp_proposal = std::make_shared<CGovBpMcListProposal>();
                ::Unserialize(is, *((CGovBpMcListProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_DEX_OP: {
                sp_proposal = std::make_shared<CGovDexOpProposal>();
                ::Unserialize(is, *((CGovDexOpProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_MINER_FEE: {
                sp_proposal = std::make_shared<CGovMinerFeeProposal>();
                ::Unserialize(is, *((CGovMinerFeeProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_COIN_TRANSFER: {
                sp_proposal = std:: make_shared<CGovCoinTransferProposal>();
                ::Unserialize(is,  *((CGovCoinTransferProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_BP_SIZE: {
                sp_proposal = std:: make_shared<CGovBpSizeProposal>();
                ::Unserialize(is,  *((CGovBpSizeProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_FEED_COINPAIR: {
                sp_proposal = std:: make_shared<CGovFeedCoinPairProposal>();
                ::Unserialize(is,  *((CGovFeedCoinPairProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_AXC_IN: {
                sp_proposal = std:: make_shared<CGovAxcInProposal>();
                ::Unserialize(is,  *((CGovAxcInProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_AXC_OUT: {
                sp_proposal = std:: make_shared<CGovAxcOutProposal>();
                ::Unserialize(is,  *((CGovAxcOutProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_AXC_COIN: {
                sp_proposal = std:: make_shared<CGovAxcCoinProposal>();
                ::Unserialize(is,  *((CGovAxcCoinProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }


            case GOV_ACCOUNT_PERM: {
                sp_proposal = std:: make_shared<CGovAccountPermProposal>();
                ::Unserialize(is,  *((CGovAccountPermProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            case GOV_ASSET_PERM: {
                sp_proposal = std:: make_shared<CGovAssetPermProposal>();
                ::Unserialize(is,  *((CGovAssetPermProposal *)(sp_proposal.get())), nType, nVersion);
                break;
            }

            default:
                throw ios_base::failure(strprintf("Unserialize: proposalType(%d) error.",
                                                  nProposalTye));
        }

        sp_proposal->proposal_type = proposalType;
    }
};


#endif //ENTITIES_PROPOSAL_H