// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"

#include "config/configuration.h"
#include "main.h"

#include <algorithm>

using namespace dex;

namespace dex {

    #define ERROR_TITLE(msg) (std::string(__FUNCTION__) + "(), " + msg)
    #define TX_ERR_TITLE ERROR_TITLE(GetTxTypeName())

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
                            shared_ptr<DexOperatorDetail> &spOperatorDetail, const string &title) {
        spOperatorDetail = make_shared<DexOperatorDetail>();
        if (!context.pCw->dexCache.GetDexOperator(dexId, *spOperatorDetail))
            return context.pState->DoS(100, ERRORMSG("%s(), the dex operator does not exist! dex_id=%u", title, dexId),
                REJECT_INVALID, "dex_operator_not_existed");
        return true;
    }

    bool CheckOrderFee(CBaseTx &baseTx, CTxExecuteContext &context, const CAccount &txAccount) {

        return baseTx.CheckFee(context, [&](CTxExecuteContext &context, uint64_t minFee) -> bool {
            if (GetFeatureForkVersion(context.height) > MAJOR_VER_R3 && baseTx.txUid.is<CPubKey>()) {
                auto token = txAccount.GetToken(SYMB::WICC);

                if (token.staked_amount > 0) {
                    minFee = std::max(std::min(COIN * COIN / token.staked_amount, minFee), (uint64_t)1);
                }
                if (baseTx.llFees < minFee){
                    string err = strprintf("The given fee is too small: %llu < %llu sawi when wicc staked_amount=%llu",
                        baseTx.llFees, minFee, token.staked_amount);
                    return context.pState->DoS(100, ERRORMSG("%s, tx=%s, height=%d, fee_symbol=%s",
                        err, baseTx.GetTxTypeName(), context.height, baseTx.fee_symbol), REJECT_INVALID, err);
                }
                return true;
            } else {
                return baseTx.CheckMinFee(context, minFee);
            }
        });
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXOrderBaseTx

    bool CDEXOrderBaseTx::CheckTx(CTxExecuteContext &context) {
        IMPLEMENT_DEFINE_CW_STATE;
        IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
        IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid);
        // TODO: ...
        //IMPLEMENT_CHECK_TX_MEMO;
        if (!kOrderTypeHelper.CheckEnum(order_type))
            return context.pState->DoS(100, ERRORMSG("%s, invalid order_type=%u", TX_ERR_TITLE,
                    order_type), REJECT_INVALID, "invalid-order-side");

        if (!kOrderSideHelper.CheckEnum(order_side))
            return context.pState->DoS(100, ERRORMSG("%s, invalid order_side=%u", TX_ERR_TITLE,
                    order_side), REJECT_INVALID, "invalid-order-side");

        if (!CheckOrderSymbols(context, coin_symbol, asset_symbol)) return false;

        if (!CheckOrderAmounts(context)) return false;

        if (!CheckOrderPrice(context)) return false;

        if (!kPublicModeHelper.CheckEnum(public_mode))
            return context.pState->DoS(100, ERRORMSG("%s, invalid public_mode=%s", TX_ERR_TITLE,
                    kPublicModeHelper.GetName(public_mode)), REJECT_INVALID, "invalid-public-mode");
        IMPLEMENT_CHECK_TX_MEMO;

        if (!CheckOrderOperator(context)) return false;

        CAccount txAccount;
        if (!cw.accountCache.GetAccount(txUid, txAccount))
            return state.DoS(100, ERRORMSG("%s, read account failed", ERROR_TITLE(GetTxTypeName())),
                REJECT_INVALID, "bad-getaccount");

        CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
        IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

        return true;
    }


    bool CDEXOrderBaseTx::ExecuteTx(CTxExecuteContext &context) {
        CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
        CAccount txAccount;
        if (!cw.accountCache.GetAccount(txUid, txAccount)) {
            return state.DoS(100, ERRORMSG("%s, read source addr account info error", ERROR_TITLE(GetTxTypeName())),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!GenerateRegID(context, txAccount)) {
            return false;
        }

        if (!txAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
            return state.DoS(100, ERRORMSG("%s, account has insufficient funds", ERROR_TITLE(GetTxTypeName())),
                            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
        }


        uint64_t coinAmount = coin_amount;
        if (order_type == ORDER_LIMIT_PRICE && order_side == ORDER_BUY)
            coinAmount = CDEXOrderBaseTx::CalcCoinAmount(asset_amount, price);

        if (order_side == ORDER_BUY) {
            if (!FreezeBalance(context, txAccount, coin_symbol, coinAmount)) return false;
        } else {
            assert(order_side == ORDER_SELL);
            if (!FreezeBalance(context, txAccount, asset_symbol, asset_amount)) return false;
        }

        assert(!txAccount.regid.IsEmpty());
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
        orderDetail.public_mode        = public_mode;
        orderDetail.dex_id             = dex_id;
        orderDetail.tx_cord            = CTxCord(context.height, context.index);
        orderDetail.user_regid         = txAccount.regid;
        if (has_operator_config)
            orderDetail.opt_operator_fee_ratios  = operator_fee_ratios;
        // other fields keep the default value

        if (!context.pCw->dexCache.CreateActiveOrder(txid, orderDetail))
            return context.pState->DoS(100, ERRORMSG("%s, create active buy order failed! txid=%s",
                TX_ERR_TITLE, txid.ToString()), REJECT_INVALID, "bad-write-dexdb");

        if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
            return state.DoS(100, ERRORMSG("%s, set account info error", ERROR_TITLE(GetTxTypeName())),
                            WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
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
                strprintf("operator_taker_fee_ratio=%llu", operator_fee_ratios.taker_fee_ratio) + ", " +
                strprintf("operator_maker_fee_ratio=%llu", operator_fee_ratios.maker_fee_ratio) + ", " +
                strprintf("operator_uid=%s", operator_uid.ToDebugString()) + ", " +
                strprintf("memo_hex=%s", nVersion) + ", " +
                strprintf("operator_signature=%s", HexStr(operator_signature));
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
        result.push_back(Pair("memo",               memo));
        result.push_back(Pair("memo_hex",           HexStr(memo)));
        if (has_operator_config) {
            CKeyID operatorKeyid;
            accountCache.GetKeyId(operator_uid, operatorKeyid);
            result.push_back(Pair("taker_fee_ratio",    operator_fee_ratios.taker_fee_ratio));
            result.push_back(Pair("maker_fee_ratio",    operator_fee_ratios.maker_fee_ratio));
            result.push_back(Pair("operator_uid",       operator_uid.ToDebugString()));
            result.push_back(Pair("operator_addr",      operatorKeyid.ToAddress()));
            result.push_back(Pair("operator_signature", HexStr(operator_signature)));
        }
        return result;
    }

    bool CDEXOrderBaseTx::CheckOrderSymbols(CTxExecuteContext &context, const TokenSymbol &coinSymbol,
                                            const TokenSymbol &assetSymbol) {

        if (coinSymbol.empty() || coinSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(coinSymbol) == 0) {
            return context.pState->DoS(100, ERRORMSG("%s, invalid order coin symbol=%s", TX_ERR_TITLE, coinSymbol),
                            REJECT_INVALID, "invalid-order-coin-symbol");
        }

        if (assetSymbol.empty() || assetSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(assetSymbol) == 0) {
            return context.pState->DoS(100, ERRORMSG("%s, invalid order asset symbol=%s", TX_ERR_TITLE, assetSymbol),
                            REJECT_INVALID, "invalid-order-asset-symbol");
        }

        //TODO: check if asset has perms for DEX
        // if (kTradingPairSet.count(make_pair(assetSymbol, coinSymbol)) == 0) {
        //     return context.pState->DoS(100, ERRORMSG("%s, not support the trading pair! coin_symbol=%s, asset_symbol=%s",
        //         TX_ERR_TITLE, coinSymbol, assetSymbol), REJECT_INVALID, "invalid-trading-pair");
        // }

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

    bool CDEXOrderBaseTx::CheckDexOperatorExist(CTxExecuteContext &context) {
        if (dex_id != DEX_RESERVED_ID) {
            if (!context.pCw->dexCache.HaveDexOperator(dex_id))
                return context.pState->DoS(100, ERRORMSG("%s, dex operator does not exist! dex_id=%d",
                    TX_ERR_TITLE, dex_id),
                    REJECT_INVALID, "bad-getaccount");
        }
        return true;
    }


    bool CDEXOrderBaseTx::CheckOrderOperator(CTxExecuteContext &context) {

        if (!CheckDexOperatorExist(context)) return false;

        if (has_operator_config) {
            const auto &hash = GetHash();
            if (!CheckOperatorFeeRatioRange(context, hash, operator_fee_ratios.taker_fee_ratio,
                    TX_ERR_TITLE + ", taker_fee_ratio"))
                return false;
            if (!CheckOperatorFeeRatioRange(context, hash, operator_fee_ratios.maker_fee_ratio,
                    TX_ERR_TITLE + ", maker_fee_ratio"))
                return false;

            if (!operator_uid.is<CRegID>())
                return context.pState->DoS(100, ERRORMSG("%s(), dex operator uid must be regid, operator_uid=%s",
                    TX_ERR_TITLE, operator_uid.ToDebugString()),
                    REJECT_INVALID, "operator-uid-not-regid");

            shared_ptr<DexOperatorDetail> spOperatorDetail;
            if(!GetDexOperator(context, dex_id, spOperatorDetail, TX_ERR_TITLE)) return false;

            const CRegID &operator_regid = operator_uid.get<CRegID>();
            if (operator_regid != spOperatorDetail->fee_receiver_regid)
                return context.pState->DoS(100, ERRORMSG("%s(), the dex operator uid is wrong, operator_uid=%s",
                    TX_ERR_TITLE, operator_uid.ToDebugString()),
                    REJECT_INVALID, "operator-uid-wrong");

            CAccount operatorAccount;
            if (!context.pCw->accountCache.GetAccount(operator_regid, operatorAccount))
                return context.pState->DoS(100, ERRORMSG("%s, operator account not existed! operator_regid=%s",
                    TX_ERR_TITLE, operator_uid.ToDebugString()),
                    REJECT_INVALID, "operator-account-not-existed");

            if (!operatorAccount.IsRegistered())
                return context.pState->DoS(100, ERRORMSG("%s, operator account must be registered! "
                    "operator_regid=%s", TX_ERR_TITLE, operator_uid.ToDebugString()),
                    REJECT_INVALID, "operator-account-unregistered");

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

    bool CDEXOrderBaseTx::FreezeBalance(CTxExecuteContext &context, CAccount &account, const TokenSymbol &tokenSymbol, const uint64_t &amount) {

        if (!account.OperateBalance(tokenSymbol, FREEZE, amount)) {
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

        // TODO: disable in v3
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXSellLimitOrderTx

    bool CDEXSellLimitOrderTx::CheckTx(CTxExecuteContext &context) {

        // TODO: disable in v3
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXBuyMarketOrderTx

    bool CDEXBuyMarketOrderTx::CheckTx(CTxExecuteContext &context) {
        // TODO: disable in v3
        return CDEXOrderBaseTx::CheckTx(context);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXSellMarketOrderTx

    bool CDEXSellMarketOrderTx::CheckTx(CTxExecuteContext &context) {
        // TODO: disable in v3
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
        IMPLEMENT_DEFINE_CW_STATE;
        IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
        IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid);

        if (order_id.IsEmpty())
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, order_id is empty"), REJECT_INVALID,
                            "invalid-order-id");
        CAccount txAccount;
        if (!cw.accountCache.GetAccount(txUid, txAccount))
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                            "bad-getaccount");

        if (!CheckOrderFee(*this, context, txAccount)) return false;
        CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
        IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

        return true;
    }

    bool CDEXCancelOrderTx::ExecuteTx(CTxExecuteContext &context) {
        CCacheWrapper &cw       = *context.pCw;
        CValidationState &state = *context.pState;

        CAccount txAccount;
        if (!cw.accountCache.GetAccount(txUid, txAccount)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, read source addr account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!GenerateRegID(context, txAccount)) {
            return false;
        }

        if (!txAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, account has insufficient funds"),
                            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
        }

        CDEXOrderDetail activeOrder;
        if (!cw.dexCache.GetActiveOrder(order_id, activeOrder)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, the order is inactive or not existed"),
                            REJECT_INVALID, "order-inactive");
        }

        if (activeOrder.generate_type != USER_GEN_ORDER) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, the order is not generate by tx of user"),
                            REJECT_INVALID, "order-inactive");
        }

        if (txAccount.regid.IsEmpty() || txAccount.regid != activeOrder.user_regid) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, can not cancel other user's order tx"),
                            REJECT_INVALID, "user-unmatched");
        }

        // get frozen money
        vector<CReceipt> receipts;
        TokenSymbol frozenSymbol;
        uint64_t frozenAmount = 0;
        if (activeOrder.order_side == ORDER_BUY) {
            frozenSymbol = activeOrder.coin_symbol;
            frozenAmount = activeOrder.coin_amount - activeOrder.total_deal_coin_amount;

            receipts.emplace_back(CUserID::NULL_ID, activeOrder.user_regid, frozenSymbol, frozenAmount,
                                ReceiptCode::DEX_UNFREEZE_COIN_TO_BUYER);
        } else if(activeOrder.order_side == ORDER_SELL) {
            frozenSymbol = activeOrder.asset_symbol;
            frozenAmount = activeOrder.asset_amount - activeOrder.total_deal_asset_amount;

            receipts.emplace_back(CUserID::NULL_ID, activeOrder.user_regid, frozenSymbol, frozenAmount,
                                ReceiptCode::DEX_UNFREEZE_ASSET_TO_SELLER);
        } else {
            assert(false && "Order side must be ORDER_BUY|ORDER_SELL");
        }

        if (!txAccount.OperateBalance(frozenSymbol, UNFREEZE, frozenAmount)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, account has insufficient frozen amount to unfreeze"),
                            UPDATE_ACCOUNT_FAIL, "unfreeze-account-coin");
        }

        if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, set account info error"),
                            WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

        if (!cw.dexCache.EraseActiveOrder(order_id, activeOrder)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, erase active order failed! order_id=%s", order_id.ToString()),
                            REJECT_INVALID, "order-erase-failed");
        }

        if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, write tx receipt failed! txid=%s", GetHash().ToString()),
                            REJECT_INVALID, "write-tx-receipt-failed");

        return true;
    }

    #define DEAL_ITEM_TITLE ERROR_TITLE(tx.GetTxTypeName() + strprintf(", i[%d]", i))

    class CDealItemExecuter {
    public:
        typedef CDEXSettleTx::DealItem DealItem;
    public:
        // input data
        DealItem &dealItem;
        uint32_t i;       // index of deal item
        CDEXSettleTx &tx;
        CTxExecuteContext &context;
        shared_ptr<CAccount> &pTxAccount;
        map<CRegID, shared_ptr<CAccount>> &accountMap;
        vector<CReceipt> &receipts;

        // found data
        CDEXOrderDetail buyOrder;
        CDEXOrderDetail sellOrder;
        shared_ptr<CAccount> pBuyOrderAccount;
        shared_ptr<CAccount> pSellOrderAccount;
        shared_ptr<DexOperatorDetail> pBuyOperatorDetail;
        shared_ptr<DexOperatorDetail> pSellOperatorDetail;
        shared_ptr<CAccount> pBuyMatchAccount;
        shared_ptr<CAccount> pSellMatchAccount;
        OrderSide takerSide;

        CDealItemExecuter(DealItem &dealItemIn, uint32_t index, CDEXSettleTx &txIn,
                          CTxExecuteContext &contextIn, shared_ptr<CAccount> &pTxAccountIn,
                          map<CRegID, shared_ptr<CAccount>> &accountMapIn,
                          vector<CReceipt> &receiptsIn)
            : dealItem(dealItemIn), i(index), tx(txIn), context(contextIn),
              pTxAccount(pTxAccountIn), accountMap(accountMapIn), receipts(receiptsIn) {}

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
        bool Execute() {
            CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

                //1.1 get and check buyDealOrder and sellDealOrder
            if (!GetDealOrder(dealItem.buyOrderId, ORDER_BUY, buyOrder)) return false;

            if (!GetDealOrder(dealItem.sellOrderId, ORDER_SELL, sellOrder)) return false;

            // 1.2 get account of order
            if (!GetAccount(buyOrder.user_regid, pBuyOrderAccount)) return false;
            if (!GetAccount(sellOrder.user_regid, pSellOrderAccount)) return false;

            // 1.3 get operator info
            if (!GetDexOperator(context, buyOrder.dex_id, pBuyOperatorDetail, DEAL_ITEM_TITLE)) return false;
            if (!GetAccount(pBuyOperatorDetail->fee_receiver_regid, pBuyMatchAccount)) return false;

            if (!GetDexOperator(context, sellOrder.dex_id, pSellOperatorDetail, DEAL_ITEM_TITLE)) return false;
            if (!GetAccount(pSellOperatorDetail->fee_receiver_regid, pSellMatchAccount)) return false;

            // 1.4 get taker side
            takerSide = GetTakerOrderSide();

            // 3. check coin type match
            if (buyOrder.coin_symbol != sellOrder.coin_symbol) {
                return state.DoS(100, ERRORMSG("%s, coin symbol unmatch! buyer coin_symbol=%s, " \
                    "seller coin_symbol=%s", DEAL_ITEM_TITLE, buyOrder.coin_symbol, sellOrder.coin_symbol),
                    REJECT_INVALID, "coin-symbol-unmatch");
            }
            // 4. check asset type match
            if (buyOrder.asset_symbol != sellOrder.asset_symbol) {
                return state.DoS(100, ERRORMSG("%s, asset symbol unmatch! buyer asset_symbol=%s, " \
                    "seller asset_symbol=%s", DEAL_ITEM_TITLE, buyOrder.asset_symbol, sellOrder.asset_symbol),
                    REJECT_INVALID, "asset-symbol-unmatch");
            }

            // 5. check price match
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

            // TODO: check operator fee ratio
            // 2. check dex_id
            if (!CheckDexOperator()) return false;
            // TODO: check taker_side

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

            // 8. subtract the buyer's coins and seller's assets
            // - unfree and subtract the coins from buyer account
            if (   !pBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, UNFREEZE, dealItem.dealCoinAmount)
                || !pBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, SUB_FREE, dealItem.dealCoinAmount)) {// - subtract buyer's coins
                return state.DoS(100, ERRORMSG("%s, subtract coins from buyer account failed!"
                    " deal_info={%s}, coin_symbol=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), buyOrder.coin_symbol),
                    REJECT_INVALID, "operate-account-failed");
            }
            // - unfree and subtract the assets from seller account
            if (   !pSellOrderAccount->OperateBalance(sellOrder.asset_symbol, UNFREEZE, dealItem.dealAssetAmount)
                || !pSellOrderAccount->OperateBalance(sellOrder.asset_symbol, SUB_FREE, dealItem.dealAssetAmount)) { // - subtract seller's assets
                return state.DoS(100, ERRORMSG("%s, subtract assets from seller account failed!"
                    " deal_info={%s}, asset_symbol=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), sellOrder.asset_symbol),
                    REJECT_INVALID, "operate-account-failed");
            }

            // 9. calc deal dex operator fees
            uint64_t buyerReceivedAssets = dealItem.dealAssetAmount;
            // 9.1 buyer pay the fee from the received assets to settler

            uint64_t buyOperatorFeeRatio = GetOperatorFeeRatio(buyOrder, *pBuyOperatorDetail, takerSide);
            uint64_t sellOperatorFeeRatio = GetOperatorFeeRatio(sellOrder, *pSellOperatorDetail, takerSide);

            if (!CheckOperatorFeeRatioRange(context, dealItem.buyOrderId, buyOperatorFeeRatio, DEAL_ITEM_TITLE))
                return false;

            uint64_t dealAssetFee;
            if (!CalcOrderFee(dealItem.dealAssetAmount, buyOperatorFeeRatio, dealAssetFee)) return false;

            buyerReceivedAssets = dealItem.dealAssetAmount - dealAssetFee;
            // pay asset fee from seller to settler
            if (!pBuyMatchAccount->OperateBalance(buyOrder.asset_symbol, ADD_FREE, dealAssetFee)) {
                return state.DoS(100, ERRORMSG("%s, pay asset fee from buyer to operator account failed!"
                    " deal_info={%s}, asset_symbol=%s, asset_fee=%llu, buy_match_regid=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), buyOrder.asset_symbol, dealAssetFee, pBuyMatchAccount->regid.ToString()),
                    REJECT_INVALID, "operate-account-failed");
            }

            receipts.emplace_back(pBuyOrderAccount->regid, pBuyMatchAccount->regid, buyOrder.asset_symbol,
                            dealAssetFee, ReceiptCode::DEX_ASSET_FEE_TO_SETTLER);

            // 9.2 seller pay the fee from the received coins to settler
            uint64_t sellerReceivedCoins = dealItem.dealCoinAmount;
            if (!CheckOperatorFeeRatioRange(context, dealItem.sellOrderId, sellOperatorFeeRatio, DEAL_ITEM_TITLE))
                return false;
            uint64_t dealCoinFee;
            if (!CalcOrderFee(dealItem.dealCoinAmount, sellOperatorFeeRatio, dealCoinFee)) return false;

            sellerReceivedCoins = dealItem.dealCoinAmount - dealCoinFee;
            // pay coin fee from buyer to settler
            if (!pTxAccount->OperateBalance(sellOrder.coin_symbol, ADD_FREE, dealCoinFee)) {
                return state.DoS(100, ERRORMSG("%s, pay coin fee from seller to operator account failed!"
                    " deal_info={%s}, coin_symbol=%s, coin_fee=%llu, sell_match_regid=%s",
                    DEAL_ITEM_TITLE, dealItem.ToString(), sellOrder.coin_symbol, dealCoinFee,
                    pSellMatchAccount->regid.ToString()),
                    REJECT_INVALID, "operate-account-failed");
            }
            receipts.emplace_back(pSellOrderAccount->regid, pTxAccount->regid, sellOrder.coin_symbol,
                                dealCoinFee, ReceiptCode::DEX_COIN_FEE_TO_SETTLER);


            // 10. add the buyer's assets and seller's coins
            if (   !pBuyOrderAccount->OperateBalance(buyOrder.asset_symbol, ADD_FREE, buyerReceivedAssets)    // + add buyer's assets
                || !pSellOrderAccount->OperateBalance(sellOrder.coin_symbol, ADD_FREE, sellerReceivedCoins)){ // + add seller's coin
                return state.DoS(100,ERRORMSG("%s, add assets to buyer or add coins to seller failed!"
                    " deal_info={%s}, asset_symbol=%s, assets=%llu, coin_symbol=%s, coins=%llu",
                    DEAL_ITEM_TITLE, dealItem.ToString(), buyOrder.asset_symbol,
                    buyerReceivedAssets, sellOrder.coin_symbol, sellerReceivedCoins),
                    REJECT_INVALID, "operate-account-failed");
            }
            receipts.emplace_back(pSellOrderAccount->regid, pBuyOrderAccount->regid, buyOrder.asset_symbol,
                                buyerReceivedAssets, ReceiptCode::DEX_ASSET_TO_BUYER);
            receipts.emplace_back(pBuyOrderAccount->regid, pSellOrderAccount->regid, buyOrder.coin_symbol,
                                sellerReceivedCoins, ReceiptCode::DEX_COIN_TO_SELLER);

            // 11. check order fullfiled or save residual amount
            if (buyResidualAmount == 0) { // buy order fulfilled
                if (buyOrder.order_type == ORDER_LIMIT_PRICE) {
                    if (buyOrder.coin_amount > buyOrder.total_deal_coin_amount) {
                        uint64_t residualCoinAmount = buyOrder.coin_amount - buyOrder.total_deal_coin_amount;

                        if (!pBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, UNFREEZE, residualCoinAmount)) {
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

        bool CheckDexOperator() {

            uint32_t buyDexId = buyOrder.dex_id;
            uint32_t sellDexId = sellOrder.dex_id;
            OrderSide makerSide = takerSide == ORDER_BUY ? ORDER_SELL : ORDER_BUY;
            if (buyDexId != sellDexId) {
                if (makerSide == ORDER_BUY && buyOrder.public_mode != ORDER_PUBLIC) {
                    return context.pState->DoS(100, ERRORMSG("%s, the buy order is maker order and must be public! "
                        "buy_dex_id=%u, sell_dex_id=%u", DEAL_ITEM_TITLE, buyDexId, sellDexId),
                        REJECT_INVALID, "buy-maker-order-not-public");
                }
                if (makerSide == ORDER_SELL && buyOrder.public_mode != ORDER_PUBLIC) {
                    return context.pState->DoS(100, ERRORMSG("%s, the sell order is maker order and must be public! "
                        "buy_dex_id=%u, sell_dex_id=%u", DEAL_ITEM_TITLE, buyDexId, sellDexId),
                        REJECT_INVALID, "sell-maker-order-not-public");
                }
            }
            return true;
        }

        bool GetDealOrder(const uint256 &orderId, const OrderSide orderSide,
                          CDEXOrderDetail &dealOrder) {
            if (!context.pCw->dexCache.GetActiveOrder(orderId, dealOrder))
                return context.pState->DoS(100, ERRORMSG("%s, get active order failed! i=%d, orderId=%s", DEAL_ITEM_TITLE,
                    orderId.ToString()), REJECT_INVALID,
                    strprintf("get-active-order-failed, i=%d, order_id=%s", i, orderId.ToString()));

            if (dealOrder.order_side != orderSide)
                return context.pState->DoS(100, ERRORMSG("%s, expected order_side=%s but got order_side=%s! orderId=%s",
                        DEAL_ITEM_TITLE, kOrderSideHelper.GetName(orderSide),
                        kOrderSideHelper.GetName(dealOrder.order_side), orderId.ToString()),
                        REJECT_INVALID,
                        strprintf("order-side-unmatched, i=%d, order_id=%s", i, orderId.ToString()));

            return true;
        }

        OrderSide GetTakerOrderSide() {
            OrderSide takerSide;
            if (buyOrder.order_type != sellOrder.order_type) {
                if (buyOrder.order_type == ORDER_MARKET_PRICE) {
                    takerSide = OrderSide::ORDER_BUY;
                } else {
                    assert(sellOrder.order_type == ORDER_MARKET_PRICE);
                    takerSide = OrderSide::ORDER_SELL;
                }
            } else { // buyOrder.order_type == sellOrder.order_type
                if (buyOrder.tx_cord < sellOrder.tx_cord) {
                    takerSide = OrderSide::ORDER_BUY;
                } else {
                    takerSide = OrderSide::ORDER_SELL;
                }
            }
            return takerSide;
        }

        uint64_t GetOperatorFeeRatio(const CDEXOrderDetail &order,
                                     const DexOperatorDetail &operatorDetail,
                                     const OrderSide &takerSide) {
            uint64_t ratio;
            OperatorFeeRatios feeRatios;
            if (order.opt_operator_fee_ratios) {
                feeRatios = order.opt_operator_fee_ratios.value();
            } else {
                feeRatios.taker_fee_ratio = operatorDetail.taker_fee_ratio;
                feeRatios.taker_fee_ratio = operatorDetail.maker_fee_ratio;
            }

            if (order.order_side == takerSide) {
                ratio = feeRatios.taker_fee_ratio;
            } else {
                ratio = feeRatios.maker_fee_ratio;
            }
            LogPrint(BCLog::DEX, "got operator_fee_ratio=%llu, is_taker=%d, order_side=%d",
                ratio, order.order_side == takerSide, kOrderSideHelper.GetName(order.order_side));
            return ratio;
        }

        bool GetAccount(const CRegID &regid, shared_ptr<CAccount> &pAccount) {

            auto accountIt = accountMap.find(regid);
            if (accountIt != accountMap.end()) {
                pAccount = accountIt->second;
            } else {
                pAccount = make_shared<CAccount>();
                if (!context.pCw->accountCache.GetAccount(regid, *pAccount)) {
                    return context.pState->DoS(100, ERRORMSG("%s, read account info error! regid=%s",
                        DEAL_ITEM_TITLE, regid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
                }
                accountMap[regid] = pAccount;
            }
            return true;
        }

        bool CalcOrderFee(uint64_t amount, uint64_t fee_ratio, uint64_t &orderFee) {

            uint128_t fee = amount * (uint128_t)fee_ratio / PRICE_BOOST;
            if (fee > (uint128_t)ULLONG_MAX)
                return context.pState->DoS(100, ERRORMSG("%s, the calc_order_fee out of range! amount=%llu, "
                    "fee_ratio=%llu", DEAL_ITEM_TITLE,  amount, fee_ratio), REJECT_INVALID, "calc-order-fee-error");
            orderFee = fee;
            return true;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // class CDEXSettleTx

    string CDEXSettleTx::DealItem::ToString() const {
        return  strprintf("buy_order_id=%s", buyOrderId.ToString()) + ", " +
                strprintf("sell_order_id=%s", sellOrderId.ToString()) + ", " +
                strprintf("price=%llu", dealPrice) + ", " +
                strprintf("coin_amount=%llu", dealCoinAmount) + ", " +
                strprintf("asset_amount=%llu", dealAssetAmount);
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
        IMPLEMENT_DEFINE_CW_STATE;
        IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
        IMPLEMENT_CHECK_TX_REGID(txUid);
        if (!CheckFee(context)) return false;
        if (txUid.get<CRegID>() != SysCfg().GetDexMatchSvcRegId()) {
            return state.DoS(100, ERRORMSG("%s, account regid=%s is not authorized dex match-svc regId=%s",
                TX_ERR_TITLE, txUid.ToString(), SysCfg().GetDexMatchSvcRegId().ToString()),
                REJECT_INVALID, "unauthorized-settle-account");
        }

        if (dealItems.empty() || dealItems.size() > MAX_SETTLE_ITEM_COUNT)
            return state.DoS(100, ERRORMSG("%s, deal items is empty or count=%d is larger than %d",
                TX_ERR_TITLE, dealItems.size(), MAX_SETTLE_ITEM_COUNT),
                REJECT_INVALID, "invalid-deal-items");

        for (size_t i = 0; i < dealItems.size(); i++) {
            const DealItem &dealItem = dealItems.at(i);
            if (dealItem.buyOrderId.IsEmpty() || dealItem.sellOrderId.IsEmpty())
                return state.DoS(100, ERRORMSG("%s, deal_items[%d], buy_order_id or sell_order_id is empty",
                    i), REJECT_INVALID, "empty-order-id");
            if (dealItem.buyOrderId == dealItem.sellOrderId)
                return state.DoS(100, ERRORMSG("%s, deal_items[%d], buy_order_id cannot equal to sell_order_id",
                    TX_ERR_TITLE, i), REJECT_INVALID, "same_order_id");
            if (dealItem.dealCoinAmount == 0 || dealItem.dealAssetAmount == 0 || dealItem.dealPrice == 0)
                return state.DoS(100, ERRORMSG("%s, deal_items[%d], deal_coin_amount or deal_asset_amount or deal_price is zero",
                    TX_ERR_TITLE, i), REJECT_INVALID, "zero_order_amount");
        }

        CAccount txAccount;
        if (!cw.accountCache.GetAccount(txUid, txAccount))
            return state.DoS(100, ERRORMSG("%s, read account failed", TX_ERR_TITLE), REJECT_INVALID,
                            "bad-getaccount");
        if (txUid.is<CRegID>() && !txAccount.HaveOwnerPubKey())
            return state.DoS(100, ERRORMSG("%s, account unregistered", TX_ERR_TITLE),
                            REJECT_INVALID, "bad-account-unregistered");

        if (!VerifySignature(context, txAccount.owner_pubkey)) return false;

        return true;
    }

    bool CDEXSettleTx::ExecuteTx(CTxExecuteContext &context) {

        CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
        vector<CReceipt> receipts;

        shared_ptr<CAccount> pTxAccount = make_shared<CAccount>();
        if (!cw.accountCache.GetAccount(txUid, *pTxAccount)) {
            return state.DoS(100, ERRORMSG("%s, read source addr account info error", TX_ERR_TITLE),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!pTxAccount->OperateBalance(fee_symbol, SUB_FREE, llFees)) {
            return state.DoS(100, ERRORMSG("%s, account has insufficient funds", TX_ERR_TITLE),
                            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
        }

        map<CRegID, shared_ptr<CAccount>> accountMap = {
            {pTxAccount->regid, pTxAccount}
        };
        for (size_t i = 0; i < dealItems.size(); i++) {
            auto &dealItem = dealItems[i];
            CDealItemExecuter dealItemExec(dealItem, i, *this, context, pTxAccount, accountMap, receipts);
            if (!dealItemExec.Execute()) {
                return false;
            }
        }

        // save accounts, include tx account
        for (auto accountItem : accountMap) {
            auto pAccount = accountItem.second;
            if (!cw.accountCache.SetAccount(pAccount->keyid, *pAccount))
                return state.DoS(100, ERRORMSG("%s, set account info error! regid=%s, addr=%s",
                    TX_ERR_TITLE, pAccount->regid.ToString(), pAccount->keyid.ToAddress()),
                    WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
        }

        if(!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
            return state.DoS(100, ERRORMSG("%s, set tx receipts failed!! txid=%s", TX_ERR_TITLE,
                            GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
        return true;
    }

} // namespace dex