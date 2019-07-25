// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_COIN_TRANSFER_H
#define TX_COIN_TRANSFER_H

#include "tx.h"

/**
 * Universal Coin Transfer Tx
 *
 */
class CCoinTransferTx: public CBaseTx {
public:
    mutable CUserID toUid;
    TokenSymbol coin_symbol;
    uint64_t coin_amount;
    TokenSymbol fee_symbol;
    UnsignedCharArray memo;

public:
    CCoinTransferTx()
        : CBaseTx(UCOIN_TRANSFER_TX), coin_symbol(SYMB::WICC), coin_amount(0), fee_symbol(SYMB::WICC) {}

    CCoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(UCOIN_TRANSFER_TX) {
        assert(UCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CCoinTransferTx *) pBaseTx;
    }

    CCoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn,
                    const TokenSymbol &coinSymbol, uint64_t coinAmount, const TokenSymbol &feeSymbol, uint64_t feesIn,
                    const UnsignedCharArray &memoIn)
        : CBaseTx(UCOIN_TRANSFER_TX, txUidIn, validHeightIn, feesIn) {
        toUid        = toUidIn;
        coin_amount  = coinAmount;
        coin_symbol  = coinSymbol;
        fee_symbol   = feeSymbol;
        memo         = memoIn;
    }

    ~CCoinTransferTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(toUid);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(memo);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << toUid << coin_symbol
               << VARINT(coin_amount) << fee_symbol << VARINT(llFees) << memo;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{coin_symbol, coin_amount}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCoinTransferTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

#endif // TX_COIN_TRANSFER_H