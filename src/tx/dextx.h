// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_H
#define TX_DEX_H

#include "tx.h"

class CDEXBuyLimitOrderTx : public CBaseTx {

public:
    CDEXBuyLimitOrderTx() : CBaseTx(DEX_BUY_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_BUY_ORDER_TX) {
        assert(DEX_BUY_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyLimitOrderTx *)pBaseTx;
    }

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                   CoinType coinTypeIn, CoinType assetTypeIn,
                   uint64_t assetAmountIn, uint64_t bidPriceIn)
        : CBaseTx(DEX_BUY_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coinType(coinTypeIn),
          assetType(assetTypeIn),
          assetAmount(assetAmountIn),
          bidPrice(bidPriceIn) {}

    ~CDEXBuyLimitOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE((uint8_t)coinType);
        READWRITE((uint8_t)assetType);
        READWRITE(VARINT(assetAmount));
        READWRITE(VARINT(bidPrice));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << assetAmount << bidPrice;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyLimitOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    
public:
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t assetAmount;     //!< amount of target asset to buy
    uint64_t bidPrice;      //!< bidding price in coinType willing to buy

};

class CDEXSellLimitOrderTx : public CBaseTx {

public:
    CDEXSellLimitOrderTx() : CBaseTx(DEX_SELL_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_SELL_ORDER_TX) {
        assert(DEX_SELL_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellLimitOrderTx *)pBaseTx;
    }

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                    CoinType coinTypeIn, CoinType assetTypeIn,
                    uint64_t assetAmountIn, uint64_t askPriceIn)
        : CBaseTx(DEX_SELL_ORDER_TX, txUidIn, validHeightIn, feesIn) {
        coinType   = coinTypeIn;
        assetType  = assetTypeIn;
        assetAmount = assetAmountIn;
        askPrice   = askPriceIn;
    }

    ~CDEXSellLimitOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE((uint8_t)coinType);
        READWRITE((uint8_t)assetType);
        READWRITE(VARINT(assetAmount));
        READWRITE(VARINT(askPrice));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << assetAmount << askPrice;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSellLimitOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

public:
    CoinType coinType;      //!< coin type (wusd) to sell asset
    CoinType assetType;     //!< holing asset type (wicc or micc) to sell in coinType
    uint64_t assetAmount;    //!< amount of holding asset to sell
    uint64_t askPrice;      //!< asking price in coinType willing to sell

};

class CDEXBuyMarketOrderTx : public CBaseTx {
public:
    CDEXBuyMarketOrderTx() : CBaseTx(DEX_BUY_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_BUY_ORDER_TX) {
        assert(DEX_BUY_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyMarketOrderTx *)pBaseTx;
    }

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         CoinType coinTypeIn, CoinType assetTypeIn, uint64_t coinAmountIn)
        : CBaseTx(DEX_BUY_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coinType(coinTypeIn),
          assetType(assetTypeIn),
          coinAmount(coinAmountIn) {}

    ~CDEXBuyMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE((uint8_t)coinType);
        READWRITE((uint8_t)assetType);
        READWRITE(VARINT(coinAmount));
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << coinAmount;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyLimitOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    
public:
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t coinAmount;   //!< amount of target coin to spend for buying asset
};


class CDEXSellMarketOrderTx : public CBaseTx {

public:
    CDEXSellMarketOrderTx() : CBaseTx(DEX_BUY_ORDER_TX) {}

    CDEXSellMarketOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_BUY_ORDER_TX) {
        assert(DEX_BUY_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellMarketOrderTx *)pBaseTx;
    }

    CDEXSellMarketOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         CoinType coinTypeIn, CoinType assetTypeIn, uint64_t assetAmountIn)
        : CBaseTx(DEX_BUY_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coinType(coinTypeIn),
          assetType(assetTypeIn),
          assetAmount(assetAmountIn) {}

    ~CDEXSellMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE((uint8_t)coinType);
        READWRITE((uint8_t)assetType);
        READWRITE(VARINT(assetAmount));
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << assetAmount;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyLimitOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountCache &view); //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    
public:
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t assetAmount;   //!< amount of target asset to buy
};

struct DEXDealItem  {
    uint256 buyOrderId;
    uint256 sellOrderId;
    uint64_t dealPrice;
    uint64_t dealAmount;

    IMPLEMENT_SERIALIZE(
        READWRITE(buyOrderId);
        READWRITE(sellOrderId);
        READWRITE(VARINT(dealPrice));
        READWRITE(VARINT(dealAmount));
    )
};

class CDEXSettleTx: public CBaseTx {

public:
    CDEXSettleTx() : CBaseTx(DEX_SETTLE_TX) {}

    CDEXSettleTx(const CBaseTx *pBaseTx): CBaseTx(DEX_SETTLE_TX) {
        assert(DEX_SETTLE_TX == pBaseTx->nTxType);
        *this = *(CDEXSettleTx *)pBaseTx;
    }

    CDEXSettleTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn):
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

    void AddDealItem(const DEXDealItem& item) {
        dealItems.push_back(item);
    }

    vector<DEXDealItem>& GetDealItems() { return dealItems; }

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