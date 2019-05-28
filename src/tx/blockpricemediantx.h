// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_MEDIAN_H
#define TX_PRICE_MEDIAN_H

#include "tx.h"


class CBlockPriceMedianTx: public CBaseTx  {
private:
    // map<tuple<CoinType, PriceType>, uint64_t> mapMedianPricePoints;
    map<CCoinPriceType, uint64_t> mapMedianPricePoints;

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

        for (auto it = mapMedianPricePoints.begin(); it != mapMedianPricePoints.end(); ++it) {
            // CoinType coinType  = std::get<0>(it->first);
            // PriceType priceType = std::get<1>(it->first);
            CCoinPriceType CCoinPriceType = it->first;
            uint64_t price = it->second;

            READWRITE(CPricePoint(CCoinPriceType, price));
        };
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << mapMedianPricePoints;

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

#endif //TX_PRICE_MEDIAN_H