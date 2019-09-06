// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_H
#define TX_DEX_H

#include "entities/asset.h"
#include "entities/dexorder.h"
#include "tx.h"
#include "persistence/dexdb.h"

class CDEXOrderBaseTx : public CBaseTx {
public:
    using CBaseTx::CBaseTx;

public:
    bool CheckOrderAmountRange(CValidationState &state, const string &title,
                             const TokenSymbol &symbol, const int64_t amount);

    bool CheckOrderPriceRange(CValidationState &state, const string &title,
                              const TokenSymbol &coin_symbol, const TokenSymbol &asset_symbol,
                              const int64_t price);
    bool CheckOrderSymbols(CValidationState &state, const string &title,
                           const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol);
public:
    static uint64_t CalcCoinAmount(uint64_t assetAmount, const uint64_t price);
};

class CDEXBuyLimitOrderTx : public CDEXOrderBaseTx {

public:
    CDEXBuyLimitOrderTx() : CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                        uint64_t fees, const TokenSymbol &coinSymbol,
                        const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                        uint64_t bid_priceIn)
        : CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          asset_amount(assetAmountIn),
          bid_price(bid_priceIn) {}

    ~CDEXBuyLimitOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(bid_price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyLimitOrderTx>(*this); }
    string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    TokenSymbol coin_symbol;   //!< coin type (wusd) to buy asset
    TokenSymbol asset_symbol;  //!< asset type
    uint64_t asset_amount;     //!< amount of target asset to buy
    uint64_t bid_price;        //!< bidding price in coin_symbol willing to buy
};

class CDEXSellLimitOrderTx : public CDEXOrderBaseTx {

public:
    CDEXSellLimitOrderTx() : CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                         uint64_t fees, TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                         uint64_t assetAmount, uint64_t askPrice)
        : CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          asset_amount(assetAmount),
          ask_price(askPrice) {}

    ~CDEXSellLimitOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(ask_price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellLimitOrderTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    TokenSymbol coin_symbol;   //!< coin type (wusd) to sell asset
    TokenSymbol asset_symbol;  //!< holding asset type (wicc or wgrt) to sell in coin_symbol
    uint64_t asset_amount;     //!< amount of holding asset to sell
    uint64_t ask_price;        //!< asking price in coin_symbol willing to sell
};

class CDEXBuyMarketOrderTx : public CDEXOrderBaseTx {
public:
    CDEXBuyMarketOrderTx() : CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                         uint64_t fees, TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                         uint64_t coinAmountIn)
        : CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          coin_amount(coinAmountIn) {}

    ~CDEXBuyMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(coin_amount);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyMarketOrderTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    TokenSymbol coin_symbol;   //!< coin type (wusd) to buy asset
    TokenSymbol asset_symbol;  //!< asset type
    uint64_t coin_amount;      //!< amount of target coin to spend for buying asset
};

class CDEXSellMarketOrderTx : public CDEXOrderBaseTx {
public:
    CDEXSellMarketOrderTx() : CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX) {}

    CDEXSellMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                          uint64_t fees, TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                          uint64_t assetAmountIn)
        : CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees),
          coin_symbol(coinSymbol),
          asset_symbol(assetSymbol),
          asset_amount(assetAmountIn) {}

    ~CDEXSellMarketOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellMarketOrderTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    TokenSymbol coin_symbol;   //!< coin type (wusd) to buy asset
    TokenSymbol asset_symbol;  //!< asset type
    uint64_t asset_amount;     //!< amount of target asset to buy
};

class CDEXCancelOrderTx : public CBaseTx {
public:
    CDEXCancelOrderTx() : CBaseTx(DEX_CANCEL_ORDER_TX) {}

    CDEXCancelOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                      uint64_t fees, uint256 orderIdIn)
        : CBaseTx(DEX_CANCEL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees),
          orderId(orderIdIn) {}

    ~CDEXCancelOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(orderId);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << orderId;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXCancelOrderTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
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

    CDEXSettleTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                 uint64_t fees, const vector<DEXDealItem> &dealItemsIn)
        : CBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feeSymbol, fees),
          dealItems(dealItemsIn) {}

    ~CDEXSettleTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(dealItems);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    void AddDealItem(const DEXDealItem& item) {
        dealItems.push_back(item);
    }

    vector<DEXDealItem>& GetDealItems() { return dealItems; }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSettleTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);

private:
    bool GetDealOrder(CCacheWrapper &cw, CValidationState &state, const uint256 &txid,
        const OrderSide orderSide, CDEXOrderDetail &dealOrder);

private:
    vector<DEXDealItem> dealItems;
};

#endif  // TX_DEX_H