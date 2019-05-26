// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRICE_FEED_H
#define PRICE_FEED_H

#include "tx.h"

class CoinPriceType {
public:
    unsigned char coinType;
    unsigned char priceType;

    CoinPriceType(CoinType coinTypeIn, PriceType priceTypeIn) :
        coinType(coinTypeIn), priceType(priceTypeIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(coinType);
        READWRITE(priceType);)
};

class CPricePoint {
private:
    CoinPriceType coinPriceType;
    uint64_t price;

public:
    CPricePoint(CoinPriceType coinPriceTypeIn, uint64_t priceIn)
        : coinPriceType(coinPriceTypeIn), price(priceIn) {}

    CPricePoint(CoinType coinTypeIn, PriceType priceTypeIn, uint64_t priceIn)
        : coinPriceType(coinTypeIn, priceTypeIn), price(priceIn) {}

    string ToString() {
        return strprintf("coinType:%u, priceType:%u, price:%lld",
                        coinPriceType.coinType, coinPriceType.priceType, price);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinPriceType);
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
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

    bool GetTopPriceFeederList(CCacheWrapper &cw, vector<CAccount> &priceFeederAccts);

};

class CBlockPriceMedianTx: public CBaseTx  {
private:
    // map<tuple<CoinType, PriceType>, uint64_t> mapMediaPricePoints;
    map<CoinPriceType, uint64_t> mapMediaPricePoints;

public:
    CBlockPriceMedianTx(): CBaseTx(BLOCK_PRICE_MEDIAN_TX) {}

    CBlockPriceMedianTx(const CBaseTx *pBaseTx): CBaseTx(BLOCK_PRICE_MEDIAN_TX) {
        assert(BLOCK_PRICE_MEDIAN_TX == pBaseTx->nTxType);
        *this = *(CBlockPriceMedianTx *)pBaseTx;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        for (auto it = mapMediaPricePoints.begin(); it != mapMediaPricePoints.end(); ++it) {
            // CoinType coinType  = std::get<0>(it->first);
            // PriceType priceType = std::get<1>(it->first);
            CoinPriceType coinPriceType = it->first;
            uint64_t price = it->second;

            READWRITE(CPricePoint(coinPriceType, price));
        };
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << mapMediaPricePoints;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual uint64_t GetValue() const { return 0; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBlockPriceMedianTx>(this); }
    virtual double GetPriority() const { return 1000.0f; }

    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &view) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

    inline uint64_t GetMedianPriceByType(const CoinType coinType, const PriceType priceType);
};

#endif