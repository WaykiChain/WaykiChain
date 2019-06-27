// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FCOIN_STAKE_H
#define FCOIN_STAKE_H

#include "tx.h"

class CFcoinStakeTx: public CBaseTx {
private:
    int64_t fcoinsToStake; // when negative, it means staking revocation

public:
    CFcoinStakeTx(): CBaseTx(FCOIN_STAKE_TX), fcoinsToStake(0) {}

    CFcoinStakeTx(const CBaseTx *pBaseTx): CBaseTx(FCOIN_STAKE_TX) {
        assert(FCOIN_STAKE_TX == pBaseTx->nTxType);
        *this = *(CFcoinStakeTx *) pBaseTx;
    }

    CFcoinStakeTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn, uint64_t fcoinsToStakeIn):
        CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feesIn),
        fcoinsToStake(fcoinsToStakeIn) {}

    ~CFcoinStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(VARINT(fcoinsToStake));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
               << VARINT(llFees) << VARINT(fcoinsToStake);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CFcoinStakeTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif