// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef NICKID_REGISTER_H
#define NICKID_REGISTER_H

#include "tx.h"


class CNickIdRegisterTx : public CBaseTx {
public:
    string nickId;

    CNickIdRegisterTx(const CUserID &txUidIn, const string nickIdin, int64_t feesIn, const TokenSymbol& feeSymbol, int32_t validHeightIn) :
            CBaseTx(NICKID_REGISTER_TX, txUidIn, validHeightIn, feeSymbol, feesIn) {
        nickId = nickIdin;
    }
    CNickIdRegisterTx(): CBaseTx(NICKID_REGISTER_TX) {}

    ~CNickIdRegisterTx() {}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(this->nVersion));
            nVersion = this->nVersion;
            READWRITE(VARINT(valid_height));
            READWRITE(txUid);
            READWRITE(fee_symbol);
            READWRITE(nickId);
            READWRITE(VARINT(llFees));
            READWRITE(signature);)

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << nickId
           << VARINT(llFees);
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CNickIdRegisterTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

#endif