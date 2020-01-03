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
#include <optional>

class CDEXOrderBaseTx : public CBaseTx {
public:
    OrderType order_type        = ORDER_LIMIT_PRICE; //!< order type
    OrderSide order_side        = ORDER_BUY;         //!< order side
    TokenSymbol coin_symbol     = "";                //!< coin symbol
    TokenSymbol asset_symbol    = "";                //!< asset symbol
    uint64_t coin_amount        = 0;                 //!< amount of coin to buy/sell asset
    uint64_t asset_amount       = 0;                 //!< amount of asset to buy/sell
    uint64_t price              = 0;                 //!< price in coinType want to buy/sell asset
    DexID dex_id                = 0;                 //!< dex id
    OrderOpt order_opt          = OrderOpt::IS_PUBLIC;  //!< order opt: is_public, has_fee_ratio
    uint64_t match_fee_ratio    = 0;                    //!< match fee ratio, effective when order_opt.HasFeeRatio()==true, otherwith must be 0
    CUserID operator_uid        = CUserID();            //!< dex operator uid
    string memo                 = "";                   //!< memo

    UnsignedCharArray operator_signature;               //!< dex operator signature

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

    bool CheckOrderFeeRate(CTxExecuteContext &context, const string &title);
    bool CheckOrderOperator(CTxExecuteContext &context, const string &title);

    bool ProcessOrder(CTxExecuteContext &context, CAccount &txAccount, const string &title);
    bool FreezeBalance(CTxExecuteContext &context, CAccount &account,
                       const TokenSymbol &tokenSymbol, const uint64_t &amount, const string &title);

public:
    static uint64_t CalcCoinAmount(uint64_t assetAmount, const uint64_t price);

};

////////////////////////////////////////////////////////////////////////////////
// buy limit order tx
class CDEXBuyLimitOrderBaseTx : public CDEXOrderBaseTx {
public:
    CDEXBuyLimitOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {
        order_type      = ORDER_LIMIT_PRICE;
        order_side      = ORDER_BUY;
    }

    CDEXBuyLimitOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees,
                            const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol,
                            uint64_t assetAmountIn, uint64_t priceIn, DexID dexIdIn,
                            OrderOpt orderOptIn, uint64_t orderFeeRatioIn,
                            const CUserID &operatorUidIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        order_type      = ORDER_LIMIT_PRICE;
        order_side      = ORDER_BUY;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = 0; // default 0 in buy limit order
        asset_amount    = assetAmountIn;
        price           = priceIn;
        dex_id          = dexIdIn;
        order_opt       = orderOptIn;
        match_fee_ratio = orderFeeRatioIn;
        operator_uid    = operatorUidIn;
        memo            = memoIn;
    }

    string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXBuyLimitOrderTx : public CDEXBuyLimitOrderBaseTx {

public:
    CDEXBuyLimitOrderTx() : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {}

    CDEXBuyLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                        uint64_t fees, const TokenSymbol &coinSymbol,
                        const TokenSymbol &assetSymbol, uint64_t assetAmountIn, uint64_t priceIn)
        : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                  coinSymbol, assetSymbol, assetAmountIn, priceIn, DEX_RESERVED_ID,
                                  OrderOpt::IS_PUBLIC, 0, CUserID(), "") {}

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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount) << VARINT(price);
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
                          const TokenSymbol &feeSymbol, uint64_t fees,
                          const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol,
                          uint64_t assetAmountIn, uint64_t priceIn, DexID dexIdIn,
                          OrderOpt orderOptIn, uint64_t orderFeeRatioIn,
                          const CUserID &operatorUidIn, const string &memoIn)
        : CDEXBuyLimitOrderBaseTx(DEX_LIMIT_BUY_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                  fees, coinSymbol, assetSymbol, assetAmountIn, priceIn, dexIdIn,
                                  orderOptIn, orderFeeRatioIn, operatorUidIn, memoIn) {}

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
        READWRITE(VARINT(dex_id));
        READWRITE(order_opt);
        READWRITE(VARINT(match_fee_ratio));
        READWRITE(operator_uid);
        READWRITE(memo);
        READWRITE(operator_signature);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount) << VARINT(price) << VARINT(dex_id) << order_opt
               << VARINT(match_fee_ratio) << operator_uid << memo;
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
    CDEXSellLimitOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {
        order_type      = ORDER_LIMIT_PRICE;
        order_side      = ORDER_SELL;
    }

    CDEXSellLimitOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                             const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                             const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                             uint64_t priceIn, DexID dexIdIn, OrderOpt orderOptIn,
                             uint64_t orderFeeRatioIn, const CUserID &operatorUidIn,
                             const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        order_type      = ORDER_LIMIT_PRICE;
        order_side      = ORDER_SELL;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = 0; // default 0 in sell limit order
        asset_amount    = assetAmountIn;
        price           = priceIn;
        dex_id          = dexIdIn;
        order_opt       = orderOptIn;
        match_fee_ratio = orderFeeRatioIn;
        operator_uid    = operatorUidIn;
        memo            = memoIn;
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXSellLimitOrderTx : public CDEXSellLimitOrderBaseTx {

public:
    CDEXSellLimitOrderTx() : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {}

    CDEXSellLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                         const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                         const TokenSymbol &assetSymbol, uint64_t assetAmount, uint64_t priceIn)
        : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   coinSymbol, assetSymbol, assetAmount, priceIn, DEX_RESERVED_ID,
                                   OrderOpt::IS_PUBLIC, 0, CUserID(), "") {}

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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount) << VARINT(price);
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
                           const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                           const TokenSymbol &assetSymbol, uint64_t assetAmount, uint64_t priceIn,
                           DexID dexIdIn, OrderOpt orderOptIn, uint64_t orderFeeRatioIn,
                           const CUserID &operatorUidIn, const string &memoIn)
        : CDEXSellLimitOrderBaseTx(DEX_LIMIT_SELL_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                   fees, coinSymbol, assetSymbol, assetAmount, priceIn, dexIdIn,
                                   orderOptIn, orderFeeRatioIn, operatorUidIn, memoIn) {}

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
        READWRITE(VARINT(dex_id));
        READWRITE(order_opt);
        READWRITE(VARINT(match_fee_ratio));
        READWRITE(operator_uid);
        READWRITE(memo);
        READWRITE(operator_signature);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount) << VARINT(price) << VARINT(dex_id) << order_opt
               << VARINT(match_fee_ratio) << operator_uid << memo;
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
    CDEXBuyMarketOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {
        order_type      = ORDER_MARKET_PRICE;
        order_side      = ORDER_BUY;
    }

    CDEXBuyMarketOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                             const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                             const TokenSymbol &assetSymbol, uint64_t coinAmountIn,
                             DexID dexIdIn, OrderOpt orderOptIn, uint64_t orderFeeRatioIn,
                             const CUserID &operatorUidIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        order_type      = ORDER_MARKET_PRICE;
        order_side      = ORDER_BUY;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = coinAmountIn;
        asset_amount    = 0; // default 0 in buy market order
        price           = 0; // default 0 in buy market order
        dex_id          = dexIdIn;
        order_opt       = orderOptIn;
        match_fee_ratio = orderFeeRatioIn;
        operator_uid    = operatorUidIn;
        memo            = memoIn;
    }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CDEXBuyMarketOrderTx : public CDEXBuyMarketOrderBaseTx {
public:
    CDEXBuyMarketOrderTx() : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {}

    CDEXBuyMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                         const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                         const TokenSymbol &assetSymbol, uint64_t coinAmountIn)
        : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees,
                                   coinSymbol, assetSymbol, coinAmountIn, DEX_RESERVED_ID,
                                   OrderOpt::IS_PUBLIC, 0, CUserID(), "") {}

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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(coin_amount);
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
                           const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                           const TokenSymbol &assetSymbol, uint64_t coinAmountIn, DexID dexIdIn,
                           OrderOpt orderOptIn, uint64_t orderFeeRatioIn, const CUserID &operatorUidIn,
                           const string &memoIn)
        : CDEXBuyMarketOrderBaseTx(DEX_MARKET_BUY_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                   fees, coinSymbol, assetSymbol, coinAmountIn, dexIdIn, orderOptIn,
                                   orderFeeRatioIn, operatorUidIn, memo) {}

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
        READWRITE(VARINT(dex_id));
        READWRITE(order_opt);
        READWRITE(VARINT(match_fee_ratio));
        READWRITE(operator_uid);
        READWRITE(memo);
        READWRITE(operator_signature);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol << VARINT(coin_amount)
               << VARINT(dex_id) << order_opt << VARINT(match_fee_ratio) << operator_uid << memo;
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
    CDEXSellMarketOrderBaseTx(TxType nTxTypeIn) : CDEXOrderBaseTx(nTxTypeIn) {
        order_type      = ORDER_MARKET_PRICE;
        order_side      = ORDER_SELL;
    }

    CDEXSellMarketOrderBaseTx(TxType nTxTypeIn, const CUserID &txUidIn, int32_t validHeightIn,
                              const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                              const TokenSymbol &assetSymbol, uint64_t assetAmountIn,
                              DexID dexIdIn, OrderOpt orderOptIn, uint64_t orderFeeRatioIn,
                              const CUserID &operatorUidIn, const string &memoIn)
        : CDEXOrderBaseTx(nTxTypeIn, txUidIn, validHeightIn, feeSymbol, fees) {
        order_type      = ORDER_MARKET_PRICE;
        order_side      = ORDER_SELL;
        coin_symbol     = coinSymbol;
        asset_symbol    = assetSymbol;
        coin_amount     = 0; // default 0 in sell market order
        asset_amount    = assetAmountIn;
        price           = 0; // default 0 in sell market order
        dex_id          = dexIdIn;
        order_opt       = orderOptIn;
        match_fee_ratio = orderFeeRatioIn;
        operator_uid    = operatorUidIn;
        memo            = memoIn;
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
                                    fees, coinSymbol, assetSymbol, assetAmountIn, DEX_RESERVED_ID,
                                    OrderOpt::IS_PUBLIC, 0, CUserID(), "") {}

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
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount);
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
                            const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbol,
                            const TokenSymbol &assetSymbol, uint64_t assetAmountIn, DexID dexIdIn,
                            OrderOpt orderOptIn, uint64_t orderFeeRatioIn, const CUserID &operatorUidIn,
                            const string &memoIn)
        : CDEXSellMarketOrderBaseTx(DEX_MARKET_SELL_ORDER_EX_TX, txUidIn, validHeightIn, feeSymbol,
                                    fees, coinSymbol, assetSymbol, assetAmountIn, dexIdIn,
                                    orderOptIn, orderFeeRatioIn, operatorUidIn, memoIn) {}

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
        READWRITE(VARINT(dex_id));
        READWRITE(order_opt);
        READWRITE(VARINT(match_fee_ratio));
        READWRITE(operator_uid);
        READWRITE(memo);
        READWRITE(operator_signature);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
               << VARINT(asset_amount) << VARINT(dex_id) << order_opt << VARINT(match_fee_ratio)
               << operator_uid << memo;
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
          order_id(orderIdIn) {}

    ~CDEXCancelOrderTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(order_id);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
               << fee_symbol << VARINT(llFees) << order_id;
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
    uint256  order_id;       //!< id of oder need to be canceled.
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

class CDEXSettleTx: public CBaseTx {

public:
    CDEXSettleTx() : CBaseTx(DEX_TRADE_SETTLE_TX) {}

    CDEXSettleTx(const CUserID &txUidIn, int32_t validHeightIn,
                 const TokenSymbol &feeSymbol, uint64_t fees,
                 const vector<DEXDealItem> &dealItemsIn)
        : CBaseTx(DEX_TRADE_SETTLE_TX, txUidIn, validHeightIn, feeSymbol, fees), dealItems(dealItemsIn) {}

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

    void AddDealItem(const DEXDealItem& item) {
        dealItems.push_back(item);
    }

    vector<DEXDealItem>& GetDealItems() { return dealItems; }

    virtual string ToString(CAccountDBCache &accountCache); //logging usage
    virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

protected:
    bool GetDealOrder(CCacheWrapper &cw, CValidationState &state, uint32_t index, const uint256 &order_id,
        const OrderSide orderSide, CDEXOrderDetail &dealOrder);
    bool CheckDexOperator(CTxExecuteContext &context, uint32_t i, const CDEXOrderDetail &buyOrder,
                    const CDEXOrderDetail &sellOrder, const OrderSide &takerSide);

    OrderSide GetTakerOrderSide(const CDEXOrderDetail &buyOrder, const CDEXOrderDetail &sellOrder);
    uint64_t GetOperatorFeeRatio(const CDEXOrderDetail &order,
                                 const DexOperatorDetail &operatorDetail,
                                 const OrderSide &makerSide);
    bool CalcOrderFee(CTxExecuteContext &context, uint32_t i, uint64_t amount, uint64_t fee_ratio,
                      uint64_t &orderFee);

protected:
    vector<DEXDealItem> dealItems;
};

#endif  // TX_DEX_H