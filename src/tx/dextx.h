// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_H
#define TX_DEX_H

#include "entities/asset.h"
#include "tx.h"
#include "persistence/dexdb.h"

class CDEXOrderBaseTx : public CBaseTx {
public:
    using CBaseTx::CBaseTx;

    virtual void GetOrderDetail(CDEXOrderDetail &orderDetail) = 0;

public:
    bool CheckOrderAmountRange(CValidationState &state, const string &title,
                             const TokenSymbol &symbol, int64_t amount);

    bool CheckOrderPriceRange(CValidationState &state, const string &title,
                              const TokenSymbol &coin_symbol, const TokenSymbol &asset_symbol,
                              int64_t price);
    bool CheckOrderSymbols(CValidationState &state, const string &title,
                           const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol);
public:
    static uint64_t CalcCoinAmount(uint64_t assetAmount, uint64_t price);
};

class CDEXBuyLimitOrderTx : public CDEXOrderBaseTx {

public:
    CDEXBuyLimitOrderTx() : CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {
        assert(DEX_LIMIT_BUY_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyLimitOrderTx *)pBaseTx;
    }

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                   TokenSymbol coinSymbol, TokenSymbol assetSymbol,
                   uint64_t assetAmountIn, uint64_t bid_priceIn)
        : CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          asset_amount(assetAmountIn),
          bid_price(bid_priceIn) {}

    ~CDEXBuyLimitOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(bid_price));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << (uint8_t)nTxType << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << coin_symbol << asset_symbol << asset_amount << bid_price;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyLimitOrderTx>(this); }
    string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);
public: // derive from CDEXOrderBaseTx
    virtual void GetOrderDetail(CDEXOrderDetail &orderDetail);

private:
    TokenSymbol coin_symbol;          //!< coin type (wusd) to buy asset
    TokenSymbol asset_symbol;        //!< asset type
    uint64_t asset_amount;       //!< amount of target asset to buy
    uint64_t bid_price;          //!< bidding price in coin_symbol willing to buy

};

class CDEXSellLimitOrderTx : public CDEXOrderBaseTx {

public:
    CDEXSellLimitOrderTx() : CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {
        assert(DEX_LIMIT_SELL_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellLimitOrderTx *)pBaseTx;
    }

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                    TokenSymbol coinSymbol, TokenSymbol assetSymbol,
                    uint64_t assetAmount, uint64_t askPrice)
        : CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX, txUidIn, validHeightIn, feesIn) {
        coin_symbol   = coinSymbol;
        asset_symbol  = assetSymbol;
        asset_amount = assetAmount;
        ask_price   = askPrice;
    }

    ~CDEXSellLimitOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(ask_price));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << (uint8_t)nTxType << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << coin_symbol << asset_symbol << asset_amount << ask_price;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSellLimitOrderTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);
public: // derive from CDEXOrderBaseTx
    virtual void GetOrderDetail(CDEXOrderDetail &orderDetail);

private:
    TokenSymbol coin_symbol;       //!< coin type (wusd) to sell asset
    TokenSymbol asset_symbol;     //!< holding asset type (wicc or wgrt) to sell in coin_symbol
    uint64_t asset_amount;    //!< amount of holding asset to sell
    uint64_t ask_price;       //!< asking price in coin_symbol willing to sell

};

class CDEXBuyMarketOrderTx : public CDEXOrderBaseTx {
public:
    CDEXBuyMarketOrderTx() : CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {
        assert(DEX_MARKET_BUY_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXBuyMarketOrderTx *)pBaseTx;
    }

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         TokenSymbol coinSymbol, TokenSymbol assetSymbol, uint64_t coinAmountIn)
        : CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          coin_amount(coinAmountIn) {}

    ~CDEXBuyMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << (uint8_t)nTxType << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << coin_symbol << asset_symbol << coin_amount;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXBuyMarketOrderTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);

public: // derive from CDEXOrderBaseTx
    virtual void GetOrderDetail(CDEXOrderDetail &orderDetail);
private:
    TokenSymbol coin_symbol;      //!< coin type (wusd) to buy asset
    TokenSymbol asset_symbol;    //!< asset type
    uint64_t coin_amount;    //!< amount of target coin to spend for buying asset
};

class CDEXSellMarketOrderTx : public CDEXOrderBaseTx {
public:
    CDEXSellMarketOrderTx() : CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX) {}

    CDEXSellMarketOrderTx(const CBaseTx *pBaseTx): CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX) {
        assert(DEX_MARKET_SELL_ORDER_TX == pBaseTx->nTxType);
        *this = *(CDEXSellMarketOrderTx *)pBaseTx;
    }

    CDEXSellMarketOrderTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
                         TokenSymbol coinSymbol, TokenSymbol assetSymbol, uint64_t assetAmountIn)
        : CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX, txUidIn, validHeightIn, feesIn),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          asset_amount(assetAmountIn) {}

    ~CDEXSellMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << (uint8_t)nTxType << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << coin_symbol << asset_symbol << asset_amount;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSellMarketOrderTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);

public: // derive from CDEXOrderBaseTx
    virtual void GetOrderDetail(CDEXOrderDetail &orderDetail);
private:
    TokenSymbol coin_symbol;      //!< coin type (wusd) to buy asset
    TokenSymbol asset_symbol;    //!< asset type
    uint64_t asset_amount;   //!< amount of target asset to buy
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

        READWRITE(VARINT(llFees));
        READWRITE(orderId);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(nValidHeight) << txUid << VARINT(llFees) << orderId;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXCancelOrderTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);
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
    CDEXSettleTx() : CBaseTx(DEX_TRADE_SETTLE_TX) {}

    CDEXSettleTx(const CBaseTx *pBaseTx): CBaseTx(DEX_TRADE_SETTLE_TX) {
        assert(DEX_TRADE_SETTLE_TX == pBaseTx->nTxType);
        *this = *(CDEXSettleTx *)pBaseTx;
    }

    CDEXSettleTx(const CUserID &txUidIn, int validHeightIn, uint64_t feesIn,
        const vector<DEXDealItem> &dealItemsIn):
        CBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feesIn), dealItems(dealItemsIn) {
    }

    ~CDEXSettleTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(dealItems);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(nValidHeight) << txUid << VARINT(llFees) << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    void AddDealItem(const DEXDealItem& item) {
        dealItems.push_back(item);
    }

    vector<DEXDealItem>& GetDealItems() { return dealItems; }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDEXSettleTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);
private:

private:
    vector<DEXDealItem> dealItems;
};

#endif  // TX_DEX_H