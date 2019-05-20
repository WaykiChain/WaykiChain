// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef FCOIN_STAKE_H
#define FCOIN_STAKE_H

#include "tx.h"

class CFcoinStakeTx: CBaseTx {

private:
    uint64_t fcoinsToStake;

public:
    CFcoinStakeTx(): CBaseTx(FCOIN_STAKE_TX) {}

    CFcoinStakeTx(const CBaseTx *pBaseTx): CBaseTx(FCOIN_STAKE_TX) {
        assert(FCOIN_STAKE_TX == pBaseTx->nTxType);
        *this = *(CFcoinStakeTx *) pBaseTx;
    }

    CFcoinStakeTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn, uint64_t bcoinsIn):
        CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feeIn) {
        txUid = txUidIn;
        bcoins = bcoinsIn;
    }

    ~CFcoinStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(fcoinsToStake));
        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
               << VARINT(fcoinsToStake);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CFcoinStakeTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(     int nIndex,
                        CAccountViewCache &view,
                        CValidationState &state,
                        CTxUndo &txundo,
                        int nHeight,
                        CTransactionDBCache &txCache,
                        CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx( int nIndex,
                        CAccountViewCache &view,
                        CValidationState &state,
                        CTxUndo &txundo,
                        int nHeight,
                        CTransactionDBCache &txCache,
                        CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};
#endif