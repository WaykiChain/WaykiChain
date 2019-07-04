// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef SCOIN_H
#define SCOIN_H

#include "tx.h"

class CScoinTransferTx: public CBaseTx {
private:
    mutable CUserID toUid;
    uint64_t scoins;
    uint8_t feesCoinType;  // default: WICC
    UnsignedCharArray memo;

public:
    CScoinTransferTx(): CBaseTx(SCOIN_TRANSFER_TX), scoins(0), feesCoinType(CoinType::WICC) {}

    CScoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(SCOIN_TRANSFER_TX) {
        assert(SCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CScoinTransferTx *) pBaseTx;
    }

    CScoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn, uint64_t feesIn,
                     CoinType feesCoinTypeIn, uint64_t scoinIn, UnsignedCharArray &memoIn)
        : CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        toUid        = toUidIn;
        feesCoinType = feesCoinTypeIn;
        scoins       = scoinIn;
        memo         = memoIn;
    }

    CScoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn, uint64_t feesIn,
                     CoinType feesCoinTypeIn, uint64_t scoinIn)
        : CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        toUid        = toUidIn;
        feesCoinType = feesCoinTypeIn;
        scoins       = scoinIn;
    }

    ~CScoinTransferTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(toUid);
        READWRITE(VARINT(llFees));
        READWRITE(feesCoinType);
        READWRITE(VARINT(scoins));
        READWRITE(memo);
        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
               << toUid << VARINT(llFees) << feesCoinType << VARINT(scoins);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WUSD, scoins}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CScoinTransferTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif