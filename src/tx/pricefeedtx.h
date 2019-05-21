// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRICE_FEED_H
#define PRICE_FEED_H

#include "tx/tx.h"
#include "scointx.h"

class CPriceFeedTx : public CBaseTx {
public:
    unsigned char coinType;
    unsigned char priceType;
    uint64_t price;

public:
    CPriceFeedTx(): CBaseTx(PRICE_FEED_TX) {
        coinType = 0;
        priceType = 0;
        price = 0;
    }
    CPriceFeedTx(const CBaseTx *pBaseTx): CBaseTx(PRICE_FEED_TX) {
        assert(PRICE_FEED_TX == pBaseTx->nTxType);
        *this = *(CPriceFeedTx *)pBaseTx;
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn,
                CoinType coinTypeIn, PriceType priceTypeIn, uint64_t priceIn):
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

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << coinType << priceType << VARINT(price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CPriceFeedTx>(this); }

    virtual bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    virtual bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                    int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    virtual bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                    CScriptDBViewCache &scriptDB);

    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountViewCache &view) const;
    virtual Object ToJson(const CAccountViewCache &AccountView) const;
    virtual bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);

    uint64_t GetPrice() const { return price; }
};


#endif