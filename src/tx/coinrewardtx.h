// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_COIN_REWARD_H
#define TX_COIN_REWARD_H

#include "tx.h"

class CCoinRewardTx : public CBaseTx {
public:
    uint8_t coinType;
    uint64_t coins;  // default: WICC

public:
    CCoinRewardTx() : CBaseTx(UCOIN_REWARD_TX), coinType(CoinType::WICC), coins(0) {}

    CCoinRewardTx(const CBaseTx *pBaseTx) : CBaseTx(UCOIN_REWARD_TX), coinType(CoinType::WICC), coins(0) {
        assert(UCOIN_REWARD_TX == pBaseTx->nTxType);
        *this = *(CCoinRewardTx *)pBaseTx;
    }

    CCoinRewardTx(const CUserID &txUidIn, const CoinType coinTypeIn, const uint64_t coinsIn, const int32_t nValidHeightIn)
        : CBaseTx(UCOIN_REWARD_TX) {
        txUid        = txUidIn;
        coinType     = coinTypeIn;
        coins        = coinsIn;
        nValidHeight = nValidHeightIn;
    }

    ~CCoinRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(VARINT(coins));
        READWRITE(coinType);
        // READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(coins) << coinType;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType(coinType), coins}}; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCoinRewardTx>(this); }
    uint64_t GetFee() const { return 0; }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

#endif  // TX_COIN_REWARD_H
