// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRICE_FEED_H
#define PRICE_FEED_H

#include "tx/tx.h"

class CPricePoint {
private:
    unsigned char coinType;
    unsigned char priceType;
    uint64_t price;

public:

    string ToString() {
        return strprintf("coinType:%u, priceType:%u, price:%lld", coinType, priceType, price);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinType);
        READWRITE(priceType);
        READWRITE(VARINT(price));)
};

class CPriceFeedTx : public CBaseTx {
private:
    vector<CPricePoint> pricePoints;

public:
    CPriceFeedTx(): CBaseTx(PRICE_FEED_TX) {}
    CPriceFeedTx(const CBaseTx *pBaseTx): CBaseTx(PRICE_FEED_TX) {
        assert(PRICE_FEED_TX == pBaseTx->nTxType);
        *this = *(CPriceFeedTx *)pBaseTx;
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn,
                const CPricePoint &pricePointIn):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feeIn) {
        pricePoints.push_back(pricePointIn);
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn,
                const vector<CPricePoint> &pricePointsIn):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feeIn) {
        if (pricePoints.size() > 3 || pricePoints.size() == 0)
            return; // limit max # of price points to be three in one shot

        pricePoints = pricePointsIn;
    }

    ~CPriceFeedTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        for(auto const& pricePoint: pricePoints) {
            READWRITE(pricePoint);
        };)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << pricePoints;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CPriceFeedTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    virtual bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                    int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    virtual bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                    CScriptDBViewCache &scriptDB);

    virtual string ToString(CAccountViewCache &view) const; //logging usage
    virtual Object ToJson(const CAccountViewCache &AccountView) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);

};


#endif