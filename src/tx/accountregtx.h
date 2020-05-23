// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ACCOUNT_REGISTER_H
#define ACCOUNT_REGISTER_H

#include "tx.h"

class CAccountRegisterTx : public CBaseTx {
public:
    mutable CUserID miner_uid;  // miner pubkey

public:
    CAccountRegisterTx(const CUserID &txUidIn, const CUserID &minerUidIn, int64_t feesIn, int32_t validHeightIn) :
        CBaseTx(ACCOUNT_REGISTER_TX, txUidIn, validHeightIn, feesIn) {
        miner_uid = minerUidIn;
    }
    CAccountRegisterTx(): CBaseTx(ACCOUNT_REGISTER_TX) {}

    ~CAccountRegisterTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(miner_uid);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << miner_uid << VARINT(llFees);
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CAccountRegisterTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(CCacheWrapper &cw) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

#endif