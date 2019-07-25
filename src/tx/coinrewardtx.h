// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_COIN_REWARD_H
#define TX_COIN_REWARD_H

#include "entities/asset.h"
#include "tx.h"

class CCoinRewardTx : public CBaseTx {
public:
    TokenSymbol coin_symbol; //default: WICC
    uint64_t coin_amount;

public:
    CCoinRewardTx() : CBaseTx(UCOIN_REWARD_TX), coin_symbol(SYMB::WICC), coin_amount(0) {}

    CCoinRewardTx(const CBaseTx *pBaseTx) : CBaseTx(UCOIN_REWARD_TX), coin_symbol(SYMB::WICC), coin_amount(0) {
        assert(UCOIN_REWARD_TX == pBaseTx->nTxType);
        *this = *(CCoinRewardTx *)pBaseTx;
    }

    CCoinRewardTx(const CUserID &txUidIn, const int32_t nValidHeightIn,
                const TokenSymbol &coinSymbol, const uint64_t coinAmount)
        : CBaseTx(UCOIN_REWARD_TX) {
        txUid        = txUidIn;
        nValidHeight = nValidHeightIn;
        coin_symbol  = coinSymbol;
        coin_amount  = coinAmount;
    }

    ~CCoinRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(txUid);
        READWRITE(VARINT(nValidHeight));

        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << txUid << VARINT(nValidHeight)
                << coin_symbol << VARINT(coin_amount);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{coin_symbol, coin_amount}}; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCoinRewardTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

#endif  // TX_COIN_REWARD_H
