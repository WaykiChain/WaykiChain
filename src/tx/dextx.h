// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_H
#define TX_DEX_H

#include "tx.h"

class CDEXBuyOrderTx : public CBaseTx {

public:
    CDEXBuyOrderTx() : CBaseTx(DEX_BUY_ORDER_TX) {}

    CDEXBuyOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_BUY_ORDER_TX) {
        assert(DEX_BUY_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyOrderTx *)pBaseTx;
    }

    CDEXBuyOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                const CoinType buyCoinTypeIn, const uint64_t buyAmountIn, const uint64_t bidPriceIn):
        CBaseTx(DEX_BUY_ORDER_TX, txUidIn, validHeightIn, feesIn) {

        buyCoinType = buyCoinTypeIn;
        buyAmount = buyAmountIn;
        bidPrice = bidPriceIn;
    }

    ~CDEXBuyOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(buyCoinType);
        READWRITE(VARINT(buyAmount));
        READWRITE(VARINT(bidPrice));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << buyCoinType << buyAmount << bidPrice;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:

    CoinType buyCoinType;   // target coin type (wicc or micc) to buy with wusd
    uint64_t buyAmount;     // amount of target coins to buy
    uint64_t bidPrice;      // bidding price willing to buy

};

class CDEXSellOrderTx : public CBaseTx {

public:
    CDEXSellOrderTx() : CBaseTx(DEX_SELL_ORDER_TX) {}

    CDEXSellOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_SELL_ORDER_TX) {
        assert(DEX_SELL_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellOrderTx *)pBaseTx;
    }

    CDEXSellOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                const CoinType sellCoinTypeIn, const uint64_t sellAmountIn, const uint64_t askPriceIn):
        CBaseTx(DEX_SELL_ORDER_TX, txUidIn, validHeightIn, feesIn) {

        sellCoinType = sellCoinTypeIn;
        sellAmount = sellAmountIn;
        askPrice = askPriceIn;
    }

    ~CDEXSellOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(sellCoinType);
        READWRITE(VARINT(sellAmount));
        READWRITE(VARINT(askPrice));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << sellCoinType << sellAmount << askPrice;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSellOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:

    CoinType sellCoinType;   // holing coin type (wicc or micc) to sell for wusd
    uint64_t sellAmount;     // amount of holding coins to sell
    uint64_t askPrice;       // asking price willing to sell

};

typedef CRegID TxCord;

struct DEXDealItem  {
    TxCord buyOrderTxCord
    TxCord sellOrderTxCord;
    uint64_t dealPrice;
    uint64_t dealAmount;

    IMPLEMENT_SERIALIZE(
        READWRITE(buyOrderTxCord);
        READWRITE(sellOrderTxCord);
        READWRITE(VARINT(dealPrice));
        READWRITE(VARINT(dealAmount));)
};

class CDEXSettleTx: public CBaseTx {

public:
    CDEXSettleTx() : CBaseTx(DEX_SETTLE_TX) {}

    CDEXSettleTx(const CBaseTx *pBaseTx): CBaseTx(DEX_SETTLE_TX) {
        assert(DEX_SETTLE_TX == pBaseTx->nTxType);
        *this = *(CDEXSettleTx *)pBaseTx;
    }

    CDEXSettleTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                const CoinType sellCoinTypeIn, const uint64_t sellAmountIn, const uint64_t askPriceIn):
        CBaseTx(DEX_SETTLE_TX, txUidIn, validHeightIn, feesIn) {

    }

    ~CDEXSettleTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(dealItems);)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSettleTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:
    vector<DEXDealItem> dealItems;

};
#endif //TX_DEX_H