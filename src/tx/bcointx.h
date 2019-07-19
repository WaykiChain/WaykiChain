// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef BASECOIN_H
#define BASECOIN_H

#include "tx/tx.h"

class CBaseCoinTransferTx : public CBaseTx {
public:
    mutable CUserID toUid;  // Recipient Regid or Keyid
    uint64_t bcoins;        // transfer amount
    UnsignedCharArray memo;

public:
    CBaseCoinTransferTx(): CBaseTx(BCOIN_TRANSFER_TX) { }

    CBaseCoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(BCOIN_TRANSFER_TX) {
        assert(BCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CBaseCoinTransferTx *)pBaseTx;
    }

    CBaseCoinTransferTx(const CUserID &txUidIn, CUserID toUidIn, uint64_t feesIn, uint64_t valueIn,
              int validHeightIn, UnsignedCharArray &memoIn) :
              CBaseTx(BCOIN_TRANSFER_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());
        else if (txUidIn.type() == typeid(CPubKey))
            assert(txUidIn.get<CPubKey>().IsFullyValid());

        if (toUidIn.type() == typeid(CRegID))
            assert(!toUidIn.get<CRegID>().IsEmpty());

        toUid   = toUidIn;
        bcoins  = valueIn;
        memo    = memoIn;
    }

    CBaseCoinTransferTx(const CUserID &txUidIn, CUserID toUidIn, uint64_t feesIn, uint64_t valueIn,
              int validHeightIn): CBaseTx(BCOIN_TRANSFER_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());
        else if (txUidIn.type() == typeid(CPubKey))
            assert(txUidIn.get<CPubKey>().IsFullyValid());

        if (toUidIn.type() == typeid(CRegID))
            assert(!toUidIn.get<CRegID>().IsEmpty());

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

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << toUid
               << VARINT(llFees) << VARINT(bcoins) << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, bcoins}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBaseCoinTransferTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};


#endif