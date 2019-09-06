// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_FEED_H
#define TX_PRICE_FEED_H

#include "tx.h"

class CPriceFeedTx : public CBaseTx {
public:
    vector<CPricePoint> price_points;

public:
    CPriceFeedTx(): CBaseTx(PRICE_FEED_TX) {}

    CPriceFeedTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                 uint64_t feesIn, const vector<CPricePoint> &pricePointsIn)
        : CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn),
          price_points(pricePointsIn) {}

    ~CPriceFeedTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));

        READWRITE(fee_symbol);
        READWRITE(price_points);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << VARINT(llFees)
               << fee_symbol << price_points;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CPriceFeedTx>(*this); }
    virtual double GetPriority() const { return kPriceFeedTransactionPriority; }    // Top priority
    virtual string ToString(CAccountDBCache &accountCache);            // logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const;  // json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

#endif //TX_PRICE_FEED_H