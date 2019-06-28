// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FCOIN_H
#define FCOIN_H

#include "tx.h"

class CFcoinTransferTx: public CBaseTx {
private:
    mutable CUserID toUid;
    uint64_t fcoins;
    UnsignedCharArray memo;

public:
    CFcoinTransferTx(): CBaseTx(FCOIN_TRANSFER_TX), fcoins(0) {}

    CFcoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(FCOIN_TRANSFER_TX) {
        assert(FCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CFcoinTransferTx *) pBaseTx;
    }

    CFcoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn, uint64_t feesIn,
                     uint64_t fcoinsIn, UnsignedCharArray &memoIn)
        : CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        toUid  = toUidIn;
        fcoins = fcoinsIn;
        memo   = memoIn;
    }

    CFcoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn, uint64_t feesIn,
                     uint64_t fcoinsIn)
        : CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        toUid  = toUidIn;
        fcoins = fcoinsIn;
    }

    ~CFcoinTransferTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(toUid);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(fcoins));
        READWRITE(memo);
        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
               << toUid << VARINT(llFees) << VARINT(fcoins);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CFcoinTransferTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif