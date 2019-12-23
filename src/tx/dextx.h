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
    DexID dex_id                       = 0;                 //!< dex id
    OrderType order_type               = ORDER_LIMIT_PRICE; //!< order type
    OrderSide order_side               = ORDER_BUY;         //!< order side
    TokenSymbol coin_symbol            = "";                //!< coin symbol
    TokenSymbol asset_symbol           = "";                //!< asset symbol
    uint64_t coin_amount               = 0;                 //!< amount of coin to buy/sell asset
    uint64_t asset_amount              = 0;                 //!< amount of asset to buy/sell
    uint64_t price                     = 0;                 //!< price in coinType want to buy/sell asset
    string memo                        = "";                //!< memo

    using CBaseTx::CBaseTx;
public:
    bool CheckOrderAmountRange(CValidationState &state, const string &title,
                             const TokenSymbol &symbol, const int64_t amount);

    bool CheckOrderPriceRange(CValidationState &state, const string &title,
                              const TokenSymbol &coin_symbol, const TokenSymbol &asset_symbol,
                              const int64_t price);
    bool CheckOrderSymbols(CValidationState &state, const string &title,
                           const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol);

    bool CheckDexOperatorExist(CTxExecuteContext &context);

    static uint64_t CalcCoinAmount(uint64_t assetAmount, const uint64_t price);


};

////////////////////////////////////////////////////////////////////////////////
// buy limit order tx
class CDEXBuyLimitOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXBuyLimitOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXBuyLimitOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                            const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol,
                            uint64_t assetAmountIn, uint64_t priceIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        dex_id       = dexIdIn;
        order_type   = ORDER_LIMIT_PRICE;
        order_side   = ORDER_BUY;
        coin_symbol  = coinSymbol;
        asset_symbol = assetSymbol;
        coin_amount  = 0; // default 0 in buy limit order
        asset_amount = assetAmountIn;
        price        = priceIn;
        memo         = memoIn;
    }

    string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
// protected:
//     DexID dex_id = DEX_RESERVED_ID;
//     TokenSymbol coin_symbol;   //!< coin type (wusd) to buy asset
//     TokenSymbol asset_symbol;  //!< asset type
//     uint64_t asset_amount = 0; //!< amount of target asset to buy
//     uint64_t price        = 0; //!< bidding price in coin_symbol willing to buy
//     string memo;
};

class CDEXBuyLimitOrderTx : public CDEXBuyLimitOrderBaseTx {

public:
    CDEXBuyLimitOrderTx() : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                        uint64_t fees, const TokenSymbol &coinSymbol,
                        const TokenSymbol &assetSymbol, uint64_t assetAmountIn, uint64_t priceIn)
        : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                  DEX_RESERVED_ID, coinSymbol, assetSymbol, assetAmountIn, priceIn,
                                  "") {}

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
        READWRITE(VARINT(price));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyLimitOrderTx>(*this); }

    virtual bool CheckTx(CTxExecuteContext &context);
};


class CDEXBuyLimitOrderExTx : public CDEXBuyLimitOrderBaseTx {
public:
    CDEXBuyLimitOrderExTx() : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_EX_TX) {}

    CDEXBuyLimitOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                          const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                          const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol,
                          uint64_t assetAmountIn, uint64_t priceIn, const string &memoIn)
        : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                  dexIdIn, coinSymbol, assetSymbol, assetAmountIn, priceIn,
                                  memoIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(VARINT(dex_id));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
        READWRITE(memo);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << VARINT(dex_id) << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(price) << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyLimitOrderExTx>(*this); }

    virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// sell limit order tx
class CDEXSellLimitOrderBaseTx : public CDEXOrderBaseTx {

public:
    CDEXSellLimitOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXSellLimitOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                         uint64_t fees, DexID dexIdIn, TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                         uint64_t assetAmountIn, uint64_t priceIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees){
        dex_id       = dexIdIn;
        order_type   = ORDER_LIMIT_PRICE;
        order_side   = ORDER_SELL;
        coin_symbol  = coinSymbol;
        asset_symbol = assetSymbol;
        coin_amount  = 0; // default 0 in sell limit order
        asset_amount = assetAmountIn;
        price        = priceIn;
        memo         = memoIn;
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
// protected:
//     DexID dex_id = DEX_RESERVED_ID;
//     TokenSymbol coin_symbol;   //!< coin type (wusd) to sell asset
//     TokenSymbol asset_symbol;  //!< holding asset type (wicc or wgrt) to sell in coin_symbol
//     uint64_t asset_amount;     //!< amount of holding asset to sell
//     uint64_t price;        //!< asking price in coin_symbol willing to sell
//     string memo;
};

class CDEXSellLimitOrderTx : public CDEXSellLimitOrderBaseTx {

public:
    CDEXSellLimitOrderTx() : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                         const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                         const TokenSymbol &assetSymbol, uint64_t assetAmount, uint64_t priceIn)
        : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   DEX_RESERVED_ID, coinSymbol, assetSymbol, assetAmount, priceIn,
                                   "") {}

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
        READWRITE(VARINT(price));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(price);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellLimitOrderTx>(*this); }
    // TODO: need check tx?
    //virtual bool CheckTx(CTxExecuteContext &context);
};

class CDEXSellLimitOrderExTx : public CDEXSellLimitOrderBaseTx {

public:
    CDEXSellLimitOrderExTx() : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_EX_TX) {}

    CDEXSellLimitOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                           const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                           TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                           uint64_t assetAmount, uint64_t priceIn, const string &memoIn)
        : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   dexIdIn, coinSymbol, assetSymbol, assetAmount, priceIn, memoIn) {
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(VARINT(dex_id));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(VARINT(price));
        READWRITE(memo);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << VARINT(dex_id) << coin_symbol << asset_symbol << VARINT(asset_amount) << VARINT(price) << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellLimitOrderExTx>(*this); }
    // TODO: need check tx?
    //virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// buy market order tx
class CDEXBuyMarketOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXBuyMarketOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXBuyMarketOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                         uint64_t fees, DexID dexIdIn, TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                         uint64_t coinAmountIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees){
        dex_id       = dexIdIn;
        order_type   = ORDER_MARKET_PRICE;
        order_side   = ORDER_BUY;
        coin_symbol  = coinSymbol;
        asset_symbol = assetSymbol;
        coin_amount  = coinAmountIn;
        asset_amount = 0; // default 0 in buy market order
        price        = 0; // default 0 in buy market order
        memo         = memoIn;
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
// private:
//     TokenSymbol coin_symbol;   //!< coin type (wusd) to buy asset
//     TokenSymbol asset_symbol;  //!< asset type
//     uint64_t coin_amount;      //!< amount of target coin to spend for buying asset
};

class CDEXBuyMarketOrderTx : public CDEXBuyMarketOrderBaseTx {
public:
    CDEXBuyMarketOrderTx() : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                         const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                         const TokenSymbol &assetSymbol, uint64_t coinAmountIn)
        : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   DEX_RESERVED_ID, coinSymbol, assetSymbol, coinAmountIn, "") {}

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

    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};


class CDEXBuyMarketOrderExTx : public CDEXBuyMarketOrderBaseTx {
public:
    CDEXBuyMarketOrderExTx() : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_EX_TX) {}

    CDEXBuyMarketOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                           const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                           TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                           uint64_t coinAmountIn, const string &memoIn)
        : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   dexIdIn, coinSymbol, assetSymbol, coinAmountIn, memo) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(VARINT(dex_id));
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(memo);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << VARINT(dex_id) << coin_symbol << asset_symbol << VARINT(coin_amount) << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyMarketOrderExTx>(*this); }

    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// sell market order tx
class CDEXSellMarketOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXSellMarketOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {}

    CDEXSellMarketOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                          uint64_t fees, DexID dexIdIn, TokenSymbol coinSymbol, const TokenSymbol &assetSymbol,
                          uint64_t assetAmountIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees){
        dex_id       = dexIdIn;
        order_type   = ORDER_MARKET_PRICE;
        order_side   = ORDER_SELL;
        coin_symbol  = coinSymbol;
        asset_symbol = assetSymbol;
        coin_amount  = 0; // default 0 in sell market order
        asset_amount = assetAmountIn;
        price        = 0; // default 0 in sell market order
        memo         = memoIn;
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXSellMarketOrderTx : public CDEXSellMarketOrderBaseTx {
public:
    CDEXSellMarketOrderTx() : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_TX) {}

    CDEXSellMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                          const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                          const TokenSymbol &assetSymbol, uint64_t assetAmountIn)
        : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol,
                                    fees, DEX_RESERVED_ID, coinSymbol, assetSymbol, assetAmountIn,
                                    "") {}

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

    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};

class CDEXSellMarketOrderExTx : public CDEXSellMarketOrderBaseTx {
public:
    CDEXSellMarketOrderExTx() : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_EX_TX) {}

    CDEXSellMarketOrderExTx(const CUserID &txUidIn, int32_t validHeightIn,
                          const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn, TokenSymbol coinSymbol,
                          const TokenSymbol &assetSymbol, uint64_t assetAmountIn, const string &memoIn)
        : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                    fees, dexIdIn, coinSymbol, assetSymbol, assetAmountIn,
                                    memoIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(VARINT(dex_id));
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(asset_symbol);
        READWRITE(VARINT(asset_amount));
        READWRITE(memo);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << VARINT(dex_id)<< coin_symbol << asset_symbol << VARINT(asset_amount) << memo;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellMarketOrderExTx>(*this); }
    // TODO: need check tx??
    //virtual bool CheckTx(CTxExecuteContext &context);
};

////////////////////////////////////////////////////////////////////////////////
// cancel order tx
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

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
public:
    uint256  orderId;       //!< id of oder need to be canceled.
};

////////////////////////////////////////////////////////////////////////////////
// settle order tx
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

    string ToString() const;
};

class CDEXSettleBaseTx: public CBaseTx {

public:
    CDEXSettleBaseTx(TxType nTxTypeIn) : CBaseTx(nTxTypeIn) {}

    CDEXSettleBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                     const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                     const vector<DEXDealItem> &dealItemsIn, const string &memoIn)
        : CBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees), dex_id(dexIdIn),
          dealItems(dealItemsIn), memo(memoIn) {}

    void AddDealItem(const DEXDealItem& item) {
        dealItems.push_back(item);
    }

    vector<DEXDealItem>& GetDealItems() { return dealItems; }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

protected:
    bool GetDealOrder(CCacheWrapper &cw, CValidationState &state, uint32_t index, const uint256 &orderId,
        const OrderSide orderSide, CDEXOrderDetail &dealOrder);
    bool CheckDexId(CTxExecuteContext &context, uint32_t i, uint32_t buyDexId, uint32_t sellDexId);

protected:
    DexID   dex_id;
    vector<DEXDealItem> dealItems;
    string memo;
};

class CDEXSettleTx: public CDEXSettleBaseTx {

public:
    CDEXSettleTx() : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX) {}

    CDEXSettleTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                 uint64_t fees, const vector<DEXDealItem> &dealItemsIn)
        : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feeSymbol, fees,
                           DEX_RESERVED_ID, dealItemsIn, "") {}

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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSettleTx>(*this); }

    // TODO: check tx
    //virtual bool CheckTx(CTxExecuteContext &context);
};

class CDEXSettleExTx: public CDEXSettleBaseTx {

public:
    CDEXSettleExTx() : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX) {}

    CDEXSettleExTx(const CUserID &txUidIn, int32_t validHeightIn,
                     const TokenSymbol &feeSymbol, uint64_t fees, DexID dexIdIn,
                     const vector<DEXDealItem> &dealItemsIn, const string &memoIn)
        : CDEXSettleBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feeSymbol, fees, dexIdIn,
                           dealItemsIn, memoIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(VARINT(dex_id));
        READWRITE(dealItems);
        READWRITE(memo);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << dealItems;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSettleExTx>(*this); }

    // TODO: check tx
    //virtual bool CheckTx(CTxExecuteContext &context);
};

#endif  // TX_DEX_H