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
private:
    mutable CUserID toUid;
    uint64_t amount;
    TokenSymbol coin_symbol;
    TokenSymbol fee_symbol;
    UnsignedCharArray memo;

public:
    CCoinTransferTx()
        : CBaseTx(UCOIN_TRANSFER_TX), amount(0), coin_symbol(SYMB::WICC), fee_symbol(SYMB::WICC) {}

    CCoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(UCOIN_TRANSFER_TX) {
        assert(UCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CCoinTransferTx *) pBaseTx;
    }

    CCoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn, uint64_t coinsIn,
                   CoinType coinTypeIn, uint64_t feesIn, CoinType feesCoinTypeIn, UnsignedCharArray &memoIn)
        : CBaseTx(UCOIN_TRANSFER_TX, txUidIn, validHeightIn, feesIn) {
        toUid        = toUidIn;
        coins        = coinsIn;
        coinType     = coinTypeIn;
        feesCoinType = feesCoinTypeIn;
        memo         = memoIn;
    }

    CCoinTransferTx(const CUserID &txUidIn, const CUserID &toUidIn, int32_t validHeightIn, uint64_t coinsIn,
                   CoinType coinTypeIn, uint64_t feesIn, CoinType feesCoinTypeIn)
        : CBaseTx(UCOIN_TRANSFER_TX, txUidIn, validHeightIn, feesIn) {
        toUid        = toUidIn;
        coins        = coinsIn;
        coinType     = coinTypeIn;
        feesCoinType = feesCoinTypeIn;
    }

    ~CCoinTransferTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(toUid);
        READWRITE(VARINT(coins));
        READWRITE((uint8_t &)coinType);
        READWRITE(VARINT(llFees));
        READWRITE((uint8_t &)feesCoinType);
        READWRITE(memo);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << toUid << VARINT(coins)
               << uint8_t(coinType) << VARINT(llFees) << uint8_t(feesCoinType) << memo;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType(coinType), coins}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCoinTransferTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif // TX_COIN_TRANSFER_H