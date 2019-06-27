// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_FEED_H
#define TX_PRICE_FEED_H

#include "tx.h"

class CPriceFeedTx : public CBaseTx {
public:
    vector<CPricePoint> pricePoints;

public:
    CPriceFeedTx(): CBaseTx(PRICE_FEED_TX) {}
    CPriceFeedTx(const CBaseTx *pBaseTx): CBaseTx(PRICE_FEED_TX) {
        assert(PRICE_FEED_TX == pBaseTx->nTxType);
        *this = *(CPriceFeedTx *)pBaseTx;
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                const CPricePoint &pricePointIn):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feesIn) {
        pricePoints.push_back(pricePointIn);
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                const vector<CPricePoint> &pricePointsIn):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feesIn) {
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

        for (auto const& pricePoint: pricePoints) {
            READWRITE(pricePoint);
        })

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << pricePoints;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CPriceFeedTx>(this); }
    virtual double GetPriority() const { return 10000.0f; }  // Top priority
    virtual string ToString(CAccountCache &view);            // logging usage
    virtual Object ToJson(const CAccountCache &view) const;  // json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

    bool GetTopPriceFeederList(CCacheWrapper &cw, vector<CAccount> &priceFeederAccts);

};

#endif //TX_PRICE_FEED_H