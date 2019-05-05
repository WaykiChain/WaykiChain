// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php


#ifndef PRICE_FEED_H
#define PRICE_FEED_H

#include "tx/tx.h"

class CPriceFeedTx : public CBaseTx {
public:
    unsigned char coinType;
    unsigned char priceType;
    uint64_t price;

public:
    CPriceFeedTx(): CBaseTx(PRICE_FEED_TX) { price = 0; }
    CPriceFeedTx(const CBaseTx *pBaseTx): CBaseTx(PRICE_FEED_TX) {
        assert(PRICE_FEED_TX == pBaseTx->nTxType);
        *this = *(CPriceFeedTx *)pBaseTx;
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn,
                CoinType coinTypeIn, PriceType priceTypeIn, uint64_t price):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feeIn) {

        coinType = coinTypeIn;
        priceType = priceTypeIn;
        price     = priceIn;
    }
    ~CPriceFeedTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(coinType);
        READWRITE(priceType);
        READWRITE(VARINT(price));)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << coinType << priceType << VARINT(price);

            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetValue() const { return rewardValue; }
    uint256 GetHash() const { return SignatureHash(); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CPriceFeedTx>(this); }
    uint64_t GetFee() const { return 0; }
    double GetPriority() const { return 0.0f; }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                       CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                       CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
        return true;
    }
};


#endif