// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef BASECOIN_H
#define BASECOIN_H

#include "tx/tx.h"

class CBaseCoinTransferTx : public CBaseTx {
public:
    mutable CUserID toUid;  // Recipient Regid or Keyid
    uint64_t bcoins;        // transfer amount
    vector_unsigned_char memo;

public:
    CBaseCoinTransferTx(): CBaseTx(BCOIN_TRANSFER_TX) { }

    CBaseCoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(BCOIN_TRANSFER_TX) {
        assert(BCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CBaseCoinTransferTx *)pBaseTx;
    }

    CBaseCoinTransferTx(const CUserID &txUidIn, CUserID toUidIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn, vector_unsigned_char &descriptionIn) :
              CBaseTx(BCOIN_TRANSFER_TX, validHeightIn, feeIn) {

        //FIXME: need to support public key
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        if (toUidIn.type() == typeid(CRegID))
            assert(!toUidIn.get<CRegID>().IsEmpty());

        txUid   = txUidIn;
        toUid   = toUidIn;
        bcoins  = valueIn;
        memo    = descriptionIn;
    }

    CBaseCoinTransferTx(const CUserID &txUidIn, CUserID toUidIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn): CBaseTx(BCOIN_TRANSFER_TX, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        if (toUidIn.type() == typeid(CRegID))
            assert(!toUidIn.get<CRegID>().IsEmpty());

        txUid  = txUidIn;
        toUid  = toUidIn;
        bcoins = valueIn;
    }

    ~CBaseCoinTransferTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(toUid);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(memo);
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << toUid
               << VARINT(llFees) << VARINT(bcoins) << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetValue() const { return bcoins; }
    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBaseCoinTransferTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                       CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                       CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};


#endif