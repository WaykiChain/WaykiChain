// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_ACCOUNTPERMSCLEARTX_H
#define TX_ACCOUNTPERMSCLEARTX_H


#include "tx.h"

class CAccountPermsClearTx: public CBaseTx {


public:

    CAccountPermsClearTx()
            : CBaseTx(ACCOUNT_PERMS_CLEAR_TX) {}

    CAccountPermsClearTx(const CUserID &txUidIn, const int32_t validHeightIn, const TokenSymbol &feeSymbol, const uint64_t feesIn)
            : CBaseTx(ACCOUNT_PERMS_CLEAR_TX, txUidIn, validHeightIn, feeSymbol, feesIn) {}

    ~CAccountPermsClearTx() {}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(this->nVersion));
            nVersion = this->nVersion;
            READWRITE(VARINT(valid_height));
            READWRITE(txUid);
            READWRITE(fee_symbol);
            READWRITE(VARINT(llFees));
            READWRITE(signature);)

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
           << fee_symbol << VARINT(llFees);
    }

    std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CAccountPermsClearTx>(*this); }
    string ToString(CAccountDBCache &accountCache) {
        return CBaseTx::ToString(accountCache);
    }
    Object ToJson(CCacheWrapper &cw) const {
        return CBaseTx::ToJson(cw);
    }

    bool CheckTx(CTxExecuteContext &context);
    bool ExecuteTx(CTxExecuteContext &context);
};


#endif //TX_ACCOUNTPERMSCLEARTX_H
