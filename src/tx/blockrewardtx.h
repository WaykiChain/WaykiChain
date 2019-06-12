// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_BLOCK_REWARD_H
#define TX_BLOCK_REWARD_H

#include "tx.h"

class CBlockRewardTx : public CBaseTx {
public:
    uint64_t rewardValue;
    int nHeight;

public:
    CBlockRewardTx(): CBaseTx(BLOCK_REWARD_TX) { rewardValue = 0; }
    CBlockRewardTx(const CBaseTx *pBaseTx): CBaseTx(BLOCK_REWARD_TX) {
        assert(BLOCK_REWARD_TX == pBaseTx->nTxType);
        *this = *(CBlockRewardTx *)pBaseTx;
    }
    CBlockRewardTx(const vector_unsigned_char &accountIn, const uint64_t rewardValueIn, const int nHeightIn):
        CBaseTx(BLOCK_REWARD_TX) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        rewardValue = rewardValueIn;
        nHeight     = nHeightIn;
    }
    ~CBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(txUid);

        // Do NOT change the order.
        READWRITE(VARINT(rewardValue));
        READWRITE(VARINT(nHeight));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << txUid << VARINT(rewardValue) << VARINT(nHeight);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint64_t GetValue() const { return rewardValue; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBlockRewardTx>(this); }
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
