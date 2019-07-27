// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_MEDIAN_H
#define TX_PRICE_MEDIAN_H

#include "tx.h"

class CBlockPriceMedianTx: public CBaseTx  {
private:
    map<CoinPricePair, uint64_t> median_price_points;

public:
    CBlockPriceMedianTx(): CBaseTx(PRICE_MEDIAN_TX) {}

    CBlockPriceMedianTx(const CBaseTx *pBaseTx): CBaseTx(PRICE_MEDIAN_TX) {
        assert(PRICE_MEDIAN_TX == pBaseTx->nTxType);
        *this = *(CBlockPriceMedianTx *)pBaseTx;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(median_price_points);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << median_price_points;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBlockPriceMedianTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);

public:
    bool SetMedianPricePoints(map<CoinPricePair, uint64_t> &mapMedianPricePointsIn) {
        median_price_points = mapMedianPricePointsIn;
        return true;
    }

    map<CoinPricePair, uint64_t> GetMedianPrice() const;

};

#endif //TX_PRICE_MEDIAN_H