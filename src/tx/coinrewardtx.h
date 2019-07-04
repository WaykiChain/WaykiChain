// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_SCOIN_REWARD_H
#define TX_SCOIN_REWARD_H

#include "tx.h"

class CCoinRewardTx : public CBaseTx {
public:
    uint8_t coinType;
    uint64_t coinValue;
    int height;

public:
    CCoinRewardTx() : CBaseTx(BLOCK_REWARD_TX), coinType(CoinType::WICC), coinValue(0), height(0) {}
    CCoinRewardTx(const CBaseTx *pBaseTx)
        : CBaseTx(BLOCK_REWARD_TX), coinType(CoinType::WICC), coinValue(0), height(0) {
        assert(BLOCK_REWARD_TX == pBaseTx->nTxType);
        *this = *(CCoinRewardTx *)pBaseTx;
    }
    CCoinRewardTx(const CUserID &txUidIn, const CoinType coinTypeIn, const uint64_t coinValueIn, const int nHeightIn)
        : CBaseTx(BLOCK_REWARD_TX) {
        txUid     = txUidIn;
        coinValue = coinValueIn;
        height    = nHeightIn;
    }
    ~CCoinRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(height));
        READWRITE(txUid);

        READWRITE(coinType);
        READWRITE(VARINT(coinValue));
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(height) << txUid << coinType << VARINT(coinValue);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const {
        return map<CoinType, uint64_t>{{CoinType(coinType), coinValue}};
    }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCoinRewardTx>(this); }
    uint64_t GetFee() const { return 0; }
    double GetPriority() const { return 0.0f; }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int height, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif // TX_SCOIN_REWARD_H
