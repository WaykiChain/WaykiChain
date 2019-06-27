// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_H
#define TX_DEX_H

#include "tx.h"
#include "persistence/dexdb.h"

class CDEXOrderBaseTx : public CBaseTx {
public:
    using CBaseTx::CBaseTx;

    virtual void GetOrderData(CDEXOrderData &orderData) = 0;
public:
    static bool CalcCoinAmount(uint64_t assetAmount, uint64_t price, uint64_t &coinAmountOut);
};

class CDEXBuyLimitOrderTx : public CDEXOrderBaseTx {

public:
    CDEXBuyLimitOrderTx() : CDEXOrderBaseTx(DEX_BUY_LIMIT_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_BUY_LIMIT_ORDER_TX) {
        assert(DEX_BUY_LIMIT_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyLimitOrderTx *)pBaseTx;
    }

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                   CoinType coinTypeIn, CoinType assetTypeIn,
                   uint64_t assetAmountIn, uint64_t bidPriceIn)
        : CDEXOrderBaseTx(DEX_BUY_LIMIT_ORDER_TX, txUidIn, validHeightIn, feesIn),
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

        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);
        READWRITE(VARINT(assetAmount));
        READWRITE(VARINT(bidPrice));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << assetAmount << bidPrice;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyLimitOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountDBCache &view); //logging usage
    virtual Object ToJson(const CAccountDBCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
public: // devive from CDEXOrderBaseTx
    virtual void GetOrderData(CDEXOrderData &orderData);
private:
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t assetAmount;     //!< amount of target asset to buy
    uint64_t bidPrice;      //!< bidding price in coinType willing to buy

};

class CDEXSellLimitOrderTx : public CDEXOrderBaseTx {

public:
    CDEXSellLimitOrderTx() : CDEXOrderBaseTx(DEX_SELL_LIMIT_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_SELL_LIMIT_ORDER_TX) {
        assert(DEX_SELL_LIMIT_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellLimitOrderTx *)pBaseTx;
    }

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                    CoinType coinTypeIn, CoinType assetTypeIn,
                    uint64_t assetAmountIn, uint64_t askPriceIn)
        : CDEXOrderBaseTx(DEX_SELL_LIMIT_ORDER_TX, txUidIn, validHeightIn, feesIn) {
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

        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);
        READWRITE(VARINT(assetAmount));
        READWRITE(VARINT(askPrice));)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << assetAmount << askPrice;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSellLimitOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountDBCache &view); //logging usage
    virtual Object ToJson(const CAccountDBCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
public: // devive from CDEXOrderBaseTx
    virtual void GetOrderData(CDEXOrderData &orderData);
private:
    CoinType coinType;      //!< coin type (wusd) to sell asset
    CoinType assetType;     //!< holing asset type (wicc or micc) to sell in coinType
    uint64_t assetAmount;    //!< amount of holding asset to sell
    uint64_t askPrice;      //!< asking price in coinType willing to sell

};

class CDEXBuyMarketOrderTx : public CDEXOrderBaseTx {
public:
    CDEXBuyMarketOrderTx() : CDEXOrderBaseTx(DEX_BUY_MARKET_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_BUY_MARKET_ORDER_TX) {
        assert(DEX_BUY_MARKET_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyMarketOrderTx *)pBaseTx;
    }

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         CoinType coinTypeIn, CoinType assetTypeIn, uint64_t coinAmountIn)
        : CDEXOrderBaseTx(DEX_BUY_MARKET_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coinType(coinTypeIn),
          assetType(assetTypeIn),
          coinAmount(coinAmountIn) {}

    ~CDEXBuyMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);
        READWRITE(VARINT(coinAmount));
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << coinAmount;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyMarketOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountDBCache &view); //logging usage
    virtual Object ToJson(const CAccountDBCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

public: // devive from CDEXOrderBaseTx
    virtual void GetOrderData(CDEXOrderData &orderData);
private:
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t coinAmount;   //!< amount of target coin to spend for buying asset
};

class CDEXSellMarketOrderTx : public CDEXOrderBaseTx {
public:
    CDEXSellMarketOrderTx() : CDEXOrderBaseTx(DEX_SELL_MARKET_ORDER_TX) {}

    CDEXSellMarketOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_SELL_MARKET_ORDER_TX) {
        assert(DEX_SELL_MARKET_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellMarketOrderTx *)pBaseTx;
    }

    CDEXSellMarketOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         CoinType coinTypeIn, CoinType assetTypeIn, uint64_t assetAmountIn)
        : CDEXOrderBaseTx(DEX_SELL_MARKET_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coinType(coinTypeIn),
          assetType(assetTypeIn),
          assetAmount(assetAmountIn) {}

    ~CDEXSellMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE((uint8_t&)coinType);
        READWRITE((uint8_t&)assetType);
        READWRITE(VARINT(assetAmount));
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << (uint8_t)coinType << (uint8_t)assetType << assetAmount;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSellMarketOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountDBCache &view); //logging usage
    virtual Object ToJson(const CAccountDBCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

public: // devive from CDEXOrderBaseTx
    virtual void GetOrderData(CDEXOrderData &orderData);
private:
    CoinType coinType;      //!< coin type (wusd) to buy asset
    CoinType assetType;     //!< asset type
    uint64_t assetAmount;   //!< amount of target asset to buy
};

class CDEXCancelOrderTx : public CBaseTx {
public:
    CDEXCancelOrderTx() : CBaseTx(DEX_CANCEL_ORDER_TX) {}

    CDEXCancelOrderTx(const CBaseTx *pBaseTx): CBaseTx(DEX_CANCEL_ORDER_TX) {
        assert(DEX_CANCEL_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXCancelOrderTx *)pBaseTx;
    }

    CDEXCancelOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         uint256 orderIdIn)
        : CBaseTx(DEX_CANCEL_ORDER_TX, txUidIn, validHeightIn, feesIn),
          orderId(orderIdIn){}

    ~CDEXCancelOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(orderId);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << orderId;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXCancelOrderTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority
    virtual string ToString(CAccountDBCache &view); //logging usage
    virtual Object ToJson(const CAccountDBCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
public:
    uint256  orderId;       //!< id of oder need to be canceled.
};

struct DEXDealItem  {
    uint256 buyOrderId;
    uint256 sellOrderId;
    uint64_t dealPrice;
    uint64_t dealCoinAmount;
    uint64_t dealAssetAmount;

    IMPLEMENT_SERIALIZE(
        READWRITE(buyOrderId);
        READWRITE(sellOrderId);
        READWRITE(VARINT(dealPrice));
        READWRITE(VARINT(dealCoinAmount));
        READWRITE(VARINT(dealAssetAmount));
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

        READWRITE(dealItems);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
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
    virtual string ToString(CAccountDBCache &view); //logging usage
    virtual Object ToJson(const CAccountDBCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:
    vector<DEXDealItem> dealItems;

};
#endif //TX_DEX_H