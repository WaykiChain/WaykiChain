// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"

#include "config/configuration.h"
#include "main.h"

using uint128_t = unsigned __int128;

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

bool CDEXOrderBaseTx::CheckOrderAmountRange(CValidationState &state, const string &title,
                                          const TokenSymbol &symbol, const int64_t amount) {
    // TODO: should check the min amount of order by symbol
    if (!CheckCoinRange(symbol, amount))
        return state.DoS(100, ERRORMSG("%s amount out of range, symbol=%s, amount=%llu",
                        title, symbol, amount), REJECT_INVALID, "invalid-coin-range");

    return true;
}

bool CDEXOrderBaseTx::CheckOrderPriceRange(CValidationState &state, const string &title,
                          const TokenSymbol &coin_symbol, const TokenSymbol &asset_symbol,
                          const int64_t price) {
    // TODO: should check the price range??
    if (price < 0)
        return state.DoS(100, ERRORMSG("%s price out of range,"
                        " coin_symbol=%s, asset_symbol=%s, price=%llu",
                        title, coin_symbol, asset_symbol, price),
                        REJECT_INVALID, "invalid-price-range");

    return true;
}

bool CDEXOrderBaseTx::CheckOrderSymbols(CValidationState &state, const string &title,
                          const TokenSymbol &coinSymbol, const TokenSymbol &assetSymbol) {
    if (coinSymbol.empty() || coinSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(coinSymbol) == 0) {
        return state.DoS(100, ERRORMSG("%s invalid coin symbol=%s", title, coinSymbol),
                        REJECT_INVALID, "invalid-coin-symbol");
    }

    if (assetSymbol.empty() || assetSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(assetSymbol) == 0) {
        return state.DoS(100, ERRORMSG("%s invalid asset symbol=%s", title, assetSymbol),
                        REJECT_INVALID, "invalid-asset-symbol");
    }

    if (kTradingPairSet.count(make_pair(assetSymbol, coinSymbol)) == 0) {
        return state.DoS(100, ERRORMSG("%s not support the trading pair! coin_symbol=%s, asset_symbol=%s",
            title, coinSymbol, assetSymbol), REJECT_INVALID, "invalid-trading-pair");
    }

    return true;
}

uint64_t CDEXOrderBaseTx::CalcCoinAmount(uint64_t assetAmount, const uint64_t price) {
    uint128_t coinAmount = assetAmount * (uint128_t)price / kPercentBoost;
    assert(coinAmount < ULLONG_MAX);
    return (uint64_t)coinAmount;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

string CDEXBuyLimitOrderTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%ld,"
        "coin_symbol=%u, asset_symbol=%u, amount=%lld, price=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        coin_symbol, asset_symbol, asset_amount, bid_price);
}

Object CDEXBuyLimitOrderTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("coin_symbol",      coin_symbol));
    result.push_back(Pair("asset_symbol",     asset_symbol));
    result.push_back(Pair("asset_amount",   asset_amount));
    result.push_back(Pair("price",          bid_price));
    return result;
}

bool CDEXBuyLimitOrderTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (!CheckOrderSymbols(state, "CDEXBuyLimitOrderTx::CheckTx,", coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, "CDEXBuyLimitOrderTx::CheckTx, asset,", asset_symbol, asset_amount)) return false;

    if (!CheckOrderPriceRange(state, "CDEXBuyLimitOrderTx::CheckTx,", coin_symbol, asset_symbol, bid_price)) return false;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey );
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyLimitOrderTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // TODO: process txUid is pubkey

    if (!srcAcct.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's coin for buying the asset
    uint64_t coinAmount = CalcCoinAmount(asset_amount, bid_price);

    if (!srcAcct.OperateBalance(coin_symbol, FREEZE, coinAmount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    assert(!srcAcct.regid.IsEmpty());
    const uint256 &txid = GetHash();
    CDEXOrderDetail orderDetail;
    orderDetail.generate_type = USER_GEN_ORDER;
    orderDetail.order_type    = ORDER_LIMIT_PRICE;
    orderDetail.order_side    = ORDER_BUY;
    orderDetail.coin_symbol   = coin_symbol;
    orderDetail.asset_symbol  = asset_symbol;
    orderDetail.coin_amount   = CalcCoinAmount(asset_amount, bid_price);
    orderDetail.asset_amount  = asset_amount;
    orderDetail.price         = bid_price;
    orderDetail.tx_cord       = CTxCord(height, index);
    orderDetail.user_regid    = srcAcct.regid;
    // other fields keep the default value

    if (!cw.dexCache.CreateActiveOrder(txid, orderDetail))
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, create active buy order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellLimitOrderTx

string CDEXSellLimitOrderTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%ld,"
        "coin_symbol=%u, asset_symbol=%u, amount=%lld, price=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        coin_symbol, asset_symbol, asset_amount, ask_price);
}

Object CDEXSellLimitOrderTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("coin_symbol",      coin_symbol));
    result.push_back(Pair("asset_symbol",     asset_symbol));
    result.push_back(Pair("asset_amount",   asset_amount));
    result.push_back(Pair("price",          ask_price));
    return result;
}

bool CDEXSellLimitOrderTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (!CheckOrderSymbols(state, "CDEXSellLimitOrderTx::CheckTx,", coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, "CDEXSellLimitOrderTx::CheckTx, asset,", asset_symbol, asset_amount)) return false;

    if (!CheckOrderPriceRange(state, "CDEXSellLimitOrderTx::CheckTx,", coin_symbol, asset_symbol, ask_price)) return false;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey );
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXSellLimitOrderTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // TODO: process txUid is pubkey

    if (!srcAcct.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    // freeze user's asset for selling.
    if (!srcAcct.OperateBalance(asset_symbol, FREEZE, asset_amount)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    assert(!srcAcct.regid.IsEmpty());
    const uint256 &txid = GetHash();
    CDEXOrderDetail orderDetail;
    orderDetail.generate_type = USER_GEN_ORDER;
    orderDetail.order_type    = ORDER_LIMIT_PRICE;
    orderDetail.order_side    = ORDER_SELL;
    orderDetail.coin_symbol   = coin_symbol;
    orderDetail.asset_symbol  = asset_symbol;
    orderDetail.coin_amount   = CalcCoinAmount(asset_amount, ask_price);
    orderDetail.asset_amount  = asset_amount;
    orderDetail.price         = ask_price;
    orderDetail.tx_cord       = CTxCord(height, index);
    orderDetail.user_regid = srcAcct.regid;
    // other fields keep the default value

    if (!cw.dexCache.CreateActiveOrder(txid, orderDetail))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, create active sell order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyMarketOrderTx

string CDEXBuyMarketOrderTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%ld,"
        "coin_symbol=%u, asset_symbol=%u, amount=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        coin_symbol, asset_symbol, coin_amount);
}

Object CDEXBuyMarketOrderTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("asset_symbol",   asset_symbol));
    result.push_back(Pair("coin_amount",    coin_amount));

    return result;
}

bool CDEXBuyMarketOrderTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (!CheckOrderSymbols(state, "CDEXBuyMarketOrderTx::CheckTx,", coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, "CDEXBuyMarketOrderTx::CheckTx, coin,", coin_symbol, coin_amount)) return false;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyMarketOrderTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // TODO: process txUid is pubkey

    if (!srcAcct.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's coin for buying the asset
    if (!srcAcct.OperateBalance(coin_symbol, FREEZE, coin_amount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    assert(!srcAcct.regid.IsEmpty());
    const uint256 &txid = GetHash();
    CDEXOrderDetail orderDetail;
    orderDetail.generate_type = USER_GEN_ORDER;
    orderDetail.order_type    = ORDER_MARKET_PRICE;
    orderDetail.order_side    = ORDER_BUY;
    orderDetail.coin_symbol   = coin_symbol;
    orderDetail.asset_symbol  = asset_symbol;
    orderDetail.coin_amount   = coin_amount;
    orderDetail.asset_amount  = 0; // unkown in buy market price order
    orderDetail.price         = 0; // unkown in buy market price order
    orderDetail.tx_cord       = CTxCord(height, index);
    orderDetail.user_regid = srcAcct.regid;
    // other fields keep the default value

    if (!cw.dexCache.CreateActiveOrder(txid, orderDetail)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, create active buy order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellMarketOrderTx

string CDEXSellMarketOrderTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%ld,"
        "coin_symbol=%u, asset_symbol=%u, amount=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        coin_symbol, asset_symbol, asset_amount);
}

Object CDEXSellMarketOrderTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("asset_symbol",   asset_symbol));
    result.push_back(Pair("asset_amount",   asset_amount));
    return result;
}

bool CDEXSellMarketOrderTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (!CheckOrderSymbols(state, "CDEXSellMarketOrderTx::CheckTx,", coin_symbol, asset_symbol)) return false;

    if (!CheckOrderAmountRange(state, "CDEXBuyMarketOrderTx::CheckTx, asset,", asset_symbol, asset_amount)) return false;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXSellMarketOrderTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // TODO: process txUid is pubkey

    if (!srcAcct.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's asset for selling
    if (!srcAcct.OperateBalance(asset_symbol, FREEZE, asset_amount)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");


    assert(!srcAcct.regid.IsEmpty());
    const uint256 &txid = GetHash();
    CDEXOrderDetail orderDetail;
    orderDetail.generate_type = USER_GEN_ORDER;
    orderDetail.order_type    = ORDER_MARKET_PRICE;
    orderDetail.order_side    = ORDER_SELL;
    orderDetail.coin_symbol   = coin_symbol;
    orderDetail.asset_symbol  = asset_symbol;
    orderDetail.coin_amount   = 0; // unkown in sell market price order
    orderDetail.asset_amount  = asset_amount;
    orderDetail.price         = 0; // unkown in sell market price order
    orderDetail.tx_cord       = CTxCord(height, index);
    orderDetail.user_regid    = srcAcct.regid;
    // other fields keep the default value

    if (!cw.dexCache.CreateActiveOrder(txid, orderDetail)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, create active sell order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXCancelOrderTx

string CDEXCancelOrderTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%ld,"
        "orderId=%s\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        orderId.GetHex());
}

Object CDEXCancelOrderTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("order_id", orderId.GetHex()));

    return result;
}

bool CDEXCancelOrderTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXCancelOrderTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // TODO: process txUid is pubkey

    if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
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

    if (srcAccount.regid.IsEmpty() || srcAccount.regid != activeOrder.user_regid) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, can not cancel other user's order tx"),
                        REJECT_INVALID, "user-unmatched");
    }

    // get frozen money
    TokenSymbol frozenSymbol;
    uint64_t frozenAmount = 0;
    if (activeOrder.order_side == ORDER_BUY) {
        frozenSymbol = activeOrder.coin_symbol;
        frozenAmount = activeOrder.coin_amount - activeOrder.total_deal_coin_amount;
    } else if(activeOrder.order_side == ORDER_SELL) {
        frozenSymbol = activeOrder.asset_symbol;
        frozenAmount = activeOrder.asset_amount - activeOrder.total_deal_asset_amount;
    } else {
        assert(false && "Order side must be ORDER_BUY|ORDER_SELL");
    }

    if (!srcAccount.OperateBalance(frozenSymbol, UNFREEZE, frozenAmount)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, account has insufficient frozen amount to unfreeze"),
                         UPDATE_ACCOUNT_FAIL, "unfreeze-account-coin");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// class CDEXSettleTx
string CDEXSettleTx::ToString(CAccountDBCache &accountCache) {
    string dealInfo;
    for (const auto &item : dealItems) {
        dealInfo += strprintf("{buy_order_id:%s, sell_order_id:%s, coin_amount:%lld, asset_amount:%lld, price:%lld}",
                        item.buyOrderId.GetHex(),item.sellOrderId.GetHex(),item.dealCoinAmount,item.dealAssetAmount,item.dealPrice);
    }

    return strprintf(
        "txType=%s, hash=%s, ver=%d, valid_height=%d, txUid=%s, llFees=%ld,"
        "deal_items=%s\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, valid_height, txUid.ToString(), llFees,
        dealInfo);
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

bool CDEXSettleTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (txUid.get<CRegID>() != SysCfg().GetDexMatchSvcRegId()) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account regId is not authorized dex match-svc regId"),
                         REJECT_INVALID, "unauthorized-settle-account");
    }

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");
    if (txUid.type() == typeid(CRegID) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey );
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

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
bool CDEXSettleTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    vector<CReceipt> receipts;

    CAccount srcAcct;
   if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!srcAcct.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    for (auto dealItem : dealItems) {
        //1. get and check buyDealOrder and sellDealOrder
        CDEXOrderDetail buyOrder, sellOrder;
        if (!GetDealOrder(cw, state, dealItem.buyOrderId, ORDER_BUY, buyOrder)) return false;
        if (!GetDealOrder(cw, state, dealItem.sellOrderId, ORDER_SELL, sellOrder)) return false;

        // 2. get account of order
        CAccount buyOrderAccount;
        if (!cw.accountCache.GetAccount(buyOrder.user_regid, buyOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read buy order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccount sellOrderAccount;
        if (!cw.accountCache.GetAccount(sellOrder.user_regid, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read sell order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        // 3. check coin type match
        if (buyOrder.coin_symbol != sellOrder.coin_symbol) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, coin type not match"),
                            REJECT_INVALID, "bad-order-match");
        }
        // 4. check asset type match
        if (buyOrder.asset_symbol != sellOrder.asset_symbol) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, asset type not match"),
                            REJECT_INVALID, "bad-order-match");
        }

        // 5. check price match
        if (buyOrder.order_type == ORDER_LIMIT_PRICE && sellOrder.order_type == ORDER_LIMIT_PRICE) {
            if ( buyOrder.price < dealItem.dealPrice
                || sellOrder.price > dealItem.dealPrice ) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatched");
            }
        } else if (buyOrder.order_type == ORDER_LIMIT_PRICE && sellOrder.order_type == ORDER_MARKET_PRICE) {
            if (dealItem.dealPrice != buyOrder.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatched");
            }
        } else if (buyOrder.order_type == ORDER_MARKET_PRICE && sellOrder.order_type == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice != sellOrder.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatched");
            }
        } else {
            assert(buyOrder.order_type == ORDER_MARKET_PRICE && sellOrder.order_type == ORDER_MARKET_PRICE);
            // no limit
        }

        // 6. check and operate deal amount
        uint64_t calcCoinAmount = CDEXOrderBaseTx::CalcCoinAmount(dealItem.dealAssetAmount, dealItem.dealPrice);
        if (calcCoinAmount != dealItem.dealCoinAmount) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the dealCoinAmount not match"),
                            REJECT_INVALID, "deal-coin-amount-unmatch");
        }
        buyOrder.total_deal_coin_amount += dealItem.dealCoinAmount;
        buyOrder.total_deal_asset_amount += dealItem.dealAssetAmount;
        sellOrder.total_deal_coin_amount += dealItem.dealCoinAmount;
        sellOrder.total_deal_asset_amount += dealItem.dealAssetAmount;

        // 7. check the order limit amount and get residual amount
        uint64_t buyResidualAmount = 0;
        uint64_t sellResidualAmount = 0;

        if (buyOrder.order_type == ORDER_MARKET_PRICE) {
            uint64_t limitCoinAmount = buyOrder.coin_amount;
            if (limitCoinAmount < buyOrder.total_deal_coin_amount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal coin amount exceed the limit coin amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitCoinAmount - buyOrder.total_deal_coin_amount;
        } else {
            uint64_t limitAssetAmount = buyOrder.asset_amount;
            if (limitAssetAmount < buyOrder.total_deal_asset_amount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitAssetAmount - buyOrder.total_deal_asset_amount;
        }

        {
            // get and check sell order residualAmount
            uint64_t limitAssetAmount = sellOrder.asset_amount;
            if (limitAssetAmount < sellOrder.total_deal_asset_amount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            sellResidualAmount = limitAssetAmount - sellOrder.total_deal_asset_amount;
        }

        // 8. subtract the buyer's coins and seller's assets
        // - unfree and subtract the coins from buyer account
        if (   !buyOrderAccount.OperateBalance(buyOrder.coin_symbol, UNFREEZE, dealItem.dealCoinAmount)
            || !buyOrderAccount.OperateBalance(buyOrder.coin_symbol, SUB_FREE, dealItem.dealCoinAmount)) {// - subtract buyer's coins
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, subtract coins from buyer account failed"),
                    REJECT_INVALID, "operate-account-failed");
        }
        // - unfree and subtract the assets from seller account
        if (   !sellOrderAccount.OperateBalance(sellOrder.asset_symbol, UNFREEZE, dealItem.dealAssetAmount)
            || !sellOrderAccount.OperateBalance(sellOrder.asset_symbol, SUB_FREE, dealItem.dealAssetAmount)) { // - subtract seller's assets
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, subtract coins from seller account failed"),
                            REJECT_INVALID, "operate-account-failed");
        }

        // 9. calc deal fees
        uint64_t dexDealFeeRatio;
        if (!cw.sysParamCache.GetParam(DEX_DEAL_FEE_RATIO, dexDealFeeRatio)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read DEX_DEAL_FEE_RATIO error"),
                                READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
        }
        uint64_t buyerReceivedAssets = dealItem.dealAssetAmount;
        // 9.1 buyer pay the fee from the received assets to settler
        if (buyOrder.generate_type == USER_GEN_ORDER) {

            uint64_t dealAssetFee = dealItem.dealAssetAmount * dexDealFeeRatio / kPercentBoost;
            buyerReceivedAssets = dealItem.dealAssetAmount - dealAssetFee;
            // give the fee to settler
            srcAcct.OperateBalance(buyOrder.asset_symbol, ADD_FREE, dealAssetFee);

            CReceipt receipt(buyOrderAccount.regid, srcAcct.regid, buyOrder.asset_symbol, dealAssetFee,
                "deal asset fee to settler");
            receipts.push_back(receipt);
        }
        // 9.2 seller pay the fee from the received coins to settler
        uint64_t sellerReceivedCoins = dealItem.dealCoinAmount;
        if (sellOrder.generate_type == USER_GEN_ORDER) {
            uint64_t dealCoinFee = dealItem.dealCoinAmount * dexDealFeeRatio / kPercentBoost;
            sellerReceivedCoins = dealItem.dealCoinAmount - dealCoinFee;
            // give the buyer fee to settler
            srcAcct.OperateBalance(buyOrder.coin_symbol, ADD_FREE, dealCoinFee);
            CReceipt receipt(buyOrderAccount.regid, srcAcct.regid, buyOrder.coin_symbol, dealCoinFee,
                "deal coin fee to settler");
            receipts.push_back(receipt);
        }

        // 10. add the buyer's assets and seller's coins
        if (   !buyOrderAccount.OperateBalance(buyOrder.asset_symbol, ADD_FREE, buyerReceivedAssets)    // + add buyer's assets
            || !sellOrderAccount.OperateBalance(sellOrder.coin_symbol, ADD_FREE, sellerReceivedCoins)){ // + add seller's coin
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, add assets to buyer or add coins to seller failed"),
                            REJECT_INVALID, "operate-account-failed");
        }
        CReceipt buyReceipt(sellOrderAccount.regid, buyOrderAccount.regid, buyOrder.asset_symbol, buyerReceivedAssets,
            "deal assets to buyer");
        receipts.push_back(buyReceipt);
        CReceipt sellReceipt(buyOrderAccount.regid, sellOrderAccount.regid, buyOrder.coin_symbol, sellerReceivedCoins,
            "deal coins to settler");
        receipts.push_back(sellReceipt);

        // 11. check order fullfiled or save residual amount
        if (buyResidualAmount == 0) { // buy order fulfilled
            if (buyOrder.order_type == ORDER_LIMIT_PRICE) {
                if (buyOrder.coin_amount > buyOrder.total_deal_coin_amount) {
                    uint64_t residualCoinAmount = buyOrder.coin_amount - buyOrder.total_deal_coin_amount;

                    buyOrderAccount.OperateBalance(SYMB::WUSD, UNFREEZE, residualCoinAmount);
                } else {
                    assert(buyOrder.coin_amount == buyOrder.total_deal_coin_amount);
                }
            }
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.buyOrderId, buyOrder)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.UpdateActiveOrder(dealItem.buyOrderId, buyOrder)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (sellResidualAmount == 0) { // sell order fulfilled
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.sellOrderId, sellOrder)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.UpdateActiveOrder(dealItem.sellOrderId, sellOrder)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (!cw.accountCache.SetAccount(buyOrder.user_regid, buyOrderAccount)
            || !cw.accountCache.SetAccount(sellOrder.user_regid, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, write account info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }

    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    if(!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
    return true;
}

bool CDEXSettleTx::GetDealOrder(CCacheWrapper &cw, CValidationState &state, const uint256 &txid,
                                const OrderSide orderSide, CDEXOrderDetail &dealOrder) {
    if (!cw.dexCache.GetActiveOrder(txid, dealOrder))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::GetDealOrder, get active order failed! txid=%s", txid.ToString()),
                        REJECT_INVALID, "get-active-order-failed");
    if (dealOrder.order_side != orderSide)
        return state.DoS(100, ERRORMSG("CDEXSettleTx::GetDealOrder, expected order_side=%s, "
            "but get order_side=%s! txid=%s", orderSide, dealOrder.order_side, txid.ToString()),
                        REJECT_INVALID, "order-side-unmatched");

    return true;
}