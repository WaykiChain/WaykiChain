// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_MEDIAN_H
#define TX_PRICE_MEDIAN_H

#include "tx.h"
#include "entities/cdp.h"
#include "entities/price.h"
#include "persistence/cdpdb.h"

class CBlockPriceMedianTx: public CBaseTx  {
private:
    PriceMap median_prices;

public:
    CBlockPriceMedianTx() : CBaseTx(PRICE_MEDIAN_TX) {}
    CBlockPriceMedianTx(const int32_t validHeight) : CBaseTx(PRICE_MEDIAN_TX) { valid_height = validHeight; }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(median_prices);
        // READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << median_prices;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CBlockPriceMedianTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) { return true; }

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

public:
    void SetMedianPrices(PriceMap &mapMedianPricesIn) {
        median_prices = mapMedianPricesIn;
    }

    PriceMap GetMedianPrice() const;

private:
    uint256 GenOrderId(CTxExecuteContext &context, const CUserCDP &cdp, TokenSymbol assetSymbol);

    bool ForceLiquidateCDPCompat(CTxExecuteContext &context, uint64_t bcoinMedianPrice,
        uint64_t fcoinMedianPrice, RatioCDPIdCache::Map &cdps);
    uint256 GenOrderIdCompat(const uint256 &txid, const uint32_t index);

};

#endif //TX_PRICE_MEDIAN_H