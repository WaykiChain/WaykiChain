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

namespace dex {

    static const EnumUnorderedSet<TxType> DEX_ORDER_TX_SET = {
        DEX_LIMIT_BUY_ORDER_TX,
        DEX_LIMIT_SELL_ORDER_TX,
        DEX_MARKET_BUY_ORDER_TX,
        DEX_MARKET_SELL_ORDER_TX,
        DEX_ORDER_TX,
        DEX_OPERATOR_ORDER_TX,
        DEX_CANCEL_ORDER_TX
    };

    class CDEXOrderBaseTx : public CBaseTx {
    public:
        OrderType order_type        = ORDER_LIMIT_PRICE;//!< order type
        OrderSide order_side        = ORDER_BUY;        //!< order side
        TokenSymbol coin_symbol     = "";               //!< coin symbol
        TokenSymbol asset_symbol    = "";               //!< asset symbol
        uint64_t coin_amount        = 0;                //!< amount of coin to buy/sell asset
        uint64_t asset_amount       = 0;                //!< amount of asset to buy/sell
        uint64_t price              = 0;                //!< price in coinType want to buy/sell asset
        DexID dex_id                = 0;                //!< dex id
        string memo                 = "";               //!< memo

        bool has_operator_config      = false;          //! has operator config
        PublicMode public_mode = PublicMode::PUBLIC;    //!< order public mode
        uint64_t maker_fee_ratio = 0;                   //!< match fee ratio
        uint64_t taker_fee_ratio = 0;                   //!< taker fee ratio
        CUserID operator_uid        = CUserID();        //!< dex operator uid
        uint64_t operator_tx_fee    = 0;                //!< tx fee paid by operator, the total_fee=llFee+operator_tx_fee
        UnsignedCharArray operator_signature;           //!< dex operator signature

        using CBaseTx::CBaseTx;
    public:
        virtual bool CheckTx(CTxExecuteContext &context);

        virtual bool ExecuteTx(CTxExecuteContext &context);

        virtual string ToString(CAccountDBCache &accountCache); //logging usage
        virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage
    protected:
        bool CheckOrderSymbols(CTxExecuteContext &context, const TokenSymbol &coinSymbol,
                            const TokenSymbol &assetSymbol);

        bool CheckOrderAmounts(CTxExecuteContext &context);
        bool CheckOrderAmount(CTxExecuteContext &context, const TokenSymbol &symbol,
                            const int64_t amount, const char *pSymbolSide);

        bool CheckOrderPrice(CTxExecuteContext &context);

        bool GetOrderOperator(CTxExecuteContext &context,
                              DexOperatorDetail &operatorDetail);
        bool GetOperatorAccount(CTxExecuteContext &context, CRegID &operatorRegid, CAccount &account);
        bool CheckOrderOperatorParam(CTxExecuteContext &context, DexOperatorDetail &operatorDetail,
                              CAccount &operatorAccount);

        bool FreezeBalance(CTxExecuteContext &context, CAccount &account,
                        const TokenSymbol &tokenSymbol, const uint64_t &amount, ReceiptType code);

    public:
        static uint64_t CalcCoinAmount(uint64_t assetAmount, const uint64_t price);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // CDEXBuyLimitOrderTx

    class CDEXBuyLimitOrderTx : public CDEXOrderBaseTx {

    public:
        CDEXBuyLimitOrderTx() : CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX) {
            order_type  = ORDER_LIMIT_PRICE;
            order_side  = ORDER_BUY;
            dex_id      = DEX_RESERVED_ID;
        }

        CDEXBuyLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                            uint64_t fees, const TokenSymbol &coinSymbolIn,
                            const TokenSymbol &assetSymbolIn, uint64_t assetAmountIn, uint64_t priceIn)
            : CDEXOrderBaseTx(DEX_LIMIT_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees){

            order_type   = ORDER_LIMIT_PRICE;
            order_side   = ORDER_BUY;
            coin_symbol  = coinSymbolIn;
            asset_symbol = assetSymbolIn;
            coin_amount  = 0;
            asset_amount = assetAmountIn;
            price        = priceIn;
            dex_id       = DEX_RESERVED_ID;
            // other order fields are default value
        }

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

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid
                    << fee_symbol << VARINT(llFees) << coin_symbol << asset_symbol
                    << VARINT(asset_amount) << VARINT(price);
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyLimitOrderTx>(*this); }

        virtual bool CheckTx(CTxExecuteContext &context);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // CDEXSellLimitOrderTx

    class CDEXSellLimitOrderTx : public CDEXOrderBaseTx {
    public:
        CDEXSellLimitOrderTx() : CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX) {
            order_type  = ORDER_LIMIT_PRICE;
            order_side  = ORDER_SELL;
            dex_id      = DEX_RESERVED_ID;
            // other order fields are default value
        }

        CDEXSellLimitOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbolIn,
                            const TokenSymbol &assetSymbolIn, uint64_t assetAmountIn, uint64_t priceIn)
            : CDEXOrderBaseTx(DEX_LIMIT_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees) {
            order_type   = ORDER_LIMIT_PRICE;
            order_side   = ORDER_SELL;
            coin_symbol  = coinSymbolIn;
            asset_symbol = assetSymbolIn;
            coin_amount  = 0;
            asset_amount = assetAmountIn;
            price        = priceIn;
            dex_id       = DEX_RESERVED_ID;
            // other order fields are default value
        }

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

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << coin_symbol << asset_symbol << VARINT(asset_amount)
            << VARINT(price);
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellLimitOrderTx>(*this); }

        virtual bool CheckTx(CTxExecuteContext &context);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // CDEXBuyMarketOrderTx

    class CDEXBuyMarketOrderTx : public CDEXOrderBaseTx {
    public:
        CDEXBuyMarketOrderTx() : CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX) {
            order_type   = ORDER_MARKET_PRICE;
            order_side   = ORDER_BUY;
            dex_id       = DEX_RESERVED_ID;
            // other order fields are default value
        }

        CDEXBuyMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbolIn,
                            const TokenSymbol &assetSymbolIn, uint64_t coinAmountIn)
            : CDEXOrderBaseTx(DEX_MARKET_BUY_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees) {
            order_type   = ORDER_MARKET_PRICE;
            order_side   = ORDER_BUY;
            coin_symbol  = coinSymbolIn;
            asset_symbol = assetSymbolIn;
            coin_amount  = coinAmountIn;
            asset_amount = 0;
            price        = 0;
            dex_id       = DEX_RESERVED_ID;
            // other order fields are default value
        }

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

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << coin_symbol << asset_symbol << VARINT(coin_amount);
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXBuyMarketOrderTx>(*this); }

        virtual bool CheckTx(CTxExecuteContext &context);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // CDEXSellMarketOrderTx

    class CDEXSellMarketOrderTx : public CDEXOrderBaseTx {
    public:
        CDEXSellMarketOrderTx() : CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX) {
            order_type   = ORDER_MARKET_PRICE;
            order_side   = ORDER_SELL;
            dex_id       = DEX_RESERVED_ID;
            // other order fields are default value
        }

        CDEXSellMarketOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, TokenSymbol coinSymbolIn,
                            const TokenSymbol &assetSymbolIn, uint64_t assetAmountIn)
            : CDEXOrderBaseTx(DEX_MARKET_SELL_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees) {
            order_type   = ORDER_MARKET_PRICE;
            order_side   = ORDER_SELL;
            coin_symbol  = coinSymbolIn;
            asset_symbol = assetSymbolIn;
            coin_amount  = 0;
            asset_amount = assetAmountIn;
            price        = 0;
            dex_id       = DEX_RESERVED_ID;
            // other order fields are default value
        }

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

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << coin_symbol << asset_symbol << VARINT(asset_amount);
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSellMarketOrderTx>(*this); }

        virtual bool CheckTx(CTxExecuteContext &context);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // CDEXOrderTx

    class CDEXOrderTx : public CDEXOrderBaseTx {
    public:
        struct OrderData {
            CDEXOrderTx &tx;

            OrderData(const CDEXOrderTx &txIn): tx(REF(txIn)) {}

            IMPLEMENT_SERIALIZE(
                READWRITE_ENUM(tx.order_type, uint8_t);
                READWRITE_ENUM(tx.order_side, uint8_t);
                READWRITE(tx.asset_symbol);
                READWRITE(tx.coin_symbol);
                READWRITE(VARINT(tx.asset_amount));
                READWRITE(VARINT(tx.coin_amount));
                READWRITE(VARINT(tx.price));
                READWRITE(VARINT(tx.dex_id));
                READWRITE(tx.memo);
            )
        };

    public:
        CDEXOrderTx() : CDEXOrderBaseTx(DEX_ORDER_TX) {}

        CDEXOrderTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                    uint64_t fees, OrderType orderTypeIn, OrderSide orderSideIn,
                    const TokenSymbol &coinSymbolIn, const TokenSymbol &assetSymbolIn,
                    uint64_t coinAmountIn, uint64_t assetAmountIn, uint64_t priceIn, DexID dexIdIn,
                    const string &memoIn)
            : CDEXOrderBaseTx(DEX_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees) {
            order_type   = orderTypeIn;
            order_side   = orderSideIn;
            coin_symbol  = coinSymbolIn;
            asset_symbol = assetSymbolIn;
            coin_amount  = coinAmountIn;
            asset_amount = assetAmountIn;
            price        = priceIn;
            dex_id       = dexIdIn;
            memo         = memoIn;
        }

        IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(this->nVersion));
            nVersion = this->nVersion;
            READWRITE(VARINT(valid_height));
            READWRITE(txUid);

            READWRITE(fee_symbol);
            READWRITE(VARINT(llFees));
            READWRITE(REF(OrderData(*this)));

            READWRITE(signature);
        )

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << OrderData(*this);
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXOrderTx>(*this); }
        virtual bool CheckTx(CTxExecuteContext &context);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // CDEXOperatorOrderTx

    class CDEXOperatorOrderTx : public CDEXOrderBaseTx {
    public:
        struct OrderData {
            CDEXOperatorOrderTx &tx;

            OrderData(const CDEXOperatorOrderTx &txIn): tx(REF(txIn)) {}

            IMPLEMENT_SERIALIZE(
                READWRITE_ENUM(tx.order_type, uint8_t);
                READWRITE_ENUM(tx.order_side, uint8_t);
                READWRITE(tx.asset_symbol);
                READWRITE(tx.coin_symbol);
                READWRITE(VARINT(tx.asset_amount));
                READWRITE(VARINT(tx.coin_amount));
                READWRITE(VARINT(tx.price));
                READWRITE(VARINT(tx.dex_id));
                READWRITE_ENUM(tx.public_mode, uint8_t);
                READWRITE(VARINT(tx.maker_fee_ratio));
                READWRITE(VARINT(tx.taker_fee_ratio));
                READWRITE(tx.operator_uid);
                READWRITE(VARINT(tx.operator_tx_fee));
                READWRITE(tx.memo);
            )
        };

    public:
        CDEXOperatorOrderTx() : CDEXOrderBaseTx(DEX_OPERATOR_ORDER_TX) {
            has_operator_config = true;
        }

        CDEXOperatorOrderTx(const CUserID &txUidIn, int32_t validHeightIn,
                            const TokenSymbol &feeSymbol, uint64_t fees, OrderType orderTypeIn,
                            OrderSide orderSideIn, const TokenSymbol &coinSymbolIn,
                            const TokenSymbol &assetSymbolIn, uint64_t coinAmountIn,
                            uint64_t assetAmountIn, uint64_t priceIn, DexID dexIdIn,
                            PublicMode publicModeIn, uint64_t makerFeeRatioIn,
                            uint64_t takerFeeRatioIn, const CUserID &operatorUidIn,
                            uint64_t operatorTxFeeIn, const string &memoIn)
            : CDEXOrderBaseTx(DEX_OPERATOR_ORDER_TX, txUidIn, validHeightIn, feeSymbol, fees) {
            order_type          = orderTypeIn;
            order_side          = orderSideIn;
            coin_symbol         = coinSymbolIn;
            asset_symbol        = assetSymbolIn;
            coin_amount         = coinAmountIn;
            asset_amount        = assetAmountIn;
            price               = priceIn;
            dex_id              = dexIdIn;
            public_mode         = publicModeIn;
            memo                = memoIn;
            has_operator_config = true;
            maker_fee_ratio     = makerFeeRatioIn;
            taker_fee_ratio     = takerFeeRatioIn;
            operator_uid        = operatorUidIn;
            operator_tx_fee     = operatorTxFeeIn;
        }

        IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(this->nVersion));
            nVersion = this->nVersion;
            READWRITE(VARINT(valid_height));
            READWRITE(txUid);

            READWRITE(fee_symbol);
            READWRITE(VARINT(llFees));
            READWRITE(REF(OrderData(*this)));

            READWRITE(signature);
            READWRITE(operator_signature);
        )

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << OrderData(*this);
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXOperatorOrderTx>(*this); }
        virtual bool CheckTx(CTxExecuteContext &context);
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

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << order_id;
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
    class CDEXSettleTx: public CBaseTx {
    public:
        struct DealItem  {
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
    public:
        vector<DealItem> dealItems;

        CDEXSettleTx() : CBaseTx(DEX_TRADE_SETTLE_TX) {}

        CDEXSettleTx(const CUserID &txUidIn, int32_t validHeightIn,
                    const TokenSymbol &feeSymbol, uint64_t fees,
                    const vector<DealItem> &dealItemsIn)
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

        virtual void SerializeForHash(CHashWriter &hw) const {
            hw << VARINT(nVersion) << (uint8_t)nTxType << VARINT(valid_height) << txUid << fee_symbol
            << VARINT(llFees) << dealItems;
        }

        virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXSettleTx>(*this); }

        virtual string ToString(CAccountDBCache &accountCache); //logging usage
        virtual Object ToJson(const CAccountDBCache &accountCache) const; //json-rpc usage

        virtual bool CheckTx(CTxExecuteContext &context);
        virtual bool ExecuteTx(CTxExecuteContext &context);
    };

} // namespace dex

#endif  // TX_DEX_H