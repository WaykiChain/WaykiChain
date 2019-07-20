// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_MEDIAN_H
#define TX_PRICE_MEDIAN_H

#include "tx.h"

class CBlockPriceMedianTx: public CBaseTx  {
private:
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
            CPricePoint pricePoint(it->first, it->second);
            READWRITE(pricePoint);
        };
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << mapMedianPricePoints;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBlockPriceMedianTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

public:
    bool SetMedianPricePoints(map<CCoinPriceType, uint64_t> &mapMedianPricePointsIn) {
        mapMedianPricePoints = mapMedianPricePointsIn;
        return true;
    }

    map<CCoinPriceType, uint64_t> GetMedianPrice() const;

};

#endif //TX_PRICE_MEDIAN_H