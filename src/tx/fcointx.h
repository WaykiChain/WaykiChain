// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FCOIN_H
#define FCOIN_H

#include "tx.h"

class CFCoinRewardTx : public CBaseTx {
public:
    uint64_t rewardValue;
    int nHeight;

public:
    CFCoinRewardTx(): CBaseTx(FCOIN_REWARD_TX) { rewardValue = 0; }

    CFCoinRewardTx(const CBaseTx *pBaseTx): CBaseTx(FCOIN_REWARD_TX) {
        assert(FCOIN_REWARD_TX == pBaseTx->nTxType);
        *this = *(CFCoinRewardTx *)pBaseTx;
    }

    ~CFCoinRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(rewardValue));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nHeight) << txUid << VARINT(rewardValue);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint64_t GetValue() const { return rewardValue; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CFCoinRewardTx>(this); }
    uint64_t GetFee() const { return 0; }
    double GetPriority() const { return 0.0f; }

    virtual string ToString(CAccountCache &accountCache);
    virtual Object ToJson(const CAccountCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif