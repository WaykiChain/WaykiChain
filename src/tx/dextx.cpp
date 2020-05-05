// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"

#include "config/configuration.h"
#include "entities/receipt.h"
#include "main.h"

#include <algorithm>

using namespace dex;

namespace dex {

    #define ERROR_TITLE(msg) (std::string(__FUNCTION__) + "(), " + msg)

    static bool CheckOperatorFeeRatioRange(CTxExecuteContext &context, const uint256 &orderId,
                                uint64_t feeRatio, const string &title) {
        static_assert(DEX_OPERATOR_FEE_RATIO_MAX < 100 * PRICE_BOOST, "fee rate must < 100%");
        if (feeRatio > DEX_OPERATOR_FEE_RATIO_MAX)
            return context.pState->DoS(100, ERRORMSG("%s(), operator fee_ratio=%llu is larger than %llu! order_id=%s",
                title, feeRatio, DEX_OPERATOR_FEE_RATIO_MAX, orderId.ToString(), feeRatio),
                REJECT_INVALID, "invalid-operator-fee-ratio");
        return true;
    }

    static bool GetDexOperator(CTxExecuteContext &context, const DexID &dexId,
                            DexOperatorDetail &operatorDetail, const string &title) {
        if (!context.pCw->dexCache.GetDexOperator(dexId, operatorDetail))
            return context.pState->DoS(100, ERRORMSG("%s(), the dex operator does not exist! dex_id=%u", title, dexId),
                REJECT_INVALID, "dex_operator_not_existed");
        return true;
    }

    uint64_t CalcOrderMinFee(uint64_t defaultMinFee, uint64_t stakedWiccAmount) {
        uint64_t stakedWiccCoins = stakedWiccAmount / COIN;
        if (stakedWiccCoins > MIN_STAKED_WICC_FOR_STEP_FEE) {
            return MIN_STAKED_WICC_FOR_STEP_FEE * defaultMinFee / stakedWiccCoins;
        } else {
            return defaultMinFee;
        }
    }

    // @return spErrMsg
    shared_ptr<string> CheckOrderMinFee(HeightType height, const CAccount &txAccount,
                                        const uint64_t txFee, uint64_t minFee,
                                        CAccount *pOperatorAccount, uint64_t operatorTxFee) {

        uint64_t totalFees = txFee;
        uint64_t stakedWiccAmount = 0;
        auto version = GetFeatureForkVersion(height);
        if (version >= MAJOR_VER_R3) {
            stakedWiccAmount = txAccount.GetToken(SYMB::WICC).staked_amount;
            if (pOperatorAccount != nullptr && operatorTxFee != 0) {
                totalFees += operatorTxFee;
                uint64_t op_staked_amount = pOperatorAccount->GetToken(SYMB::WICC).staked_amount;
                if (stakedWiccAmount < op_staked_amount) {
                    stakedWiccAmount = op_staked_amount;
                    LogPrint(BCLog::DEX, "Use operator stake amount=%llu instead", stakedWiccAmount);
                }
            }
            minFee = CalcOrderMinFee(minFee, stakedWiccAmount);
            if (totalFees < minFee){
                return make_shared<string>(strprintf("The given fees is too small: %llu < %llu sawi, staked_wicc_amount=%llu",
                    totalFees, minFee, stakedWiccAmount));
            }
        } else {
            if (totalFees < minFee){
                return make_shared<string>(strprintf("The given fees is too small: %llu < %llu sawi",
                    totalFees, minFee));
            }
        }
        return nullptr;
    }

    bool CheckOrderMinFee(const CBaseTx &tx, CTxExecuteContext &context, uint64_t minFee,
            CAccount *pOperatorAccount, uint64_t operatorTxFee) {

        auto spErr = CheckOrderMinFee(context.height, *tx.sp_tx_account, tx.llFees, minFee,
                                      pOperatorAccount, operatorTxFee);
        if (spErr)
            return context.pState->DoS(100, ERRORMSG("%s, %s, height=%d, fee_symbol=%s",
                TX_OBJ_ERR_TITLE(tx), *spErr, context.height, tx.fee_symbol), REJECT_INVALID, *spErr);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXOrderBaseTx

    bool CDEXOrderBaseTx::CheckTx(CTxExecuteContext &context) {
        CValidationState &state = *context.pState;

        IMPLEMENT_CHECK_TX_MEMO;

        if (!kOrderTypeHelper.CheckEnum(order_type))
            return context.pState->DoS(100, ERRORMSG("%s, invalid order_type=%u", TX_ERR_TITLE,
                    order_type), REJECT_INVALID, "invalid-order-side");

        if (!kOrderSideHelper.CheckEnum(order_side))
            return context.pState->DoS(100, ERRORMSG("%s, invalid order_side=%u", TX_ERR_TITLE,
                    order_side), REJECT_INVALID, "invalid-order-side");

        if (!CheckOrderSymbols(context, coin_symbol, asset_symbol)) return false;

        if (!CheckOrderAmounts(context)) return false;

        if (!CheckOrderPrice(context)) return false;

        DexOperatorDetail operatorDetail;
        if (!GetOrderOperator(context, operatorDetail))
            return false;

        auto spOperatorAccount = GetAccount(context, operatorDetail.fee_receiver_regid, "fee_receiver");
        if (!spOperatorAccount) return false;

        if (!CheckOrderOperatorParam(context, operatorDetail, *spOperatorAccount)) return false;

        return true;
    }


    bool CDEXOrderBaseTx::ExecuteTx(CTxExecuteContext &context) {
        CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

        if (has_operator_config && operator_tx_fee > 0) {
            DexOperatorDetail operatorDetail;
            if (!GetOrderOperator(context, operatorDetail))
                return false;

            auto spOperatorAccount = GetAccount(context, operatorDetail.fee_receiver_regid, "fee_receiver");
            if (!spOperatorAccount) return false;

            if (!spOperatorAccount->OperateBalance(fee_symbol, SUB_FREE, operator_tx_fee,
                                                ReceiptType::BLOCK_REWARD_TO_MINER, receipts)) {
                return state.DoS(100, ERRORMSG("%s, operator account has insufficient funds for tx fee",
                                ERROR_TITLE(GetTxTypeName())), UPDATE_ACCOUNT_FAIL, "operator-account-insufficient");
            }
        }

        uint64_t coinAmount = coin_amount;
        if (order_type == ORDER_LIMIT_PRICE && order_side == ORDER_BUY)
            coinAmount = CDEXOrderBaseTx::CalcCoinAmount(asset_amount, price);

        if (order_side == ORDER_BUY) {
            if (!FreezeBalance(context, *sp_tx_account, coin_symbol, coinAmount, ReceiptType::DEX_FREEZE_COIN_TO_BUYER))
                return false;
        } else {
            assert(order_side == ORDER_SELL);
            if (!FreezeBalance(context, *sp_tx_account, asset_symbol, asset_amount, ReceiptType::DEX_FREEZE_ASSET_TO_SELLER))
                return false;
        }

        assert(!sp_tx_account->regid.IsEmpty());
        const uint256 &txid = GetHash();
        CDEXOrderDetail orderDetail;
        orderDetail.generate_type      = USER_GEN_ORDER;
        orderDetail.order_type         = order_type;
        orderDetail.order_side         = order_side;
        orderDetail.coin_symbol        = coin_symbol;
        orderDetail.asset_symbol       = asset_symbol;
        orderDetail.coin_amount        = coinAmount;
        orderDetail.asset_amount       = asset_amount;
        orderDetail.price              = price;
        orderDetail.dex_id             = dex_id;
        orderDetail.tx_cord            = CTxCord(context.height, context.index);
        orderDetail.user_regid         = sp_tx_account->regid;
        if (has_operator_config) {
            orderDetail.opt_operator_params  = {
                open_mode,
                maker_fee_ratio,
                taker_fee_ratio,
            };
        }
        // other fields keep as is

        if (!cw.dexCache.CreateActiveOrder(txid, orderDetail))
            return context.pState->DoS(100, ERRORMSG("%s, create active buy order failed! txid=%s",
                TX_ERR_TITLE, txid.ToString()), REJECT_INVALID, "bad-write-dexdb");

        return true;
    }

    string CDEXOrderBaseTx::ToString(CAccountDBCache &accountCache) {
        return  strprintf("txType=%s", GetTxType(nTxType)) + ", " +
                strprintf("hash=%s", GetHash().GetHex()) + ", " +
                strprintf("ver=%d", nVersion) + ", " +
                strprintf("valid_height=%d", valid_height) + ", " +
                strprintf("tx_uid=%s", txUid.ToDebugString()) + ", " +
                strprintf("fee_symbol=%llu", fee_symbol) + ", " +
                strprintf("fees=%llu", llFees) + ", " +
                strprintf("signature=%s", HexStr(signature)) + ", " +
                strprintf("order_type=%s", kOrderTypeHelper.GetName(order_type)) + ", " +
                strprintf("order_side=%s", kOrderSideHelper.GetName(order_side)) + ", " +
                strprintf("coin_symbol=%s", coin_symbol) + ", " +
                strprintf("asset_symbol=%s", asset_symbol) + ", " +
                strprintf("coin_amount=%llu", coin_amount) + ", " +
                strprintf("asset_amount=%llu", asset_amount) + ", " +
                strprintf("price=%llu", price) + ", " +
                strprintf("dex_id=%u", dex_id) + ", " +
                strprintf("has_operator_config=%d", has_operator_config) + ", " +
                strprintf("operator_taker_fee_ratio=%llu", taker_fee_ratio) + ", " +
                strprintf("operator_maker_fee_ratio=%llu", maker_fee_ratio) + ", " +
                strprintf("operator_uid=%s", operator_uid.ToDebugString()) + ", " +
                strprintf("operator_signature=%s", HexStr(operator_signature)) + ", " +
                strprintf("memo_hex=%s", nVersion);
    }

    Object CDEXOrderBaseTx::ToJson(const CAccountDBCache &accountCache) const {
        Object result = CBaseTx::ToJson(accountCache);
        result.push_back(Pair("order_type",         kOrderTypeHelper.GetName(order_type)));
        result.push_back(Pair("order_side",         kOrderSideHelper.GetName(order_side)));
        result.push_back(Pair("coin_symbol",        coin_symbol));
        result.push_back(Pair("asset_symbol",       asset_symbol));
        result.push_back(Pair("coin_amount",        coin_amount));
        result.push_back(Pair("asset_amount",       asset_amount));
        result.push_back(Pair("price",              price));
        result.push_back(Pair("dex_id",             (uint64_t)dex_id));
        result.push_back(Pair("has_operator_config",  has_operator_config));
        if (has_operator_config) {
            CKeyID operatorKeyid;
            accountCache.GetKeyId(operator_uid, operatorKeyid);
            result.push_back(Pair("taker_fee_ratio",    taker_fee_ratio));
            result.push_back(Pair("maker_fee_ratio",    maker_fee_ratio));
            result.push_back(Pair("operator_uid",       operator_uid.ToString()));
            result.push_back(Pair("operator_addr",      operatorKeyid.ToAddress()));
            result.push_back(Pair("operator_signature", HexStr(operator_signature)));
        }
        result.push_back(Pair("memo",               memo));
        result.push_back(Pair("memo_hex",           HexStr(memo)));
        return result;
    }

    bool CDEXOrderBaseTx::CheckMinFee(CTxExecuteContext &context, uint64_t minFee) {

        if (has_operator_config && operator_tx_fee != 0) {
            DexOperatorDetail operatorDetail;
            if (!GetOrderOperator(context, operatorDetail)) return false;
            auto spOperatorAccount= GetAccount(context, operatorDetail.fee_receiver_regid, "operator");
            if (!spOperatorAccount) return false;
            return dex::CheckOrderMinFee(*this, context, minFee, spOperatorAccount.get(), operator_tx_fee);
        } else {
            return dex::CheckOrderMinFee(*this, context, minFee, nullptr, 0);
        }
    }

    bool CDEXOrderBaseTx::CheckOrderSymbols(CTxExecuteContext &context, const TokenSymbol &coinSymbol,
                                            const TokenSymbol &assetSymbol) {

        if (coinSymbol == assetSymbol)
            return context.pState->DoS(100,
                                       ERRORMSG("%s, the coinSymbol=%s is same to assetSymbol=%s",
                                                TX_ERR_TITLE, coinSymbol, assetSymbol),
                                       REJECT_INVALID, "same-coin-asset-symbol");
        if (!context.pCw->assetCache.CheckDexBaseSymbol(assetSymbol))
            return context.pState->DoS(100, ERRORMSG("%s, unsupported dex order asset_symbol=%s", TX_ERR_TITLE, assetSymbol),
                                    REJECT_INVALID, "unsupported-order-asset-symbol");
        if (!context.pCw->assetCache.CheckDexQuoteSymbol(coinSymbol))
            return context.pState->DoS(100, ERRORMSG("%s, unsupported dex order coin_symbol=%s", TX_ERR_TITLE, coinSymbol),
                                    REJECT_INVALID, "unsupported-order-coin-symbol");
        return true;
    }

    bool CDEXOrderBaseTx::CheckOrderAmounts(CTxExecuteContext &context) {

        static_assert(MIN_DEX_ORDER_AMOUNT < INT64_MAX, "minimum dex order amount out of range");
        if (order_type == ORDER_MARKET_PRICE && order_side == ORDER_BUY) {
            if (asset_amount != 0)
                return context.pState->DoS(100, ERRORMSG("%s, asset amount=%llu must be 0 when order_type=%s, order_side=%s",
                        TX_ERR_TITLE, asset_amount, kOrderTypeHelper.GetName(order_type),
                        kOrderSideHelper.GetName(order_side)), REJECT_INVALID, "asset-amount-not-zero");
            if (!CheckOrderAmount(context, coin_symbol, coin_amount, "coin")) return false;
        } else {
            if (coin_amount != 0)
                return context.pState->DoS(100, ERRORMSG("%s, coin amount=%llu must be 0 when order_type=%s, order_side=%s",
                        TX_ERR_TITLE, coin_amount, kOrderTypeHelper.GetName(order_type),
                        kOrderSideHelper.GetName(order_side)), REJECT_INVALID, "coin-amount-not-zero");

            if (!CheckOrderAmount(context, asset_symbol, asset_amount, "asset")) return false;
        }
        return true;
    }

    bool CDEXOrderBaseTx::CheckOrderAmount(CTxExecuteContext &context, const TokenSymbol &symbol, const int64_t amount,
        const char *pSymbolSide) {

        if (amount < (int64_t)MIN_DEX_ORDER_AMOUNT)
            return context.pState->DoS(100, ERRORMSG("%s, %s amount is too small, symbol=%s, amount=%llu, min_amount=%llu",
                    TX_ERR_TITLE, pSymbolSide, symbol, amount, MIN_DEX_ORDER_AMOUNT),
                    REJECT_INVALID, "order-amount-too-small");

        if (!CheckCoinRange(symbol, amount))
            return context.pState->DoS(100, ERRORMSG("%s, %s amount is out of range, symbol=%s, amount=%llu",
                    TX_ERR_TITLE, pSymbolSide, symbol, amount),
                    REJECT_INVALID, "invalid-order-amount-range");

        return true;
    }

    bool CDEXOrderBaseTx::CheckOrderPrice(CTxExecuteContext &context) {
        if (order_type == ORDER_MARKET_PRICE) {
            if (price != 0)
                return context.pState->DoS(100, ERRORMSG("%s, price must be 0 when order_type=%s",
                        TX_ERR_TITLE, price, kOrderTypeHelper.GetName(order_type)),
                        REJECT_INVALID, "order-price-not-zero");
        } else {
            assert(order_type == ORDER_LIMIT_PRICE);
            // TODO: should check the price range??
            if (price <= 0)
                return context.pState->DoS(100, ERRORMSG("%s price=%llu out of range, order_type=%s",
                        TX_ERR_TITLE, price, kOrderTypeHelper.GetName(order_type)),
                        REJECT_INVALID, "invalid-price-range");
        }
        return true;
    }

    bool CDEXOrderBaseTx::CheckOrderOperatorParam(CTxExecuteContext &context,
                                                  DexOperatorDetail &operatorDetail,
                                                  CAccount &operatorAccount) const {

        if (has_operator_config) {
            const auto &hash = GetHash();

            if (!kOpenModeHelper.CheckEnum(open_mode))
                return context.pState->DoS(100, ERRORMSG("%s, invalid open_mode=%s", TX_ERR_TITLE,
                        kOpenModeHelper.GetName(open_mode)), REJECT_INVALID, "invalid-open-mode");

            if (!CheckOperatorFeeRatioRange(context, hash, taker_fee_ratio, TX_ERR_TITLE + ", taker_fee_ratio"))
                return false;
            if (!CheckOperatorFeeRatioRange(context, hash, maker_fee_ratio, TX_ERR_TITLE + ", maker_fee_ratio"))
                return false;

            if (!operator_uid.is<CRegID>())
                return context.pState->DoS(100, ERRORMSG("%s(), dex operator uid must be regid, operator_uid=%s",
                    TX_ERR_TITLE, operator_uid.ToDebugString()),
                    REJECT_INVALID, "operator-uid-not-regid");

            const CRegID &operator_regid = operator_uid.get<CRegID>();
            if (operator_regid != operatorDetail.fee_receiver_regid)
                return context.pState->DoS(100, ERRORMSG("%s(), the dex operator uid is wrong, operator_uid=%s",
                    TX_ERR_TITLE, operator_uid.ToDebugString()),
                    REJECT_INVALID, "operator-uid-wrong");

            if (!CheckSignatureSize(operator_signature)) {
                return context.pState->DoS(100, ERRORMSG("%s, operator signature size=%d invalid",
                    TX_ERR_TITLE, operator_signature.size()), REJECT_INVALID, "bad-operator-sig-size");
            }
            uint256 sighash = GetHash();
            if (!::VerifySignature(sighash, operator_signature, operatorAccount.owner_pubkey)) {
                return context.pState->DoS(100, ERRORMSG("%s, check operator signature error",
                    TX_ERR_TITLE), REJECT_INVALID, "bad-operator-signature");
            }
        }

        return true;
    }

    bool CDEXOrderBaseTx::GetOrderOperator(CTxExecuteContext &context,
                            DexOperatorDetail &operatorDetail) const {

        if (!GetDexOperator(context, dex_id, operatorDetail, TX_ERR_TITLE))
            return false;

        if (!operatorDetail.activated)
            return context.pState->DoS(
                100, ERRORMSG("%s, dex operator is inactived! dex_id=%d", TX_ERR_TITLE, dex_id),
                REJECT_INVALID, "dex-operator-inactived");
        return true;
    }

    bool CDEXOrderBaseTx::FreezeBalance(CTxExecuteContext &context, CAccount &account, const TokenSymbol &tokenSymbol,
                                        const uint64_t &amount, ReceiptType code) {

        if (!account.OperateBalance(tokenSymbol, FREEZE, amount, code, receipts)) {
            return context.pState->DoS(100,
                ERRORMSG("%s, account has insufficient funds! regid=%s, symbol=%s, amount=%llu", TX_ERR_TITLE,
                        account.regid.ToString(), tokenSymbol, amount),
                UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
        }
        return true;
    }

    uint64_t CDEXOrderBaseTx::CalcCoinAmount(uint64_t assetAmount, const uint64_t price) {
        uint128_t coinAmount = assetAmount * (uint128_t)price / PRICE_BOOST;
        assert(coinAmount < ULLONG_MAX);
        return (uint64_t)coinAmount;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXBuyLimitOrderTx

    bool CDEXBuyLimitOrderTx::CheckTx(CTxExecuteContext &context) {
        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R2)) return false;
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXSellLimitOrderTx

    bool CDEXSellLimitOrderTx::CheckTx(CTxExecuteContext &context) {
        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R2)) return false;
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXBuyMarketOrderTx

    bool CDEXBuyMarketOrderTx::CheckTx(CTxExecuteContext &context) {
        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R2)) return false;
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXSellMarketOrderTx

    bool CDEXSellMarketOrderTx::CheckTx(CTxExecuteContext &context) {
        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R2)) return false;
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXOrderTx

    bool CDEXOrderTx::CheckTx(CTxExecuteContext &context) {
        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R3)) return false;
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXOperatorOrderTx

    bool CDEXOperatorOrderTx::CheckTx(CTxExecuteContext &context) {
        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R3)) return false;
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXCancelOrderTx

    string CDEXCancelOrderTx::ToString(CAccountDBCache &accountCache) {
        return strprintf(
            "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, order_id=%s",
            GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
            order_id.GetHex());
    }

    Object CDEXCancelOrderTx::ToJson(const CAccountDBCache &accountCache) const {
        Object result = CBaseTx::ToJson(accountCache);
        result.push_back(Pair("order_id", order_id.GetHex()));

        return result;
    }

    bool CDEXCancelOrderTx::CheckTx(CTxExecuteContext &context) {
        CValidationState &state = *context.pState;

        if (order_id.IsEmpty())
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, order_id is empty"), REJECT_INVALID,
                            "invalid-order-id");

        return true;
    }

    bool CDEXCancelOrderTx::ExecuteTx(CTxExecuteContext &context) {
        IMPLEMENT_DEFINE_CW_STATE;

        CDEXOrderDetail activeOrder;
        if (!cw.dexCache.GetActiveOrder(order_id, activeOrder)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, the order is inactive or not existed"),
                            REJECT_INVALID, "order-inactive");
        }

        if (activeOrder.generate_type != USER_GEN_ORDER) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, the order is not generate by tx of user"),
                            REJECT_INVALID, "order-inactive");
        }

        if (sp_tx_account->regid.IsEmpty() || sp_tx_account->regid != activeOrder.user_regid) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, can not cancel other user's order tx"),
                            REJECT_INVALID, "user-unmatched");
        }

        // get frozen money
        TokenSymbol frozenSymbol;
        uint64_t frozenAmount = 0;
        ReceiptType code;
        if (activeOrder.order_side == ORDER_BUY) {
            frozenSymbol = activeOrder.coin_symbol;
            frozenAmount = activeOrder.coin_amount - activeOrder.total_deal_coin_amount;
            code = ReceiptType::DEX_UNFREEZE_COIN_TO_BUYER;

        } else if(activeOrder.order_side == ORDER_SELL) {
            frozenSymbol = activeOrder.asset_symbol;
            frozenAmount = activeOrder.asset_amount - activeOrder.total_deal_asset_amount;
            code = ReceiptType::DEX_UNFREEZE_ASSET_TO_SELLER;
        } else {
            assert(false && "Order side must be ORDER_BUY|ORDER_SELL");
        }

        if (!sp_tx_account->OperateBalance(frozenSymbol, UNFREEZE, frozenAmount, code, receipts))
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, account has insufficient frozen amount to unfreeze"),
                            UPDATE_ACCOUNT_FAIL, "unfreeze-account-coin");

        if (!cw.dexCache.EraseActiveOrder(order_id, activeOrder)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, erase active order failed! order_id=%s", order_id.ToString()),
                            REJECT_INVALID, "order-erase-failed");
        }

        return true;
    }


    bool CDEXCancelOrderTx::CheckMinFee(CTxExecuteContext &context, uint64_t minFee) {
        return dex::CheckOrderMinFee(*this, context, minFee, nullptr, 0);
    }

    #define DEAL_ITEM_TITLE ERROR_TITLE(tx.GetTxTypeName() + strprintf(", i[%d]", idx))

////////////////////////////////////////////////////////////////////////////////
// class CDealItemExecuter
    class CDealItemExecuter {
    public:
        typedef CDEXSettleTx::DealItem DealItem;
    public:
        // input data
        DealItem &dealItem;
        uint32_t idx;       // index of deal item
        CDEXSettleTx &tx;
        CTxExecuteContext &context;
        shared_ptr<CAccount> &pTxAccount;
        map<CRegID, shared_ptr<CAccount>> &accountMap;
        vector<CReceipt> &receipts;

        // found data
        CDEXOrderDetail buyOrder;
        CDEXOrderDetail sellOrder;
        shared_ptr<CAccount> spBuyOrderAccount;
        shared_ptr<CAccount> spSellOrderAccount;
        DexOperatorDetail buyOperatorDetail;
        DexOperatorDetail sellOperatorDetail;
        shared_ptr<CAccount> spBuyOpAccount;
        shared_ptr<CAccount> spSellOpAccount;
        COrderOperatorParams buyOrderOperatorParams;
        COrderOperatorParams sellOrderOperatorParams;
        OrderSide takerSide;

        CDealItemExecuter(DealItem &dealItemIn, uint32_t idxIn, CDEXSettleTx &txIn,
                          CTxExecuteContext &contextIn, shared_ptr<CAccount> &pTxAccountIn,
                          map<CRegID, shared_ptr<CAccount>> &accountMapIn,
                          vector<CReceipt> &receiptsIn)
            : dealItem(dealItemIn), idx(idxIn), tx(txIn), context(contextIn),
              pTxAccount(pTxAccountIn), accountMap(accountMapIn), receipts(receiptsIn) {}

        bool Execute();

        COrderOperatorParams GetOrderOperatorParams(CDEXOrderDetail &order, DexOperatorDetail &operatorDetail);

        bool CheckOrderOpenMode();
        bool CheckTotalTradingFees(uint64_t dealAmount, uint64_t frictionFee, uint64_t dealFee,
                                   const char *name);

        bool GetDealOrder(const uint256 &orderId, const OrderSide orderSide,
                          CDEXOrderDetail &dealOrder);

        OrderSide GetTakerOrderSide();

        uint64_t GetOperatorFeeRatio(const CDEXOrderDetail &order,
                                     const COrderOperatorParams &orderOperatorParams,
                                     const OrderSide &takerSide);

        bool CalcDealFee(const uint256 &orderId, const CDEXOrderDetail &order,
                          const COrderOperatorParams &orderOperatorParams,
                          const OrderSide &takerSide, uint64_t amount, uint64_t &orderFee);

        bool CalcWusdFrictionFee(uint64_t amount, uint64_t &frictionFee);

        bool ProcessWusdFrictionFee(CAccount &fromAccount, uint64_t amount, uint64_t frictionFee);
    };

    /* process flow for settle tx
        1. get and check buyDealOrder and sellDealOrder
            a. get and check active order from db
            b. get and check order detail
                I. if order is USER_GEN_ORDER:
                    step 1. get and check order tx object from block file
                    step 2. get order detail from tx object
                II. if order is SYS_GEN_ORDER:
                    step 1. get sys order object from dex db
                    step 2. get order detail from sys order object
        2. get account of order's owner
            a. get buyOderAccount from account db
            b. get sellOderAccount from account db
        3. check coin type match
            buyOrder.coin_symbol == sellOrder.coin_symbol
        4. check asset type match
            buyOrder.asset_symbol == sellOrder.asset_symbol
        5. check price match
            a. limit type <-> limit type
                I.   dealPrice <= buyOrder.price
                II.  dealPrice >= sellOrder.price
            b. limit type <-> market type
                I.   dealPrice == buyOrder.price
            c. market type <-> limit type
                I.   dealPrice == sellOrder.price
            d. market type <-> market type
                no limit
        6. check and operate deal amount
            a. check: dealCoinAmount == CalcCoinAmount(dealAssetAmount, price)
            b. else check: (dealCoinAmount / 10000) == (CalcCoinAmount(dealAssetAmount, price) /
        10000) c. operate total deal: buyActiveOrder.total_deal_coin_amount  += dealCoinAmount
                buyActiveOrder.total_deal_asset_amount += dealAssetAmount
                sellActiveOrder.total_deal_coin_amount  += dealCoinAmount
                sellActiveOrder.total_deal_asset_amount += dealAssetAmount
        7. check the order limit amount and get residual amount
            a. buy order
                if market price order {
                    limitCoinAmount = buyOrder.coin_amount
                    check: limitCoinAmount >= buyActiveOrder.total_deal_coin_amount
                    residualAmount = limitCoinAmount - buyActiveOrder.total_deal_coin_amount
                } else { //limit price order
                    limitAssetAmount = buyOrder.asset_amount
                    check: limitAssetAmount >= buyActiveOrder.total_deal_asset_amount
                    residualAmount = limitAssetAmount - buyActiveOrder.total_deal_asset_amount
                }
            b. sell order
                limitAssetAmount = sellOrder.limitAmount
                check: limitAssetAmount >= sellActiveOrder.total_deal_asset_amount
                residualAmount = limitAssetAmount - dealCoinAmount

        8. subtract the buyer's coins and seller's assets
            a. buyerFrozenCoins     -= dealCoinAmount
            b. sellerFrozenAssets   -= dealAssetAmount
        9. calc deal fees
            buyerReceivedAssets = dealAssetAmount
            if buy order is USER_GEN_ORDER
                dealAssetFee = dealAssetAmount * 0.04%
                buyerReceivedAssets -= dealAssetFee
                add dealAssetFee to totalFee of tx
            sellerReceivedAssets = dealCoinAmount
            if buy order is SYS_GEN_ORDER
                dealCoinFee = dealCoinAmount * 0.04%
                sellerReceivedCoins -= dealCoinFee
                add dealCoinFee to totalFee of tx
        10. add the buyer's assets and seller's coins
            a. buyerAssets          += dealAssetAmount - dealAssetFee
            b. sellerCoins          += dealCoinAmount - dealCoinFee
        11. check order fullfiled or save residual amount
            a. buy order
                if buy order is fulfilled {
                    if buy limit order {
                        residualCoinAmount = buyOrder.coin_amount -
        buyActiveOrder.total_deal_coin_amount if residualCoinAmount > 0 {
                            buyerUnfreeze(residualCoinAmount)
                        }
                    }
                    erase active order from dex db
                } else {
                    update active order to dex db
                }
            a. sell order
                if sell order is fulfilled {
                    erase active order from dex db
                } else {
                    update active order to dex db
                }
    */
    bool CDealItemExecuter::Execute() {
        CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

        //1.1 get and check buyDealOrder and sellDealOrder
        if (!GetDealOrder(dealItem.buyOrderId, ORDER_BUY, buyOrder)) return false;

        if (!GetDealOrder(dealItem.sellOrderId, ORDER_SELL, sellOrder)) return false;

        // 1.2 get account of order

        spBuyOrderAccount = tx.GetAccount(context, buyOrder.user_regid, "buyer");
        if (!spBuyOrderAccount) return false;

        spSellOrderAccount = tx.GetAccount(context, sellOrder.user_regid, "seller");
        if (!spSellOrderAccount) return false;

        // 1.3 get operator info
        if (!GetDexOperator(context, buyOrder.dex_id, buyOperatorDetail, DEAL_ITEM_TITLE)) return false;

        spBuyOpAccount = tx.GetAccount(context, buyOperatorDetail.fee_receiver_regid, "buy_operator");
        if (!spBuyOpAccount) return false;

        if (!GetDexOperator(context, sellOrder.dex_id, sellOperatorDetail, DEAL_ITEM_TITLE)) return false;

        spSellOpAccount = tx.GetAccount(context, sellOperatorDetail.fee_receiver_regid, "sell_operator");
        if (!spSellOpAccount) return false;

        // 1.4 get order operator params
        buyOrderOperatorParams = GetOrderOperatorParams(buyOrder, buyOperatorDetail);
        sellOrderOperatorParams = GetOrderOperatorParams(sellOrder, sellOperatorDetail);

        // 1.5 get taker side
        takerSide = GetTakerOrderSide();

        // 2. check coin type match
        if (buyOrder.coin_symbol != sellOrder.coin_symbol) {
            return state.DoS(100, ERRORMSG("%s, coin symbol unmatch! buyer coin_symbol=%s, " \
                "seller coin_symbol=%s", DEAL_ITEM_TITLE, buyOrder.coin_symbol, sellOrder.coin_symbol),
                REJECT_INVALID, "coin-symbol-unmatch");
        }
        // 3. check asset type match
        if (buyOrder.asset_symbol != sellOrder.asset_symbol) {
            return state.DoS(100, ERRORMSG("%s, asset symbol unmatch! buyer asset_symbol=%s, " \
                "seller asset_symbol=%s", DEAL_ITEM_TITLE, buyOrder.asset_symbol, sellOrder.asset_symbol),
                REJECT_INVALID, "asset-symbol-unmatch");
        }

        // 4. check price match
        if (buyOrder.order_type == ORDER_LIMIT_PRICE && sellOrder.order_type == ORDER_LIMIT_PRICE) {
            if ( buyOrder.price < dealItem.dealPrice
                || sellOrder.price > dealItem.dealPrice ) {
                return state.DoS(100, ERRORMSG("%s, the expected price unmatch! buyer limit price=%llu, "
                    "seller limit price=%llu, deal_price=%llu",
                    DEAL_ITEM_TITLE, buyOrder.price, sellOrder.price, dealItem.dealPrice),
                    REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrder.order_type == ORDER_LIMIT_PRICE && sellOrder.order_type == ORDER_MARKET_PRICE) {
            if (dealItem.dealPrice != buyOrder.price) {
                return state.DoS(100, ERRORMSG("%s, the expected price unmatch! buyer limit price=%llu, "
                    "seller market price, deal_price=%llu",
                    DEAL_ITEM_TITLE, buyOrder.price, dealItem.dealPrice),
                    REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrder.order_type == ORDER_MARKET_PRICE && sellOrder.order_type == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice != sellOrder.price) {
                return state.DoS(100, ERRORMSG("%s, the expected price unmatch! buyer market price, "
                    "seller limit price=%llu, deal_price=%llu",
                    DEAL_ITEM_TITLE, sellOrder.price, dealItem.dealPrice),
                    REJECT_INVALID, "deal-price-unmatch");
            }
        } else {
            assert(buyOrder.order_type == ORDER_MARKET_PRICE && sellOrder.order_type == ORDER_MARKET_PRICE);
            // no limit
        }

        // 5. check cross exchange trading with public mode
        if (!CheckOrderOpenMode()) return false;

        // 6. check and operate deal amount
        uint64_t calcCoinAmount = CDEXOrderBaseTx::CalcCoinAmount(dealItem.dealAssetAmount, dealItem.dealPrice);
        int64_t dealAmountDiff = calcCoinAmount - dealItem.dealCoinAmount;
        bool isCoinAmountMatch = false;
        if (buyOrder.order_type == ORDER_MARKET_PRICE) {
            isCoinAmountMatch = (std::abs(dealAmountDiff) <= std::max<int64_t>(1, (1 * dealItem.dealPrice / PRICE_BOOST)));
        } else {
            isCoinAmountMatch = (dealAmountDiff == 0);
        }
        if (!isCoinAmountMatch)
            return state.DoS(100, ERRORMSG("%s, the deal_coin_amount unmatch! deal_info={%s}, calcCoinAmount=%llu",
                DEAL_ITEM_TITLE, dealItem.ToString(), calcCoinAmount),
                REJECT_INVALID, "deal-coin-amount-unmatch");

        buyOrder.total_deal_coin_amount += dealItem.dealCoinAmount;
        buyOrder.total_deal_asset_amount += dealItem.dealAssetAmount;
        sellOrder.total_deal_coin_amount += dealItem.dealCoinAmount;
        sellOrder.total_deal_asset_amount += dealItem.dealAssetAmount;

        // 7. check the order amount limits and get residual amount
        uint64_t buyResidualAmount  = 0;
        uint64_t sellResidualAmount = 0;

        if (buyOrder.order_type == ORDER_MARKET_PRICE) {
            uint64_t limitCoinAmount = buyOrder.coin_amount;
            if (limitCoinAmount < buyOrder.total_deal_coin_amount) {
                return state.DoS(100, ERRORMSG( "%s, the total_deal_coin_amount=%llu exceed the buyer's "
                    "coin_amount=%llu", DEAL_ITEM_TITLE, buyOrder.total_deal_coin_amount, limitCoinAmount),
                    REJECT_INVALID, "buy-deal-coin-amount-exceeded");
            }

            buyResidualAmount = limitCoinAmount - buyOrder.total_deal_coin_amount;
        } else {
            uint64_t limitAssetAmount = buyOrder.asset_amount;
            if (limitAssetAmount < buyOrder.total_deal_asset_amount) {
                return state.DoS(
                    100,
                    ERRORMSG("%s, the total_deal_asset_amount=%llu exceed the "
                            "buyer's asset_amount=%llu",
                            DEAL_ITEM_TITLE, buyOrder.total_deal_asset_amount, limitAssetAmount),
                    REJECT_INVALID, "buy-deal-amount-exceeded");
            }
            buyResidualAmount = limitAssetAmount - buyOrder.total_deal_asset_amount;
        }

        {
            // get and check sell order residualAmount
            uint64_t limitAssetAmount = sellOrder.asset_amount;
            if (limitAssetAmount < sellOrder.total_deal_asset_amount) {
                return state.DoS(
                    100,
                    ERRORMSG("%s, the total_deal_asset_amount=%llu exceed the "
                            "seller's asset_amount=%llu",
                            DEAL_ITEM_TITLE, sellOrder.total_deal_asset_amount, limitAssetAmount),
                    REJECT_INVALID, "sell-deal-amount-exceeded");
            }
            sellResidualAmount = limitAssetAmount - sellOrder.total_deal_asset_amount;
        }

        FeatureForkVersionEnum version = GetFeatureForkVersion(context.height);

        // 8. calc WUSD friction fee
        uint64_t frictionCoinFee = 0;
        uint64_t frictionAssetFee = 0;
        if (version >= FeatureForkVersionEnum::MAJOR_VER_R3) {
            if (buyOrder.coin_symbol == SYMB::WUSD) {
                if (!CalcWusdFrictionFee(dealItem.dealCoinAmount, frictionCoinFee)) return false;
            } else if (sellOrder.asset_symbol == SYMB::WUSD) {
                if (!CalcWusdFrictionFee(dealItem.dealAssetAmount, frictionCoinFee)) return false;
            }
        }

        // 9. calc deal fees for dex operator
        // 9.1. calc deal asset fee payed by buyer for buy operator
        uint64_t dealAssetFee = 0;
        if (!CalcDealFee(dealItem.buyOrderId, buyOrder, buyOrderOperatorParams, takerSide,
                          dealItem.dealAssetAmount, dealAssetFee))
            return false;
        // 9.2. calc deal coin fee payed by seller for sell operator
        uint64_t dealCoinFee = 0;
        if (!CalcDealFee(dealItem.sellOrderId, sellOrder, sellOrderOperatorParams, takerSide,
                          dealItem.dealCoinAmount, dealCoinFee))
            return false;

        // 10. check total trading fees
        if (!CheckTotalTradingFees(dealItem.dealCoinAmount, frictionCoinFee, dealCoinFee, "coin")) return false;
        if (!CheckTotalTradingFees(dealItem.dealAssetAmount, frictionAssetFee, dealAssetFee, "asset")) return false;

        // 11. Deal for the coins and assets
        // 11.1 buyer's coins -> seller
        if (!spBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, DEX_DEAL, dealItem.dealCoinAmount,
                                              ReceiptType::DEX_COIN_TO_SELLER, receipts, spSellOrderAccount.get())) {
            return state.DoS(100, ERRORMSG("%s, deal buyer's coins failed! deal_info={%s}, coin_symbol=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), buyOrder.coin_symbol),
                    REJECT_INVALID, "deal-buyer-coins-failed");
        }
        // 11.2 seller's assets -> buyer
        if (!spSellOrderAccount->OperateBalance(sellOrder.asset_symbol, DEX_DEAL, dealItem.dealAssetAmount,
                                              ReceiptType::DEX_ASSET_TO_BUYER, receipts, spBuyOrderAccount.get())) {
            return state.DoS(100, ERRORMSG("%s, deal seller's assets failed! deal_info={%s}, asset_symbol=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), sellOrder.asset_symbol),
                    REJECT_INVALID, "deal-seller-assets-failed");
        }

        // 12. transfer the deal fee of coins and assets to dex operators
        // 12.1. transfer deal coin fee from seller to sell operator
        if (!spSellOrderAccount->OperateBalance(sellOrder.coin_symbol, SUB_FREE, dealCoinFee,
                                               ReceiptType::DEX_COIN_FEE_TO_OPERATOR, receipts,
                                               spSellOpAccount.get())) {
            return state.DoS(100, ERRORMSG("%s, transfer deal coin fee from seller to sell operator failed!"
                    " deal_info={%s}, coin_symbol=%s, coin_fee=%llu, sell_op_regid=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), sellOrder.coin_symbol, dealCoinFee,
                    spSellOpAccount->regid.ToString()),
                    REJECT_INVALID, "transfer-deal-coin-fee-failed");
        }

        // 12.2. transfer deal asset fee from buyer to buy operator
        if (!spBuyOrderAccount->OperateBalance(buyOrder.asset_symbol, SUB_FREE, dealAssetFee,
                                              ReceiptType::DEX_OPERATOR_REG_UPDATE_FEE_FROM_OPERATOR, receipts,
                                              spBuyOpAccount.get())) {
            return state.DoS(100, ERRORMSG("%s, transfer deal asset fee from buyer to buy operator failed!"
                " deal_info={%s}, asset_symbol=%s, asset_fee=%llu, buy_match_regid=%s",
                DEAL_ITEM_TITLE, dealItem.ToString(), buyOrder.asset_symbol, dealAssetFee, spBuyOpAccount->regid.ToString()),
                REJECT_INVALID, "transfer-deal-asset-fee-failed");
        }

        // 13. process WUSD friction fee payed for risk-reserve
        if (version >= FeatureForkVersionEnum::MAJOR_VER_R3) {
            if (sellOrder.asset_symbol == SYMB::WUSD) {
                if (!ProcessWusdFrictionFee(*spSellOrderAccount, dealItem.dealCoinAmount, frictionCoinFee)) return false;
            } else if (buyOrder.coin_symbol == SYMB::WUSD) {
                if (!ProcessWusdFrictionFee(*spBuyOrderAccount, dealItem.dealAssetAmount, frictionAssetFee)) return false;
            }
        }

        // 14. check order fullfiled or save residual amount
        if (buyResidualAmount == 0) { // buy order fulfilled
            if (buyOrder.order_type == ORDER_LIMIT_PRICE) {
                if (buyOrder.coin_amount > buyOrder.total_deal_coin_amount) {
                    uint64_t residualCoinAmount = buyOrder.coin_amount - buyOrder.total_deal_coin_amount;
                    if (!spBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, UNFREEZE, residualCoinAmount,
                                                        ReceiptType::DEX_UNFREEZE_COIN_TO_BUYER, receipts)) {
                        return state.DoS(100, ERRORMSG("%s, unfreeze buyer's residual coins failed!"
                            " deal_info={%s}, coin_symbol=%s, residual_coins=%llu",
                            DEAL_ITEM_TITLE, dealItem.ToString(), buyOrder.coin_symbol, residualCoinAmount),
                            REJECT_INVALID, "operate-account-failed");
                    }
                } else {
                    assert(buyOrder.coin_amount == buyOrder.total_deal_coin_amount);
                }
            }
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.buyOrderId, buyOrder)) {
                return state.DoS(100, ERRORMSG("%s, finish the active buy order failed! deal_info={%s}",
                    DEAL_ITEM_TITLE, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.UpdateActiveOrder(dealItem.buyOrderId, buyOrder)) {
                return state.DoS(100, ERRORMSG("%s, update active buy order failed! deal_info={%s}",
                    DEAL_ITEM_TITLE, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (sellResidualAmount == 0) { // sell order fulfilled
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.sellOrderId, sellOrder)) {
                return state.DoS(100, ERRORMSG("%s, finish active sell order failed! deal_info={%s}",
                    DEAL_ITEM_TITLE, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.UpdateActiveOrder(dealItem.sellOrderId, sellOrder)) {
                return state.DoS(100, ERRORMSG("%s, update active sell order failed! deal_info={%s}",
                    DEAL_ITEM_TITLE, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        }
        return true;
    }

    COrderOperatorParams CDealItemExecuter::GetOrderOperatorParams(CDEXOrderDetail &order,
            DexOperatorDetail &operatorDetail) {
        if (order.opt_operator_params) {
            return order.opt_operator_params.value();
        } else {
            return {
                operatorDetail.order_open_mode,
                operatorDetail.maker_fee_ratio,
                operatorDetail.taker_fee_ratio
            };
        }
    }

    bool CDealItemExecuter::CheckOrderOpenMode() {

        uint32_t buyDexId = buyOrder.dex_id;
        uint32_t sellDexId = sellOrder.dex_id;
        OpenMode buyOrderOpenMode = buyOrderOperatorParams.open_mode;
        OpenMode sellOrderOpenMode = sellOrderOperatorParams.open_mode;
        OrderSide makerSide = takerSide == ORDER_BUY ? ORDER_SELL : ORDER_BUY;

        if (buyDexId != sellDexId) {
            if (makerSide == ORDER_BUY) {
                if (buyOrderOpenMode != OpenMode::PUBLIC)
                    return context.pState->DoS(100, ERRORMSG("%s, the buy maker order not public! "
                        "buy_dex_id=%u, sell_dex_id=%u", DEAL_ITEM_TITLE, buyDexId, sellDexId),
                        REJECT_INVALID, "buy-maker-order-not-public");
                const auto &orderOpenDexopSet = buyOperatorDetail.order_open_dexop_set;
                if (!orderOpenDexopSet.empty() && orderOpenDexopSet.count(sellDexId) == 0)
                    return context.pState->DoS(100, ERRORMSG("%s, the buy maker order operator=%llu not public"
                        " to the sell operator=%llu! buy_dex_id=%u, sell_dex_id=%u",
                        DEAL_ITEM_TITLE, buyDexId, sellDexId, buyDexId, sellDexId),
                        REJECT_INVALID, "buy-order-operator-public-limit");
            } else {
                assert(makerSide == ORDER_SELL);
                if (sellOrderOpenMode != OpenMode::PUBLIC)
                    return context.pState->DoS(100, ERRORMSG("%s, the sell maker order not public! "
                        "buy_dex_id=%u, sell_dex_id=%u", DEAL_ITEM_TITLE, buyDexId, sellDexId),
                        REJECT_INVALID, "sell-maker-order-not-public");
                const auto &orderOpenDexopSet = sellOperatorDetail.order_open_dexop_set;
                if (!orderOpenDexopSet.empty() && orderOpenDexopSet.count(buyDexId) == 0)
                    return context.pState->DoS(100, ERRORMSG("%s, the sell maker order operator=%llu not public"
                        " to the buy operator=%llu! buy_dex_id=%u, sell_dex_id=%u",
                        DEAL_ITEM_TITLE, sellDexId, buyDexId, buyDexId, sellDexId),
                        REJECT_INVALID, "sell-order-operator-public-limit");
            }
        }
        return true;
    }

    bool CDealItemExecuter::CheckTotalTradingFees(uint64_t dealAmount, uint64_t frictionFee,
                                                  uint64_t dealFee, const char *name) {
        if (dealAmount < frictionFee + dealFee) {
            return context.pState->DoS(100, ERRORMSG("%s, total %s fees of dealItem is too large! deal_info={%s}, "
                    "frictionCoinFee=%llu, dealCoinFee=%llu",
                    DEAL_ITEM_TITLE, name, dealItem.ToString(), frictionFee, dealFee),
                    REJECT_INVALID, "deal-buyer-coins-failed");
        }
        return true;
    }

    bool CDealItemExecuter::GetDealOrder(const uint256 &orderId, const OrderSide orderSide,
                                         CDEXOrderDetail &dealOrder) {
        if (!context.pCw->dexCache.GetActiveOrder(orderId, dealOrder))
            return context.pState->DoS(100, ERRORMSG("%s, get active order failed! orderId=%s", DEAL_ITEM_TITLE,
                orderId.ToString()), REJECT_INVALID,
                strprintf("get-active-order-failed, i=%d, order_id=%s", idx, orderId.ToString()));

        if (dealOrder.order_side != orderSide)
            return context.pState->DoS(100, ERRORMSG("%s, expected order_side=%s but got order_side=%s! orderId=%s",
                    DEAL_ITEM_TITLE, kOrderSideHelper.GetName(orderSide),
                    kOrderSideHelper.GetName(dealOrder.order_side), orderId.ToString()),
                    REJECT_INVALID,
                    strprintf("order-side-unmatched, i=%d, order_id=%s", idx, orderId.ToString()));

        return true;
    }

    OrderSide CDealItemExecuter::GetTakerOrderSide() {
        OrderSide takerSide;
        if (buyOrder.order_type != sellOrder.order_type) {
            if (buyOrder.order_type == ORDER_MARKET_PRICE) {
                takerSide = OrderSide::ORDER_BUY;
            } else {
                assert(sellOrder.order_type == ORDER_MARKET_PRICE);
                takerSide = OrderSide::ORDER_SELL;
            }
        } else { // buyOrder.order_type == sellOrder.order_type
            takerSide = (buyOrder.tx_cord < sellOrder.tx_cord) ? OrderSide::ORDER_SELL : OrderSide::ORDER_BUY;
        }
        return takerSide;
    }

    uint64_t CDealItemExecuter::GetOperatorFeeRatio(const CDEXOrderDetail &order,
                                                    const COrderOperatorParams &orderOperatorParams,
                                                    const OrderSide &takerSide) {
        uint64_t ratio = (order.order_side == takerSide) ? orderOperatorParams.taker_fee_ratio :
                        orderOperatorParams.maker_fee_ratio;

        LogPrint(BCLog::DEX, "got operator_fee_ratio=%llu, is_taker=%d, order_side=%d\n", ratio,
                order.order_side == takerSide, kOrderSideHelper.GetName(order.order_side));

        return ratio;
    }

    bool CDealItemExecuter::CalcDealFee(const uint256 &orderId, const CDEXOrderDetail &order,
                                         const COrderOperatorParams &orderOperatorParams,
                                         const OrderSide &takerSide, uint64_t amount,
                                         uint64_t &orderFee) {

        uint64_t feeRatio = GetOperatorFeeRatio(order, orderOperatorParams, takerSide);
        if (!CheckOperatorFeeRatioRange(context, orderId, feeRatio, DEAL_ITEM_TITLE))
            return false;
        if (!CalcAmountByRatio(amount, feeRatio, PRICE_BOOST, orderFee))
            return context.pState->DoS(100, ERRORMSG("%s, the calc_deal_fee overflow! amount=%llu, "
                "fee_ratio=%llu, side=%s", DEAL_ITEM_TITLE,  amount, feeRatio,
                kOrderSideHelper.GetName(order.order_side)),
                REJECT_INVALID, "calc-deal-fee-overflow");
        return true;
    }

    bool CDealItemExecuter::CalcWusdFrictionFee(uint64_t amount, uint64_t &frictionFee) {
            uint64_t frictionFeeRatio;
            if (!context.pCw->sysParamCache.GetParam(TRANSFER_SCOIN_FRICTION_FEE_RATIO, frictionFeeRatio))
                return context.pState->DoS(100, ERRORMSG("%s, read TRANSFER_SCOIN_FRICTION_FEE_RATIO error", DEAL_ITEM_TITLE),
                                READ_SYS_PARAM_FAIL, "bad-read-sysparamdb");
        if (!CalcAmountByRatio(amount, frictionFeeRatio, RATIO_BOOST, frictionFee))
            return context.pState->DoS(100, ERRORMSG("%s, the calc_friction_fee overflow! amount=%llu, "
                "fee_ratio=%llu", DEAL_ITEM_TITLE,  amount, frictionFeeRatio),
                REJECT_INVALID, "calc-friction-fee-overflow");
        return true;
    }

    bool CDealItemExecuter::ProcessWusdFrictionFee(CAccount &fromAccount, uint64_t amount, uint64_t frictionFee) {

        CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
        if (frictionFee > 0) {

            uint64_t reserveScoins = frictionFee / 2;
            uint64_t buyScoins  = frictionFee - reserveScoins;  // handle odd amount
            const auto &fcoinRegid = SysCfg().GetFcoinGenesisRegId();
            auto spFcoinAccount = tx.GetAccount(context, fcoinRegid, "fcoin");
            if (!spFcoinAccount) return false;

            // 1) transfer all risk fee to risk-reserve
            if (!fromAccount.OperateBalance(SYMB::WUSD, BalanceOpType::SUB_FREE, frictionFee,
                                                    ReceiptType::SOIN_FRICTION_FEE_TO_RESERVE, receipts, spFcoinAccount.get())) {
                return state.DoS(100, ERRORMSG("transfer risk fee to risk-reserve account failed"),
                                UPDATE_ACCOUNT_FAIL, "transfer-risk-fee-failed");
            }

            // 2) sell 50% risk fees and burn it
            // should freeze user's coin for buying the WGRT
            if (buyScoins > 0) {
                if (!spFcoinAccount->OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, buyScoins,
                                                        ReceiptType::BUY_FCOINS_FOR_DEFLATION, receipts)) {
                    return state.DoS(100, ERRORMSG("account has insufficient funds"),
                                    UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
                }
                CHashWriter hashWriter(SER_GETHASH, 0);
                hashWriter << tx.GetHash() << SYMB::WUSD << VARINT(idx);
                uint256 orderId = hashWriter.GetHash();
                auto pSysBuyMarketOrder = dex::CSysOrder::CreateBuyMarketOrder(context.GetTxCord(), SYMB::WUSD, SYMB::WGRT, buyScoins);
                if (!cw.dexCache.CreateActiveOrder(orderId, *pSysBuyMarketOrder)) {
                    return state.DoS(100, ERRORMSG("create system buy order failed, orderId=%s", orderId.ToString()),
                                    CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
                }
            }
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXSettleTx

    string CDEXSettleTx::DealItem::ToString() const {
        return  strprintf("buy_order_id=%s",    buyOrderId.ToString()) + ", " +
                strprintf("sell_order_id=%s",   sellOrderId.ToString()) + ", " +
                strprintf("price=%llu",         dealPrice) + ", " +
                strprintf("coin_amount=%llu",   dealCoinAmount) + ", " +
                strprintf("asset_amount=%llu",  dealAssetAmount);
    }

    string CDEXSettleTx::ToString(CAccountDBCache &accountCache) {
        string dealInfo="";
        for (const auto &item : dealItems) {
            dealInfo += "{" + item.ToString() + "},";
        }

        return strprintf("txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, "
                        "deal_items=[%s]",
                        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height,
                        txUid.ToString(), llFees, dealInfo);
    }

    Object CDEXSettleTx::ToJson(const CAccountDBCache &accountCache) const {
        Array arrayItems;
        for (const auto &item : dealItems) {
            Object subItem;
            subItem.push_back(Pair("buy_order_id",      item.buyOrderId.GetHex()));
            subItem.push_back(Pair("sell_order_id",     item.sellOrderId.GetHex()));
            subItem.push_back(Pair("coin_amount",       item.dealCoinAmount));
            subItem.push_back(Pair("asset_amount",      item.dealAssetAmount));
            subItem.push_back(Pair("price",             item.dealPrice));
            arrayItems.push_back(subItem);
        }

        Object result = CBaseTx::ToJson(accountCache);
        result.push_back(Pair("deal_items",  arrayItems));

        return result;
    }

    bool CDEXSettleTx::CheckTx(CTxExecuteContext &context) {
        IMPLEMENT_DEFINE_CW_STATE

        if (!CheckTxAvailableFromVer(context, MAJOR_VER_R2)) return false;

        CRegID matchRegId = cw.sysParamCache.GetDexMatchSvcRegId();
        if (txUid.get<CRegID>() != matchRegId ) {
            return state.DoS(100, ERRORMSG("%s, account regid=%s is not authorized dex match-svc regId=%s", TX_ERR_TITLE,
                            txUid.ToString(), matchRegId.ToString()), REJECT_INVALID, "unauthorized-settle-account");
        }

        if (dealItems.empty() || dealItems.size() > MAX_SETTLE_ITEM_COUNT)
            return state.DoS(100, ERRORMSG("%s, deal items is empty or count=%d is larger than %d",
                TX_ERR_TITLE, dealItems.size(), MAX_SETTLE_ITEM_COUNT),
                REJECT_INVALID, "invalid-deal-items");

        for (size_t i = 0; i < dealItems.size(); i++) {
            const DealItem &dealItem = dealItems.at(i);
            if (dealItem.buyOrderId.IsEmpty() || dealItem.sellOrderId.IsEmpty())
                return state.DoS(100, ERRORMSG("%s, deal_items[%d], buy_order_id or sell_order_id is empty", i),
                                REJECT_INVALID, "empty-order-id");

            if (dealItem.buyOrderId == dealItem.sellOrderId)
                return state.DoS(100, ERRORMSG("%s, deal_items[%d], buy_order_id cannot equal to sell_order_id", TX_ERR_TITLE, i),
                                REJECT_INVALID, "same_order_id");

            if (dealItem.dealCoinAmount == 0 || dealItem.dealAssetAmount == 0 || dealItem.dealPrice == 0)
                return state.DoS(100, ERRORMSG("%s, deal_items[%d], deal_coin_amount or deal_asset_amount or deal_price is zero", TX_ERR_TITLE, i),
                                REJECT_INVALID, "zero_order_amount");
        }

        return true;
    }

    bool CDEXSettleTx::ExecuteTx(CTxExecuteContext &context) {
        IMPLEMENT_DEFINE_CW_STATE;

        map<CRegID, std::shared_ptr<CAccount>> accountMap = {
            {sp_tx_account->regid, sp_tx_account}
        };

        for (size_t idx = 0; idx < dealItems.size(); idx++) {
            CDealItemExecuter dealItemExec(dealItems[idx], idx, *this, context, sp_tx_account, accountMap, receipts);
            if (!dealItemExec.Execute()) {
                return false;
            }
        }

        // save accounts, include tx account
        for (auto accountItem : accountMap) {
            auto pAccount = accountItem.second;
            if (!sp_tx_account->IsSelfUid(pAccount->regid)) {
                if (!cw.accountCache.SetAccount(pAccount->keyid, *pAccount))
                    return state.DoS(100, ERRORMSG("%s, set account info error! regid=%s, addr=%s",
                        TX_ERR_TITLE, pAccount->regid.ToString(), pAccount->keyid.ToAddress()),
                        WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }

        return true;
    }

} // namespace dex
