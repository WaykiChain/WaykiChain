// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_PROPROSALTX_H
#define TX_PROPROSALTX_H

#include "tx/tx.h"
#include "entities/proposal.h"


class CProposalCreateTx: public CBaseTx {
public:
    CProposalStorageBean proposalBean ;

public:
    CProposalCreateTx(): CBaseTx(PROPOSAL_REQUEST_TX) {}

    CProposalCreateTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                 uint64_t feesIn, CProposalStorageBean proposalIn )
            : CBaseTx(PROPOSAL_REQUEST_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn),
              proposalBean(proposalIn) {}

    ~CProposalCreateTx() {}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(this->nVersion));
            nVersion = this->nVersion;
            READWRITE(VARINT(valid_height));
            READWRITE(txUid);
            READWRITE(VARINT(llFees));
            READWRITE(fee_symbol);
            READWRITE(proposalBean);
            READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << VARINT(llFees)
           << fee_symbol <<proposalBean;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CProposalCreateTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);            // logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const;  // json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};




class CProposalAssentTx: public CBaseTx {
public:
    TxID txid;

public:
    CProposalAssentTx(): CBaseTx(PROPOSAL_APPROVAL_TX) {}

    CProposalAssentTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                      uint64_t feesIn, const TxID& txidIn)
            : CBaseTx(PROPOSAL_APPROVAL_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn),
              txid(txidIn){}

    ~CProposalAssentTx() {}

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


    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CProposalAssentTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);            // logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const;  // json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};


#endif //TX_PROPROSALTX_H
