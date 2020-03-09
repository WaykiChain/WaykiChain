// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_PROPROSALTX_H
#define TX_PROPROSALTX_H

#include "tx/tx.h"
#include "entities/proposal.h"


class CProposalRequestTx: public CBaseTx {
public:
    CProposalStorageBean proposal;

public:
    CProposalRequestTx(): CBaseTx(PROPOSAL_REQUEST_TX) {}

    CProposalRequestTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                    uint64_t feesIn, CProposalStorageBean proposalIn ) : 
                    CBaseTx(PROPOSAL_REQUEST_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn), proposal(proposalIn) {}

    ~CProposalRequestTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));
        READWRITE(fee_symbol);
        READWRITE(proposal);
        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << VARINT(llFees)
           << fee_symbol <<proposal;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CProposalRequestTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);            // logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const;  // json-rpc usage
    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CProposalApprovalTx: public CBaseTx {
public:
    TxID txid;

public:
    CProposalApprovalTx(): CBaseTx(PROPOSAL_APPROVAL_TX) {}

    CProposalApprovalTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                        uint64_t feesIn, const TxID& txidIn) : 
                        CBaseTx(PROPOSAL_APPROVAL_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn), txid(txidIn) {}

    ~CProposalApprovalTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(txid);
        READWRITE(signature);
    )


    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << VARINT(llFees)
           << fee_symbol << txid ;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CProposalApprovalTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);            // logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const;  // json-rpc usage
    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};


#endif //TX_PROPROSALTX_H
