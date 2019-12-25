// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"

#include "config/configuration.h"
#include "main.h"

#include <algorithm>

using uint128_t = unsigned __int128;

#define ERROR_TITLE(msg) (std::string(__FUNCTION__) + "(), " + msg)

static bool CheckOrderFeeRateRange(CTxExecuteContext &context, const uint256 &orderId,
                              uint64_t fee_ratio, const string &title) {
    static_assert(DEX_ORDER_FEE_RATIO_MAX < 100 * PRICE_BOOST, "fee rate must < 100%");
    if (fee_ratio <= DEX_ORDER_FEE_RATIO_MAX)
        return context.pState->DoS(100, ERRORMSG("%s(), order fee_ratio invalid! order_id=%s, fee_rate=%llu",
            title, orderId.ToString(), fee_ratio), REJECT_INVALID, "invalid-fee-ratio");
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

///////////////////////////////////////////////////////////////////////////////
// class CDEXOrderBaseTx

bool CDEXOrderBaseTx::CheckOrderAmountRange(CValidationState &state, const string &title,
                                          const TokenSymbol &symbol, const int64_t amount) {
    // TODO: should check the min amount of order by symbol
    static_assert(MIN_DEX_ORDER_AMOUNT < INT64_MAX, "minimum dex order amount out of range");
    if (amount < (int64_t)MIN_DEX_ORDER_AMOUNT)
        return state.DoS(100, ERRORMSG("%s amount is too small, symbol=%s, amount=%llu, min_amount=%llu",
                        title, symbol, amount, MIN_DEX_ORDER_AMOUNT), REJECT_INVALID, "order-amount-too-small");

    if (!CheckCoinRange(symbol, amount))
        return state.DoS(100, ERRORMSG("%s amount is out of range, symbol=%s, amount=%llu",
                        title, symbol, amount), REJECT_INVALID, "invalid-order-amount-range");

    return true;
}

bool CDEXOrderBaseTx::CheckOrderPriceRange(CValidationState &state, const string &title,
                          const TokenSymbol &coin_symbol, const TokenSymbol &asset_symbol,
                          const int64_t price) {
    // TODO: should check the price range??
    if (price <= 0)
        return state.DoS(100, ERRORMSG("%s price out of range,"
                        " coin_symbol=%s, asset_symbol=%s, price=%llu",
                        title, coin_symbol, asset_symbol, price),
                        REJECT_INVALID, "invalid-price-range");

    return true;
}

bool CDEXOrderBaseTx::CheckOrderSymbols(CValidationState &state, const string &title,
                          const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol) {
    if (coinSymbol.empty() || coinSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(coinSymbol) == 0) {
        return state.DoS(100, ERRORMSG("%s invalid order coin symbol=%s", title, coinSymbol),
                        REJECT_INVALID, "invalid-order-coin-symbol");
    }

    if (assetSymbol.empty() || assetSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(assetSymbol) == 0) {
        return state.DoS(100, ERRORMSG("%s invalid order asset symbol=%s", title, assetSymbol),
                        REJECT_INVALID, "invalid-order-asset-symbol");
    }

    if (kTradingPairSet.count(make_pair(assetSymbol, coinSymbol)) == 0) {
        return state.DoS(100, ERRORMSG("%s not support the trading pair! coin_symbol=%s, asset_symbol=%s",
            title, coinSymbol, assetSymbol), REJECT_INVALID, "invalid-trading-pair");
    }

    return true;
}

bool CDEXOrderBaseTx::CheckDexOperatorExist(CTxExecuteContext &context) {
    if (dex_id != DEX_RESERVED_ID) {
        if (!context.pCw->dexCache.HaveDexOperator(dex_id))
            return context.pState->DoS(100, ERRORMSG("%s, dex operator does not exist! dex_id=%d", ERROR_TITLE(GetTxTypeName()), dex_id),
                REJECT_INVALID, "bad-getaccount");
    }
    return true;
}

bool CDEXOrderBaseTx::CheckOrderFeeRate(CTxExecuteContext &context, const string &title) {
    if (mode.value == OrderOperatorMode::DEFAULT) {
        if (operator_fee_ratio != 0)
            return context.pState->DoS(100, ERRORMSG("%s(), order fee ratio must be 0 in DEFAULT_MODE mode, operator_fee_ratio=%llu",
                title, operator_fee_ratio), REJECT_INVALID, "invalid-order-fee-ratio");
    } else if (mode.value == OrderOperatorMode::REQUIRE_AUTH) {
        if (!CheckOrderFeeRateRange(context, GetHash(), operator_fee_ratio, title))
            return false;
    } else {
        return context.pState->DoS(100, ERRORMSG("%s(), invalid order operator mode=%d", title, mode.Name()),
            REJECT_INVALID, "invalid-order-operator-mode");
    }
    return true;
}

bool CDEXOrderBaseTx::CheckOrderOperator(CTxExecuteContext &context, const string &title) {

    if (!mode.IsValid())
        return context.pState->DoS(100, ERRORMSG("%s, invalid order operator mode=%d",
            title, mode.Name()), REJECT_INVALID, "invalid-order-operator-mode");

    if (mode == OrderOperatorMode::DEFAULT) {
        if (operator_fee_ratio != 0)
            return context.pState->DoS(100, ERRORMSG("%s(), order fee ratio=%llu must be 0 in %s mode",
                title, operator_fee_ratio, mode.Name()), REJECT_INVALID, "invalid-order-fee-ratio");

        if (operator_signature_pair)
            return context.pState->DoS(100, ERRORMSG("%s, the optional operator signature must be empty in %s mode",
                title, mode.Name()), REJECT_INVALID, "bad-getaccount");

        if (!CheckDexOperatorExist(context)) return false;

    } else {

        if (!CheckOrderFeeRateRange(context, GetHash(), operator_fee_ratio, title))
            return false;

        if (!operator_signature_pair) {
            return context.pState->DoS(100, ERRORMSG("%s, the optional operator signature can not be empty in %s mode",
                title, mode.Name()), REJECT_INVALID, "bad-getaccount");
        }

        shared_ptr<DexOperatorDetail> spOperatorDetail;
        if(!GetDexOperator(context, dex_id, spOperatorDetail, title)) return false;

        CRegID &operator_regid = operator_signature_pair.value().regid;
        if (operator_regid != spOperatorDetail->match_regid)
            return context.pState->DoS(100, ERRORMSG("%s, wrong operator regid=%s vs %s! mode=%s",
                title, operator_regid.ToString(), spOperatorDetail->match_regid.ToString(), mode.Name()),
                REJECT_INVALID, "bad-getaccount");

        CAccount operatorAccount;
        if (!context.pCw->accountCache.GetAccount(operator_regid, operatorAccount))
            return context.pState->DoS(100, ERRORMSG("%s, read account failed! operator_regid=%s, mode=%s",
                title, operator_regid.ToString(), mode.Name()),
                REJECT_INVALID, "bad-getaccount");

        if (!operatorAccount.IsRegistered())
            return context.pState->DoS(100, ERRORMSG("%s, the operator account must be registered! "
                "operator_regid=%s, mode=%s", title, operator_regid.ToString(), mode.Name()),
                REJECT_INVALID, "operator-account-unregistered");

        const UnsignedCharArray operatorSignature = operator_signature_pair.value().signature;
        if (!CheckSignatureSize(operatorSignature)) {
            return context.pState->DoS(100, ERRORMSG("%s, operator signature size=%d invalid! mode=%s", title,
                operatorSignature.size(), mode.Name()), REJECT_INVALID, "bad-operator-sig-size");
        }
        uint256 sighash = GetHash();
        if (!VerifySignature(sighash, operatorSignature, operatorAccount.owner_pubkey)) {
            return context.pState->DoS(100, ERRORMSG("%s, check operator signature error! mode=%s",
                title, mode.Name()), REJECT_INVALID, "bad-operator-signature");
        }
    }
    return true;
}

bool CDEXOrderBaseTx::ProcessOrder(CTxExecuteContext &context, CAccount &txAccount, const string &title) {
    // shared_ptr<DexOperatorDetail> pOperatorDetail;
    // if (!GetDexOperator(context, dex_id, pOperatorDetail, ERROR_TITLE(GetTxTypeName()))) return false;

    uint64_t coinAmount = coin_amount;
    if (order_type == ORDER_MARKET_PRICE && order_side == ORDER_BUY)
        coinAmount = CalcCoinAmount(asset_amount, price);

    if (order_side == ORDER_BUY) {
        if (!FreezeBalance(context, txAccount, coin_symbol, coinAmount, title)) return false;
    } else {
        assert(order_side == ORDER_SELL);
        if (!FreezeBalance(context, txAccount, asset_symbol, asset_amount, title)) return false;
    }

    assert(!txAccount.regid.IsEmpty());
    const uint256 &txid = GetHash();
    CDEXOrderDetail orderDetail;
    orderDetail.mode               = mode;
    orderDetail.dex_id             = dex_id;
    orderDetail.operator_fee_ratio = operator_fee_ratio;
    orderDetail.generate_type      = USER_GEN_ORDER;
    orderDetail.order_type         = ORDER_LIMIT_PRICE;
    orderDetail.order_side         = ORDER_BUY;
    orderDetail.coin_symbol        = coin_symbol;
    orderDetail.asset_symbol       = asset_symbol;
    orderDetail.coin_amount        = coinAmount;
    orderDetail.asset_amount       = asset_amount;
    orderDetail.price              = price;
    orderDetail.tx_cord            = CTxCord(context.height, context.index);
    orderDetail.user_regid         = txAccount.regid;
    // other fields keep the default value

    if (!context.pCw->dexCache.CreateActiveOrder(txid, orderDetail))
        return context.pState->DoS(100, ERRORMSG("%s, create active buy order failed! txid=%s",
            title, txid.ToString()), REJECT_INVALID, "bad-write-dexdb");

    return true;
}

bool CDEXOrderBaseTx::FreezeBalance(CTxExecuteContext &context, CAccount &account,
                                    const TokenSymbol &tokenSymbol, const uint64_t &amount,
                                    const string &title) {

    if (!account.OperateBalance(tokenSymbol, FREEZE, amount)) {
        return context.pState->DoS(100,
            ERRORMSG("%s, account has insufficient funds! regid=%s, symbol=%s, amount=%llu", title,
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
// class CDEXBuyLimitOrderBaseTx

string CDEXBuyLimitOrderBaseTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, "
                     "mode=%s, dex_id=%u, operator_fee_ratio=%llu, coin_symbol=%s, "
                     "asset_symbol=%s, amount=%llu, price=%llu, memo_hex=%s",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height,
                     txUid.ToString(), llFees, mode.Name(), dex_id, operator_fee_ratio, coin_symbol,
                     asset_symbol, asset_amount, price, HexStr(memo));
}

Object CDEXBuyLimitOrderBaseTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("mode",               mode.Name()));
    result.push_back(Pair("dex_id",             (uint64_t)dex_id));
    result.push_back(Pair("operator_fee_ratio", operator_fee_ratio));
    result.push_back(Pair("coin_symbol",        coin_symbol));
    result.push_back(Pair("asset_symbol",       asset_symbol));
    result.push_back(Pair("asset_amount",       asset_amount));
    result.push_back(Pair("price",              price));
    result.push_back(Pair("memo",               memo));
    result.push_back(Pair("memo_hex",           HexStr(memo)));
    return result;
}

bool CDEXBuyLimitOrderBaseTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;

    if (!CheckOrderSymbols(state, ERROR_TITLE(GetTxTypeName()), coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, ERROR_TITLE(GetTxTypeName() + " asset"), asset_symbol, asset_amount)) return false;

    if (!CheckOrderPriceRange(state, ERROR_TITLE(GetTxTypeName()), coin_symbol, asset_symbol, price)) return false;

    if (!CheckOrderOperator(context, ERROR_TITLE(GetTxTypeName())) ) return false;

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("%s, read account failed", ERROR_TITLE(GetTxTypeName())),
            REJECT_INVALID, "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyLimitOrderBaseTx::ExecuteTx(CTxExecuteContext &context) {
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

    if (!ProcessOrder(context, txAccount, ERROR_TITLE(GetTxTypeName()))) return false;

    if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
        return state.DoS(100, ERRORMSG("%s, set account info error", ERROR_TITLE(GetTxTypeName())),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

bool CDEXBuyLimitOrderTx::CheckTx(CTxExecuteContext &context) {
    // TODO: check version < 3
    return CDEXBuyLimitOrderBaseTx::CheckTx(context);
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

bool CDEXBuyLimitOrderExTx::CheckTx(CTxExecuteContext &context) {
    // TODO: check version >= 3
    return CDEXBuyLimitOrderBaseTx::CheckTx(context);
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellLimitOrderTx

string CDEXSellLimitOrderBaseTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, "
                     "mode=%s, dex_id=%u, operator_fee_ratio=%llu, coin_symbol=%s, "
                     "asset_symbol=%s, amount=%llu, price=%llu, memo_hex=%s",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height,
                     txUid.ToString(), llFees, mode.Name(), dex_id, operator_fee_ratio, coin_symbol,
                     asset_symbol, asset_amount, price, HexStr(memo));
}

Object CDEXSellLimitOrderBaseTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("mode",               mode.Name()));
    result.push_back(Pair("dex_id",             (uint64_t)dex_id));
    result.push_back(Pair("operator_fee_ratio", operator_fee_ratio));
    result.push_back(Pair("coin_symbol",        coin_symbol));
    result.push_back(Pair("asset_symbol",       asset_symbol));
    result.push_back(Pair("asset_amount",       asset_amount));
    result.push_back(Pair("price",              price));
    result.push_back(Pair("memo",               memo));
    result.push_back(Pair("memo_hex",           HexStr(memo)));
    return result;
}

bool CDEXSellLimitOrderBaseTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;

    if (!CheckOrderSymbols(state, "CDEXSellLimitOrderTx::CheckTx,", coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, "CDEXSellLimitOrderTx::CheckTx, asset,", asset_symbol, asset_amount)) return false;

    if (!CheckOrderPriceRange(state, "CDEXSellLimitOrderTx::CheckTx,", coin_symbol, asset_symbol, price)) return false;

    if (!CheckOrderOperator(context, ERROR_TITLE(GetTxTypeName())) ) return false;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey );
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXSellLimitOrderBaseTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, txAccount)) {
        return false;
    }

    if (!txAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    // freeze user's asset for selling.
    if (!txAccount.OperateBalance(asset_symbol, FREEZE, asset_amount)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!ProcessOrder(context, txAccount, ERROR_TITLE(GetTxTypeName()))) return false;

    if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyMarketOrderBaseTx

string CDEXBuyMarketOrderBaseTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, "
                     "mode=%s, dex_id=%u, operator_fee_ratio=%llu, coin_symbol=%s, "
                     "asset_symbol=%s, coin_amount=%llu, memo_hex=%s",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height,
                     txUid.ToString(), llFees, mode.Name(), dex_id, operator_fee_ratio, coin_symbol,
                     asset_symbol, coin_amount, HexStr(memo));
}

Object CDEXBuyMarketOrderBaseTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("mode",               mode.Name()));
    result.push_back(Pair("dex_id",             (uint64_t)dex_id));
    result.push_back(Pair("operator_fee_ratio", operator_fee_ratio));
    result.push_back(Pair("coin_symbol",        coin_symbol));
    result.push_back(Pair("asset_symbol",       asset_symbol));
    result.push_back(Pair("coin_amount",        coin_amount));
    result.push_back(Pair("memo",               memo));
    result.push_back(Pair("memo_hex",           HexStr(memo)));

    return result;
}

bool CDEXBuyMarketOrderBaseTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;

    if (!CheckOrderSymbols(state, "CDEXBuyMarketOrderTx::CheckTx,", coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, "CDEXBuyMarketOrderTx::CheckTx, coin,", coin_symbol, coin_amount)) return false;

    if (!CheckOrderOperator(context, ERROR_TITLE(GetTxTypeName())) ) return false;

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyMarketOrderBaseTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, txAccount)) {
        return false;
    }

    if (!txAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's coin for buying the asset
    if (!txAccount.OperateBalance(coin_symbol, FREEZE, coin_amount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!ProcessOrder(context, txAccount, ERROR_TITLE(GetTxTypeName()))) return false;

    if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellMarketOrderBaseTx

string CDEXSellMarketOrderBaseTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, "
                     "mode=%s, dex_id=%u, operator_fee_ratio=%llu, coin_symbol=%s, "
                     "asset_symbol=%s, amount=%llu, memo_hex=%s",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height,
                     txUid.ToString(), llFees, mode.Name(), dex_id, operator_fee_ratio, coin_symbol,
                     asset_symbol, asset_amount, HexStr(memo));
}

Object CDEXSellMarketOrderBaseTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("mode",               mode.Name()));
    result.push_back(Pair("dex_id",             (uint64_t)dex_id));
    result.push_back(Pair("operator_fee_ratio", operator_fee_ratio));
    result.push_back(Pair("coin_symbol",        coin_symbol));
    result.push_back(Pair("asset_symbol",       asset_symbol));
    result.push_back(Pair("asset_amount",       asset_amount));
    result.push_back(Pair("memo",               memo));
    result.push_back(Pair("memo_hex",           HexStr(memo)));
    return result;
}

bool CDEXSellMarketOrderBaseTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;

    if (!CheckOrderSymbols(state, "CDEXSellMarketOrderTx::CheckTx,", coin_symbol, asset_symbol))
        return false;

    if (!CheckOrderAmountRange(state, "CDEXBuyMarketOrderTx::CheckTx, asset,", asset_symbol, asset_amount))
        return false;

    if (!CheckOrderOperator(context, ERROR_TITLE(GetTxTypeName())) ) return false;

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXSellMarketOrderBaseTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, txAccount)) {
        return false;
    }

    if (!txAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's asset for selling
    if (!txAccount.OperateBalance(asset_symbol, FREEZE, asset_amount)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!ProcessOrder(context, txAccount, ERROR_TITLE(GetTxTypeName()))) return false;

    if (!cw.accountCache.SetAccount(CUserID(txAccount.keyid), txAccount))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXCancelOrderTx

string CDEXCancelOrderTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, orderId=%s",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        orderId.GetHex());
}

Object CDEXCancelOrderTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("order_id", orderId.GetHex()));

    return result;
}

bool CDEXCancelOrderTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;

    if (orderId.IsEmpty())
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, order_id is empty"), REJECT_INVALID,
                         "invalid-order-id");
    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : txAccount.owner_pubkey);
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
    if (!cw.dexCache.GetActiveOrder(orderId, activeOrder)) {
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

    if (!cw.dexCache.EraseActiveOrder(orderId, activeOrder)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, erase active order failed! order_id=%s", orderId.ToString()),
                         REJECT_INVALID, "order-erase-failed");
    }

    if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, write tx receipt failed! txid=%s", GetHash().ToString()),
                         REJECT_INVALID, "write-tx-receipt-failed");

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// class DEXDealItem
string DEXDealItem::ToString() const {
    return strprintf(
        "buy_order_id=%s, sell_order_id=%s, price=%llu, coin_amount=%llu, asset_amount=%llu",
        buyOrderId.ToString(), sellOrderId.ToString(), dealPrice, dealCoinAmount, dealAssetAmount);
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSettleTx

string CDEXSettleBaseTx::ToString(CAccountDBCache &accountCache) {
    string dealInfo="";
    for (const auto &item : dealItems) {
        dealInfo += "{" + item.ToString() + "},";
    }

    return strprintf("txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%llu, dex_id, "
                     "deal_items=[%s], memo_hex=%s",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height,
                     txUid.ToString(), llFees, dex_id, dealInfo, HexStr(memo));
}

Object CDEXSettleBaseTx::ToJson(const CAccountDBCache &accountCache) const {
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
    result.push_back(Pair("dex_id",  (uint64_t)dex_id));
    result.push_back(Pair("deal_items",  arrayItems));
    result.push_back(Pair("memo",  memo));
    result.push_back(Pair("memo_hex", HexStr(memo)));

    return result;
}

bool CDEXSettleBaseTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;

    if (txUid.get<CRegID>() != SysCfg().GetDexMatchSvcRegId()) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account regId is not authorized dex match-svc regId"),
                         REJECT_INVALID, "unauthorized-settle-account");
    }

    if (dealItems.empty() || dealItems.size() > MAX_SETTLE_ITEM_COUNT)
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, deal items is empty or count=%d is too large than %d",
            dealItems.size(), MAX_SETTLE_ITEM_COUNT), REJECT_INVALID, "invalid-deal-items");

    for (size_t i = 0; i < dealItems.size(); i++) {
        const DEXDealItem & dealItem = dealItems.at(i);
        if (dealItem.buyOrderId.IsEmpty() || dealItem.sellOrderId.IsEmpty())
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, deal_items[%d], buy_order_id or sell_order_id is empty",
                i), REJECT_INVALID, "invalid-deal-item");
        if (dealItem.buyOrderId == dealItem.sellOrderId)
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, deal_items[%d], buy_order_id cannot equal to sell_order_id",
                i), REJECT_INVALID, "invalid-deal-item");
        if (dealItem.dealCoinAmount == 0 || dealItem.dealAssetAmount == 0 || dealItem.dealPrice == 0)
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, deal_items[%d],"
                " deal_coin_amount or deal_asset_amount or deal_price is zero",
                i), REJECT_INVALID, "invalid-deal-item");
    }

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(txUid, txAccount))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");
    if (txUid.type() == typeid(CRegID) && !txAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    IMPLEMENT_CHECK_TX_SIGNATURE(txAccount.owner_pubkey);

    return true;
}

static bool GetAccount(CTxExecuteContext &context, const CRegID &regid,
                       map<CRegID, shared_ptr<CAccount>> &accountMap,
                       shared_ptr<CAccount> &pAccount) {

    auto accountIt = accountMap.find(regid);
    if (accountIt != accountMap.end()) {
        pAccount = accountIt->second;
    } else {
        pAccount = make_shared<CAccount>();
        if (!context.pCw->accountCache.GetAccount(regid, *pAccount)) {
            return context.pState->DoS(100, ERRORMSG("%s(), read account info error! regid=%s",
                __func__, regid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        accountMap[regid] = pAccount;
    }
    return true;
}

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
    b. else check: (dealCoinAmount / 10000) == (CalcCoinAmount(dealAssetAmount, price) / 10000)
    c. operate total deal:
        buyActiveOrder.total_deal_coin_amount  += dealCoinAmount
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
                residualCoinAmount = buyOrder.coin_amount - buyActiveOrder.total_deal_coin_amount
                if residualCoinAmount > 0 {
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
bool CDEXSettleBaseTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    vector<CReceipt> receipts;

    shared_ptr<CAccount> pSrcAccount = make_shared<CAccount>();
   if (!cw.accountCache.GetAccount(txUid, *pSrcAccount)) {
        return state.DoS(100, ERRORMSG("%s(), read source addr account info error", __func__),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!pSrcAccount->OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("%s(), account has insufficient funds", __func__),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    shared_ptr<DexOperatorDetail> pSettleOperatorDetail;
    if (!GetDexOperator(context, dex_id, pSettleOperatorDetail, ERROR_TITLE(GetTxTypeName()))) return false;

    if (!pSrcAccount->IsMyUid(pSettleOperatorDetail->match_regid))
        return state.DoS(100, ERRORMSG("%s(), tx account is not the matcher of dex operator! dex_id=%u, "
            "tx_uid=%s, match_regid=%s", __func__, dex_id, txUid.ToDebugString(),
            pSettleOperatorDetail->match_regid.ToString()),
            REJECT_INVALID, "invalid_dex_operator_matcher");

    map<CRegID, shared_ptr<CAccount>> accountMap = {
        {pSrcAccount->regid, pSrcAccount}
    };
    for (size_t i = 0; i < dealItems.size(); i++) {
        auto &dealItem = dealItems[i];
        //1. get and check buyDealOrder and sellDealOrder
        CDEXOrderDetail buyOrder, sellOrder;
        if (!GetDealOrder(cw, state, i, dealItem.buyOrderId, ORDER_BUY, buyOrder))
            return false;

        if (!GetDealOrder(cw, state, i, dealItem.sellOrderId, ORDER_SELL, sellOrder))
            return false;

        // 2. get account of order
        shared_ptr<CAccount> pBuyOrderAccount = nullptr;
        if (!GetAccount(context, buyOrder.user_regid, accountMap, pBuyOrderAccount)) return false;

        shared_ptr<CAccount> pSellOrderAccount = nullptr;
        if (!GetAccount(context, sellOrder.user_regid, accountMap, pSellOrderAccount)) return false;

        // check dex_id
        uint32_t buyDexId = buyOrder.dex_id;
        uint32_t sellDexId = sellOrder.dex_id;
        if (!CheckDexId(context, i, buyDexId, sellDexId)) return false;

        shared_ptr<DexOperatorDetail> pBuyOperatorDetail, pSellOperatorDetail;
        shared_ptr<CAccount> pBuyMatchAccount, pSellMatchAccount;
        if (buyDexId == dex_id) {
            pBuyOperatorDetail = pSettleOperatorDetail;
            pBuyMatchAccount = pSrcAccount;
        } else {
            if (!GetDexOperator(context, buyOrder.dex_id, pBuyOperatorDetail, ERROR_TITLE(GetTxTypeName()))) return false;
            if (!GetAccount(context, pBuyOperatorDetail->match_regid, accountMap, pBuyMatchAccount)) return false;
        }
        if (sellDexId == dex_id) {
            pSellOperatorDetail = pSettleOperatorDetail;
            pSellMatchAccount = pSrcAccount;
        } else {
            if (!GetDexOperator(context, sellOrder.dex_id, pSellOperatorDetail, ERROR_TITLE(GetTxTypeName()))) return false;
            if (!GetAccount(context, pSellOperatorDetail->match_regid, accountMap, pSellMatchAccount)) return false;
        }

        // 3. check coin type match
        if (buyOrder.coin_symbol != sellOrder.coin_symbol) {
            return state.DoS(100, ERRORMSG("%s(), i[%d] coin symbol unmatch! buyer coin_symbol=%s, " \
                "seller coin_symbol=%s", __FUNCTION__, i, buyOrder.coin_symbol, sellOrder.coin_symbol),
                REJECT_INVALID, "coin-symbol-unmatch");
        }
        // 4. check asset type match
        if (buyOrder.asset_symbol != sellOrder.asset_symbol) {
            return state.DoS(100, ERRORMSG("%s(), i[%d] asset symbol unmatch! buyer asset_symbol=%s, " \
                "seller asset_symbol=%s", __FUNCTION__, i, buyOrder.asset_symbol, sellOrder.asset_symbol),
                REJECT_INVALID, "asset-symbol-unmatch");
        }

        // 5. check price match
        if (buyOrder.order_type == ORDER_LIMIT_PRICE && sellOrder.order_type == ORDER_LIMIT_PRICE) {
            if ( buyOrder.price < dealItem.dealPrice
                || sellOrder.price > dealItem.dealPrice ) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] the expected price unmatch! buyer limit price=%llu, "
                    "seller limit price=%llu, deal_price=%llu",
                    __FUNCTION__, i, buyOrder.price, sellOrder.price, dealItem.dealPrice),
                    REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrder.order_type == ORDER_LIMIT_PRICE && sellOrder.order_type == ORDER_MARKET_PRICE) {
            if (dealItem.dealPrice != buyOrder.price) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] the expected price unmatch! buyer limit price=%llu, "
                    "seller market price, deal_price=%llu",
                    __FUNCTION__, i, buyOrder.price, dealItem.dealPrice),
                    REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrder.order_type == ORDER_MARKET_PRICE && sellOrder.order_type == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice != sellOrder.price) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] the expected price unmatch! buyer market price, "
                    "seller limit price=%llu, deal_price=%llu",
                    __FUNCTION__, i, sellOrder.price, dealItem.dealPrice),
                    REJECT_INVALID, "deal-price-unmatch");
            }
        } else {
            assert(buyOrder.order_type == ORDER_MARKET_PRICE && sellOrder.order_type == ORDER_MARKET_PRICE);
            // no limit
        }

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
            return state.DoS(100, ERRORMSG("%s(), i[%d] the deal_coin_amount unmatch!"
                " deal_info={%s}, calcCoinAmount=%llu",
                __FUNCTION__, i, dealItem.ToString(), calcCoinAmount),
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
                return state.DoS(100, ERRORMSG( "%s(), i[%d] the total_deal_coin_amount=%llu exceed the buyer's "
                    "coin_amount=%llu", __FUNCTION__, i, buyOrder.total_deal_coin_amount, limitCoinAmount),
                    REJECT_INVALID, "buy-deal-coin-amount-exceeded");
            }

            buyResidualAmount = limitCoinAmount - buyOrder.total_deal_coin_amount;
        } else {
            uint64_t limitAssetAmount = buyOrder.asset_amount;
            if (limitAssetAmount < buyOrder.total_deal_asset_amount) {
                return state.DoS(
                    100,
                    ERRORMSG("%s(), i[%d] the total_deal_asset_amount=%llu exceed the "
                             "buyer's asset_amount=%llu",
                             __FUNCTION__, i, buyOrder.total_deal_asset_amount, limitAssetAmount),
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
                    ERRORMSG("%s(), i[%d] the total_deal_asset_amount=%llu exceed the "
                             "seller's asset_amount=%llu",
                             __FUNCTION__, i, sellOrder.total_deal_asset_amount, limitAssetAmount),
                    REJECT_INVALID, "sell-deal-amount-exceeded");
            }
            sellResidualAmount = limitAssetAmount - sellOrder.total_deal_asset_amount;
        }

        // 8. subtract the buyer's coins and seller's assets
        // - unfree and subtract the coins from buyer account
        if (   !pBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, UNFREEZE, dealItem.dealCoinAmount)
            || !pBuyOrderAccount->OperateBalance(buyOrder.coin_symbol, SUB_FREE, dealItem.dealCoinAmount)) {// - subtract buyer's coins
            return state.DoS(100, ERRORMSG("%s(), i[%d] subtract coins from buyer account failed!"
                " deal_info={%s}, coin_symbol=%s",
                __FUNCTION__, i, dealItem.ToString(), buyOrder.coin_symbol),
                REJECT_INVALID, "operate-account-failed");
        }
        // - unfree and subtract the assets from seller account
        if (   !pSellOrderAccount->OperateBalance(sellOrder.asset_symbol, UNFREEZE, dealItem.dealAssetAmount)
            || !pSellOrderAccount->OperateBalance(sellOrder.asset_symbol, SUB_FREE, dealItem.dealAssetAmount)) { // - subtract seller's assets
            return state.DoS(100, ERRORMSG("%s(), i[%d] subtract assets from seller account failed!"
                " deal_info={%s}, asset_symbol=%s",
                __FUNCTION__, i, dealItem.ToString(), sellOrder.asset_symbol),
                REJECT_INVALID, "operate-account-failed");
        }

        // 9. calc deal dex operator fees
        uint64_t buyerReceivedAssets = dealItem.dealAssetAmount;
        // 9.1 buyer pay the fee from the received assets to settler

        OrderSide takerSide = GetTakerOrderSide(buyOrder, sellOrder);
        uint64_t buyOperatorFeeRatio = GetOperatorFeeRatio(buyOrder, *pBuyOperatorDetail, takerSide);
        uint64_t sellOperatorFeeRatio = GetOperatorFeeRatio(sellOrder, *pSellOperatorDetail, takerSide);


        if (buyOperatorFeeRatio != 0) {
            if (!CheckOrderFeeRateRange(context, dealItem.buyOrderId, buyOperatorFeeRatio, ERROR_TITLE(GetTxTypeName())))
                return false;

            uint64_t dealAssetFee;
            if (!CalcOrderFee(context, i, dealItem.dealAssetAmount, buyOperatorFeeRatio, dealAssetFee)) return false;

            buyerReceivedAssets = dealItem.dealAssetAmount - dealAssetFee;
            // pay asset fee from seller to settler
            if (!pBuyMatchAccount->OperateBalance(buyOrder.asset_symbol, ADD_FREE, dealAssetFee)) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] pay asset fee from buyer to operator matcher failed!"
                    " deal_info={%s}, asset_symbol=%s, asset_fee=%llu, buy_match_regid=%s",
                    __FUNCTION__, i, dealItem.ToString(), buyOrder.asset_symbol, dealAssetFee, pBuyMatchAccount->regid.ToString()),
                    REJECT_INVALID, "operate-account-failed");
            }

            receipts.emplace_back(pBuyOrderAccount->regid, pBuyMatchAccount->regid, buyOrder.asset_symbol,
                               dealAssetFee, ReceiptCode::DEX_ASSET_FEE_TO_SETTLER);
        }
        // 9.2 seller pay the fee from the received coins to settler
        uint64_t sellerReceivedCoins = dealItem.dealCoinAmount;
        if (sellOperatorFeeRatio != 0) {
            if (!CheckOrderFeeRateRange(context, dealItem.sellOrderId, sellOperatorFeeRatio, ERROR_TITLE(GetTxTypeName())))
                return false;
            uint64_t dealCoinFee;
            if (!CalcOrderFee(context, i, dealItem.dealCoinAmount, sellOperatorFeeRatio, dealCoinFee)) return false;

            sellerReceivedCoins = dealItem.dealCoinAmount - dealCoinFee;
            // pay coin fee from buyer to settler
            if (!pSrcAccount->OperateBalance(sellOrder.coin_symbol, ADD_FREE, dealCoinFee)) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] pay coin fee from seller to operator matcher failed!"
                    " deal_info={%s}, coin_symbol=%s, coin_fee=%llu, sell_match_regid=%s",
                    __FUNCTION__, i, dealItem.ToString(), sellOrder.coin_symbol, dealCoinFee, pSellMatchAccount->regid.ToString()),
                    REJECT_INVALID, "operate-account-failed");
            }
            receipts.emplace_back(pSellOrderAccount->regid, pSrcAccount->regid, sellOrder.coin_symbol,
                                  dealCoinFee, ReceiptCode::DEX_COIN_FEE_TO_SETTLER);
        }

        // 10. add the buyer's assets and seller's coins
        if (   !pBuyOrderAccount->OperateBalance(buyOrder.asset_symbol, ADD_FREE, buyerReceivedAssets)    // + add buyer's assets
            || !pSellOrderAccount->OperateBalance(sellOrder.coin_symbol, ADD_FREE, sellerReceivedCoins)){ // + add seller's coin
            return state.DoS(100,ERRORMSG("%s(), i[%d] add assets to buyer or add coins to seller failed!"
                " deal_info={%s}, asset_symbol=%s, assets=%llu, coin_symbol=%s, coins=%llu",
                __FUNCTION__, i, dealItem.ToString(), buyOrder.asset_symbol,
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
                        return state.DoS(100, ERRORMSG("%s(), i[%d] unfreeze buyer's residual coins failed!"
                            " deal_info={%s}, coin_symbol=%s, residual_coins=%llu",
                            __FUNCTION__, i, dealItem.ToString(), buyOrder.coin_symbol, residualCoinAmount),
                            REJECT_INVALID, "operate-account-failed");
                    }
                } else {
                    assert(buyOrder.coin_amount == buyOrder.total_deal_coin_amount);
                }
            }
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.buyOrderId, buyOrder)) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] finish the active buy order failed! deal_info={%s}",
                    __FUNCTION__, i, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.UpdateActiveOrder(dealItem.buyOrderId, buyOrder)) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] update active buy order failed! deal_info={%s}",
                    __FUNCTION__, i, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (sellResidualAmount == 0) { // sell order fulfilled
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.sellOrderId, sellOrder)) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] finish active sell order failed! deal_info={%s}",
                    __func__, i, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.UpdateActiveOrder(dealItem.sellOrderId, sellOrder)) {
                return state.DoS(100, ERRORMSG("%s(), i[%d] update active sell order failed! deal_info={%s}",
                    __func__, i, dealItem.ToString()),
                    REJECT_INVALID, "write-dexdb-failed");
            }
        }
    }

    // save accounts, include tx account
    for (auto accountItem : accountMap) {
        auto pAccount = accountItem.second;
        if (!cw.accountCache.SetAccount(pAccount->keyid, *pAccount))
            return state.DoS(100, ERRORMSG("%s(), set account info error! regid=%s, addr=%s",
                __func__, pAccount->regid.ToString(), pAccount->keyid.ToAddress()),
                WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    if(!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("%s(), set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
    return true;
}

bool CDEXSettleBaseTx::GetDealOrder(CCacheWrapper &cw, CValidationState &state, uint32_t index, const uint256 &orderId,
                                const OrderSide orderSide, CDEXOrderDetail &dealOrder) {
    if (!cw.dexCache.GetActiveOrder(orderId, dealOrder))
        return state.DoS(100, ERRORMSG("%s(), get active order failed! i=%d, orderId=%s", __func__,
            index, orderId.ToString()), REJECT_INVALID,
            strprintf("get-active-order-failed, i=%d, order_id=%s", index, orderId.ToString()));

    if (dealOrder.order_side != orderSide)
        return state.DoS(100, ERRORMSG("%s(), expected order_side=%s "
                "but got order_side=%s! i=%d, orderId=%s", __func__, GetOrderSideName(orderSide),
                GetOrderSideName(dealOrder.order_side), index, orderId.ToString()),
                REJECT_INVALID,
                strprintf("order-side-unmatched, i=%d, order_id=%s", index, orderId.ToString()));

    return true;
}

bool CDEXSettleBaseTx::CheckDexId(CTxExecuteContext &context, uint32_t i, uint32_t buyDexId, uint32_t sellDexId) {
    if (buyDexId != dex_id && sellDexId != dex_id) {
        return context.pState->DoS(100, ERRORMSG("%s(), i[%d] can not settle the other operator' orders! settle_dex_id=%u, "
            "buy_dex_id=%u, sell_dex_id=%u", __FUNCTION__, i, dex_id, buyDexId, sellDexId),
            REJECT_INVALID, "dex-id-unmatch");
    } else if (buyDexId != sellDexId) {
        // Only orders from dex operator 0 can be shared
        if (buyDexId == dex_id && sellDexId != DEX_RESERVED_ID) {
            return context.pState->DoS(100, ERRORMSG("%s(), i[%d] the sell order is not shared! settle_dex_id=%u, "
                "buy_dex_id=%u, sell_dex_id=%u", __FUNCTION__, i, dex_id, buyDexId, sellDexId),
                REJECT_INVALID, "dex-id-unmatch");
        } else if (sellDexId == dex_id && buyDexId != DEX_RESERVED_ID) {
            return context.pState->DoS(100, ERRORMSG("%s(), i[%d] the buy order is not shared! settle_dex_id=%u, "
                "buy_dex_id=%u, sell_dex_id=%u", __FUNCTION__, i, dex_id, buyDexId, sellDexId),
                REJECT_INVALID, "dex-id-unmatch");
        }
    }
    return true;
}


OrderSide CDEXSettleBaseTx::GetTakerOrderSide(const CDEXOrderDetail &buyOrder, const CDEXOrderDetail &sellOrder) {
    OrderSide takerSide;
    if (buyOrder.order_type != sellOrder.order_type) {
        if (buyOrder.order_type == ORDER_MARKET_PRICE) {
            takerSide = OrderSide::ORDER_BUY;
        } else {
            assert(buyOrder.order_type == ORDER_MARKET_PRICE);
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

uint64_t CDEXSettleBaseTx::GetOperatorFeeRatio(const CDEXOrderDetail &order,
                                               const DexOperatorDetail &operatorDetail,
                                               const OrderSide &takerSide) {
    uint64_t ratio;
    if (order.mode.value == OrderOperatorMode::DEFAULT) {
        if (order.order_side == takerSide) {
            ratio = operatorDetail.taker_fee_ratio;
        } else {
            ratio = operatorDetail.maker_fee_ratio;
        }
    } else {
        ratio = order.operator_fee_ratio;
    }
    return ratio;
}

bool CDEXSettleBaseTx::CalcOrderFee(CTxExecuteContext &context, uint32_t i, uint64_t amount, uint64_t fee_ratio,
                                    uint64_t &orderFee) {

    uint128_t fee = amount * (uint128_t)fee_ratio / PRICE_BOOST;
    if (fee > (uint128_t)ULLONG_MAX)
        return context.pState->DoS(100, ERRORMSG("%s(), i[%d] the calc_order_fee out of range! amount=%llu, "
            "fee_ratio=%llu", __func__, i,  amount, fee_ratio), REJECT_INVALID, "calc-order-fee-error");
    orderFee = fee;
    return true;
}