// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"

#include "configuration.h"
#include "main.h"

using uint128_t = unsigned __int128;

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

bool CDEXOrderBaseTx::CalcCoinAmount(uint64_t assetAmount, uint64_t price, uint64_t &coinAmountOut) {
    uint128_t coinAmount = assetAmount * (uint128_t)price / COIN;
    if (coinAmount > ULLONG_MAX) return false;
    coinAmountOut = coinAmount;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

string CDEXBuyLimitOrderTx::ToString(CAccountDBCache &view) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "coin_type=%u, asset_type=%u, amount=%lld, price=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        coinType, assetType, assetAmount, bidPrice);
}

Object CDEXBuyLimitOrderTx::ToJson(const CAccountDBCache &view) const {
    Object result;

    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("coin_type",      coinType));
    result.push_back(Pair("asset_type",     assetType));
    result.push_back(Pair("amount",         assetAmount));
    result.push_back(Pair("price",          bidPrice));
    
    return result;
}

bool CDEXBuyLimitOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    if (COINT_TYPE_SET.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (COINT_TYPE_SET.count(assetType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, invalid assetType"), REJECT_INVALID,
                         "bad-assetType");
    }

    if (coinType == assetType) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, coinType can not equal to assetType"), REJECT_INVALID,
                         "bad-coinType-assetType");
    }
    // TODO: check asset amount range? min asset amount limit?
    // TODO: check bidPrice range?

    uint64_t coinAmount = 0;
    if (!CalcCoinAmount(assetAmount, bidPrice, coinAmount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, calculated coin amount out of range"), REJECT_INVALID,
                         "coins-out-range");
    }

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyLimitOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's coin for buying the asset
    uint64_t coinAmount = 0;
    CalcCoinAmount(assetAmount, bidPrice, coinAmount);
    if (!srcAcct.FreezeDexCoin(coinType, coinAmount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveOrder activeBuyOrder;
    activeBuyOrder.generateType = USER_GEN_ORDER;
    activeBuyOrder.totalDealCoinAmount = 0;
    activeBuyOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, activeBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, set active buy order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txid = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXBuyLimitOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {
    if (!UndoTxAddresses(cw, state)) return false;

    if (!cw.dexCache.UndoActiveOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, undo active buy order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyId;
    if (!cw.accountCache.GetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, read account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    if (!account.UndoOperateAccount(accountLog)) {
        return state.DoS(100,
                         ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, undo operate account failed"),
                         UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, write account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}

void CDEXBuyLimitOrderTx::GetOrderData(CDEXOrderData &orderData) {
    assert(txUid.type() == typeid(CRegID));
    orderData.userRegId = txUid.get<CRegID>();
    orderData.orderType = ORDER_LIMIT_PRICE;     //!< order type
    orderData.direction = ORDER_BUY;
    orderData.coinType = coinType;      //!< coin type
    orderData.assetType = assetType;     //!< asset type
    orderData.coinAmount = bidPrice * assetAmount;    //!< amount of coin to buy asset
    orderData.assetAmount = assetAmount;   //!< amount of asset to buy/sell
    orderData.price = bidPrice;         //!< price in coinType want to buy asset
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellLimitOrderTx

string CDEXSellLimitOrderTx::ToString(CAccountDBCache &view) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "coin_type=%u, asset_type=%u, amount=%lld, price=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        coinType, assetType, assetAmount, askPrice);
}

Object CDEXSellLimitOrderTx::ToJson(const CAccountDBCache &view) const {
    Object result;

    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("coin_type",      coinType));
    result.push_back(Pair("asset_type",     assetType));
    result.push_back(Pair("amount",         assetAmount));
    result.push_back(Pair("price",          askPrice));
    
    return result;
}

bool CDEXSellLimitOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (COINT_TYPE_SET.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (COINT_TYPE_SET.count(assetType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, invalid assetType"), REJECT_INVALID,
                         "bad-assetType");
    }

    if (coinType == assetType) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, coinType can not equal to assetType"), REJECT_INVALID,
                         "bad-coinType-assetType");
    }
    // TODO: check asset amount range? min asset amount limit?
    // TODO: check bidPrice range?

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    IMPLEMENT_CHECK_TX_SIGNATURE(srcAccount.pubKey);

    return true;
}

bool CDEXSellLimitOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // freeze user's asset for selling.
    if (!srcAcct.FreezeDexAsset(assetType, assetAmount)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveOrder activeSellOrder;
    activeSellOrder.generateType = USER_GEN_ORDER;
    activeSellOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, activeSellOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, create active sell order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txid = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSellLimitOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    if (!cw.dexCache.UndoActiveOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, undo active sell order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyId;
    if (!cw.accountCache.GetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!account.UndoOperateAccount(accountLog)) {
        return state.DoS(100,
                         ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, undo operate account failed"),
                         UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, write account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}

void CDEXSellLimitOrderTx::GetOrderData(CDEXOrderData &orderData) {
    assert(txUid.type() == typeid(CRegID));
    orderData.userRegId = txUid.get<CRegID>();
    orderData.orderType = ORDER_LIMIT_PRICE;     //!< order type
    orderData.direction = ORDER_SELL;
    orderData.coinType = coinType;      //!< coin type
    orderData.assetType = assetType;     //!< asset type
    orderData.coinAmount = askPrice * assetAmount;    //!< amount of coin to buy asset
    orderData.assetAmount = assetAmount;   //!< amount of asset to buy/sell
    orderData.price = askPrice;         //!< price in coinType want to buy asset
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyMarketOrderTx

string CDEXBuyMarketOrderTx::ToString(CAccountDBCache &view) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "coin_type=%u, asset_type=%u, amount=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        coinType, assetType, coinAmount);
}

Object CDEXBuyMarketOrderTx::ToJson(const CAccountDBCache &view) const {
    Object result;

    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("coin_type",      coinType));
    result.push_back(Pair("asset_type",     assetType));
    result.push_back(Pair("amount",         coinAmount));
    
    return result;
}

bool CDEXBuyMarketOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    if (COINT_TYPE_SET.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (COINT_TYPE_SET.count(assetType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, invalid assetType"), REJECT_INVALID,
                         "bad-assetType");
    }

    if (coinType == assetType) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, coinType can not equal to assetType"), REJECT_INVALID,
                         "bad-coinType-assetType");
    }
    // TODO: check coin amount range? min coin amount limit?
    // TODO: check bidPrice range?

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyMarketOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's coin for buying the asset
    if (!srcAcct.FreezeDexCoin(coinType, coinAmount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveOrder activeBuyOrder;
    activeBuyOrder.generateType = USER_GEN_ORDER;
    activeBuyOrder.totalDealCoinAmount = 0;
    activeBuyOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, activeBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, create active buy order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txid = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXBuyMarketOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    if (!cw.dexCache.UndoActiveOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, undo active buy order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyId;
    if (!cw.accountCache.GetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, read account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!account.UndoOperateAccount(accountLog)) {
        return state.DoS(100,
                         ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, undo operate account failed"),
                         UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, write account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}

void CDEXBuyMarketOrderTx::GetOrderData(CDEXOrderData &orderData) {
    assert(txUid.type() == typeid(CRegID));
    orderData.userRegId = txUid.get<CRegID>();
    orderData.orderType = ORDER_MARKET_PRICE;     //!< order type
    orderData.direction = ORDER_BUY;
    orderData.coinType = coinType;      //!< coin type
    orderData.assetType = assetType;     //!< asset type
    orderData.coinAmount = coinAmount;    //!< amount of coin to buy asset
    orderData.assetAmount = 0;          //!< unknown assetAmount in order
    orderData.price = 0;                //!< unknown price in order
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellMarketOrderTx

string CDEXSellMarketOrderTx::ToString(CAccountDBCache &view) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "coin_type=%u, asset_type=%u, amount=%lld\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        coinType, assetType, assetAmount);
}

Object CDEXSellMarketOrderTx::ToJson(const CAccountDBCache &view) const {
    Object result;

    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("coin_type",      coinType));
    result.push_back(Pair("asset_type",     assetType));
    result.push_back(Pair("amount",         assetAmount));
    
    return result;
}

bool CDEXSellMarketOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    if (COINT_TYPE_SET.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (COINT_TYPE_SET.count(assetType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, invalid assetType"), REJECT_INVALID,
                         "bad-assetType");
    }

    if (coinType == assetType) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, coinType can not equal to assetType"), REJECT_INVALID,
                         "bad-coinType-assetType");
    }
    // TODO: check asset amount range? min asset amount limit?
    // TODO: check bidPrice range?

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXSellMarketOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    CAccount srcAcct;
    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    // should freeze user's asset for selling
    if (!srcAcct.FreezeDexAsset(assetType, assetAmount)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveOrder activeSellOrder;
    activeSellOrder.generateType = USER_GEN_ORDER;
    activeSellOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, activeSellOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, create active sell order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txid = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSellMarketOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    if (!cw.dexCache.UndoActiveOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::UndoExecuteTx, undo active sell order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyId;
    if (!cw.accountCache.GetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::UndoExecuteTx, read account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    if (!account.UndoOperateAccount(accountLog)) {
        return state.DoS(100,
                         ERRORMSG("CDEXSellMarketOrderTx::UndoExecuteTx, undo operate account failed"),
                         UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::UndoExecuteTx, write account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}

void CDEXSellMarketOrderTx::GetOrderData(CDEXOrderData &orderData) {
    assert(txUid.type() == typeid(CRegID));
    orderData.userRegId = txUid.get<CRegID>();
    orderData.orderType = ORDER_MARKET_PRICE;     //!< order type
    orderData.direction = ORDER_SELL;
    orderData.coinType = coinType;          //!< coin type
    orderData.assetType = assetType;        //!< asset type
    orderData.coinAmount = 0;               //!< unknown coinAmount in order
    orderData.assetAmount = assetAmount;    //!< asset amount want to sell
    orderData.price = 0;                    //!< unknown price in order
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXCancelOrderTx

string CDEXCancelOrderTx::ToString(CAccountDBCache &view) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "orderId=%s\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        orderId.GetHex());
}

Object CDEXCancelOrderTx::ToJson(const CAccountDBCache &view) const {
    Object result;

    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("order_id",       orderId.GetHex()));
    
    return result;
}

bool CDEXCancelOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXCancelOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog srcAcctLog(srcAccount);
    CAccountLog desAcctLog;
    if (!srcAccount.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    CDEXActiveOrder activeOrder;
    if (!cw.dexCache.GetActiveOrder(orderId, activeOrder)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, the order is inactive or not existed"),
                        REJECT_INVALID, "order-inactive");
    }
    if (activeOrder.generateType != USER_GEN_ORDER) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, the order is not generate by tx of user"),
                        REJECT_INVALID, "order-inactive");
    }

    CDEXOrderData orderData;
    shared_ptr<CDEXOrderBaseTx> pOrderTx;
    if(!ReadTxFromDisk(activeOrder.txCord, pOrderTx)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, read order tx by tx cord failed"),
                        REJECT_INVALID, "bad-read-tx");
    }
    pOrderTx->GetOrderData(orderData);
    assert(txUid.type() == typeid(CRegID));

    if (txUid.get<CRegID>() != orderData.userRegId) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, can not cancel other user's order tx"),
                        REJECT_INVALID, "user-unmatch");
    }

    CoinType frozenType;
    uint64_t frozenAmount = 0;
    if (orderData.direction == ORDER_BUY) {
        frozenType = orderData.coinType;
        frozenAmount = orderData.coinAmount - activeOrder.totalDealCoinAmount;
    } else {
        assert(orderData.direction == ORDER_SELL);
        frozenType = orderData.assetType;
        frozenAmount = orderData.assetAmount - activeOrder.totalDealAssetAmount;
    }

    if (!srcAccount.UnFreezeDexCoin(frozenType, frozenAmount)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, account has insufficient frozen amount to unfreeze"),
                         UPDATE_ACCOUNT_FAIL, "unfreeze-account-coin");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyId), srcAccount))
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txid = GetHash();
    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXCancelOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    if (!cw.dexCache.UndoActiveOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::UndoExecuteTx, undo active sell order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    static const int ACCOUNT_LOG_SIZE = 2;

    if (cw.txUndo.accountLogs.size() != ACCOUNT_LOG_SIZE) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::UndoExecuteTx, accountLogs size must be %d", ACCOUNT_LOG_SIZE),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }

    for (auto accountLog : cw.txUndo.accountLogs) {
        CAccount account;
        CUserID userId = accountLog.keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::UndoExecuteTx, read account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(accountLog)) {
            return state.DoS(100,
                            ERRORMSG("CDEXCancelOrderTx::UndoExecuteTx, undo operate account failed"),
                            UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::UndoExecuteTx, write account info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CDEXSettleTx
string CDEXSettleTx::ToString(CAccountDBCache &view) {
    string dealInfo;
    for (const auto &item : dealItems) {
        dealInfo += strprintf("{buy_order_id:%s, sell_order_id:%s, coin_amount:%lld, asset_amount:%lld, price:%lld}",
                        item.buyOrderId.GetHex(),item.sellOrderId.GetHex(),item.dealCoinAmount,item.dealAssetAmount,item.dealPrice);
    }

    return strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "deal_items=%s\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        dealInfo);
}

Object CDEXSettleTx::ToJson(const CAccountDBCache &view) const {
    Object result;

    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

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

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("deal_items",     arrayItems));
    
    return result;
}

bool CDEXSettleTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
}

bool CDEXSettleTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (txUid.get<CRegID>() != kDEXSettleRegId) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account regId is not authorized dex settle regId"),
                         REJECT_INVALID, "unauthorized-settle-account");
    }

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");
    if (!srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");


    IMPLEMENT_CHECK_TX_SIGNATURE(srcAccount.pubKey);

    return true;
}


/* process flow for settle tx
1. get and check active order
    a. get and check activeBuyOrder
    b. get and check activeSellOrder
2. get order data:
    a. if order is USER_GEN_ORDER:
        step 1. get and check order tx object from block store
        step 2. get order data from tx object
    b. if order is SYS_GEN_ORDER:
        step 1. get sys order object from dex db
        then get order detail from sys order object
3. get account of order's owner
    a. get buyerAccount from account db
    b. get sellerAccount from account db
4. check coin type match
    buyOrder.coinType == sellOrder.coinType
5. check asset type match
    buyOrder.assetType == sellOrder.assetType
6. check price is right in valid height block
7. check price match
    a. limit type <-> limit type
        I.   dealPrice <= buyOrder.price
        II.  dealPrice >= sellOrder.price
    b. limit type <-> market type
        I.   dealPrice == buyOrder.price
    c. market type <-> limit type
        I.   dealPrice == sellOrder.price
    d. market type <-> market type
        no limit
8. check and operate deal amount
    a. check: dealCoinAmount == CalcCoinAmount(dealAssetAmount, price)
    b. else check: (dealCoinAmount / 10000) == (CalcCoinAmount(dealAssetAmount, price) / 10000)
    c. operate total deal:
        activeBuyOrder.totalDealCoinAmount  += dealCoinAmount
        activeBuyOrder.totalDealAssetAmount += dealAssetAmount
        activeSellOrder.totalDealCoinAmount  += dealCoinAmount
        activeSellOrder.totalDealAssetAmount += dealAssetAmount
9. check the order limit amount and get residual amount
    a. buy order
        if market price order {
            limitCoinAmount = buyOrder.coinAmount
            check: limitCoinAmount >= activeBuyOrder.totalDealCoinAmount
            residualAmount = limitCoinAmount - activeBuyOrder.totalDealCoinAmount
        } else { //limit price order
            limitAssetAmount = buyOrder.assetAmount
            check: limitAssetAmount >= activeBuyOrder.totalDealAssetAmount
            residualAmount = limitAssetAmount - activeBuyOrder.totalDealAssetAmount
        }
    b. sell order
        limitAssetAmount = sellOrder.limitAmount
        check: limitAssetAmount >= activeSellOrder.totalDealAssetAmount
        residualAmount = limitAssetAmount - dealCoinAmount
10. calc deal fees
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
11. operate account
    a. buyerFrozenCoins     -= dealCoinAmount
    b. buyerAssets          += dealAssetAmount - dealAssetFee
    c. sellerCoins          += dealCoinAmount - dealCoinFee
    d. sellerFrozenAssets   -= dealAssetAmount
12. check order fulfiled or save residual amount
    a. buy order
        if buy order is fulfilled {
            if buy limit order {
                residualCoinAmount = buyOrder.coinAmount - activeBuyOrder.totalDealCoinAmount
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
bool CDEXSettleTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
   if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    cw.txUndo.accountLogs.push_back(CAccountLog(srcAcct));

    for (auto dealItem : dealItems) {
        // 1. get and check active order
        CDEXActiveOrder activeBuyOrder;
        if (!cw.dexCache.GetActiveOrder(dealItem.buyOrderId, activeBuyOrder)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, get buy order by tx cord failed"),
                            REJECT_INVALID, "bad-get-buy-order");
        }
        CDEXActiveOrder activeSellOrder;
        if (!cw.dexCache.GetActiveOrder(dealItem.sellOrderId, activeSellOrder)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, get sell order by tx cord failed"),
                            REJECT_INVALID, "bad-get-sell-order");
        }
        // 2. get order detail:
        CDEXOrderData buyOrderData;
        if (activeBuyOrder.generateType == USER_GEN_ORDER) {
            shared_ptr<CDEXOrderBaseTx> pBuyOrderTx;
            if(!ReadTxFromDisk(activeBuyOrder.txCord, pBuyOrderTx)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read buy order tx by tx cord failed"),
                                REJECT_INVALID, "bad-read-tx");
            }
            assert(dealItem.buyOrderId == pBuyOrderTx->GetHash());
            pBuyOrderTx->GetOrderData(buyOrderData);
            if (buyOrderData.direction != ORDER_BUY) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the buy order tx is not buy direction"),
                                REJECT_INVALID, "bad-read-tx");
            }
        } else {
            assert(activeBuyOrder.generateType == SYSTEM_GEN_ORDER);
            CDEXSysBuyOrder sysBuyOrder;
            if (!cw.dexCache.GetSysBuyOrder(dealItem.buyOrderId, sysBuyOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, get sys buy order from db failed"),
                                REJECT_INVALID, "bad-read-tx");
            }
            sysBuyOrder.GetOrderData(buyOrderData);
        }
        CDEXOrderData sellOrderData;
        if (activeSellOrder.generateType == USER_GEN_ORDER) {
            shared_ptr<CDEXSellLimitOrderTx> pSellOrderTx;
            if(!ReadTxFromDisk(activeSellOrder.txCord, pSellOrderTx)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, read sell order tx by tx cord failed"),
                                REJECT_INVALID, "bad-read-tx");
            }
            assert(dealItem.buyOrderId == pSellOrderTx->GetHash());
            pSellOrderTx->GetOrderData(sellOrderData);
            if (sellOrderData.direction != ORDER_SELL) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the sell order tx is not sell direction"),
                                REJECT_INVALID, "bad-read-tx");
            }
        } else {
            assert(activeSellOrder.generateType == SYSTEM_GEN_ORDER);
            CDEXSysSellOrder sysSellOrder;
            if (!cw.dexCache.GetSysSellOrder(dealItem.sellOrderId, sysSellOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, get sys sell order from db failed"),
                                REJECT_INVALID, "bad-read-tx");
            }
            sysSellOrder.GetOrderData(sellOrderData);
        }

        // 3. get account of order
        CAccount buyOrderAccount;
        if (!cw.accountCache.GetAccount(buyOrderData.userRegId, buyOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read buy order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(buyOrderAccount));
        CAccount sellOrderAccount;
        if (!cw.accountCache.GetAccount(sellOrderData.userRegId, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read sell order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(sellOrderAccount));

        // 4. check coin type match
        if (buyOrderData.coinType != sellOrderData.coinType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, coin type not match"),
                            REJECT_INVALID, "bad-order-match");
        }
        // 5. check asset type match
        if (buyOrderData.assetType != sellOrderData.assetType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, asset type not match"),
                            REJECT_INVALID, "bad-order-match");
        }

        // 6. check price is right in valid height block
        // TODO: ...

        // 7. check price match
        if (buyOrderData.orderType == ORDER_LIMIT_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE) {
            if ( buyOrderData.price < dealItem.dealPrice
                || sellOrderData.price > dealItem.dealPrice ) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrderData.orderType == ORDER_LIMIT_PRICE && sellOrderData.orderType == ORDER_MARKET_PRICE) {
            if (dealItem.dealPrice != buyOrderData.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrderData.orderType == ORDER_MARKET_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice != sellOrderData.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatch");
            }
        } else {
            assert(buyOrderData.orderType == ORDER_MARKET_PRICE && sellOrderData.orderType == ORDER_MARKET_PRICE);
            // no limit
        }

        // 8. check and operate deal amount
        uint64_t calcCoinAmount = 0;
        if (!CDEXOrderBaseTx::CalcCoinAmount(dealItem.dealAssetAmount, dealItem.dealPrice, calcCoinAmount)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the calculated coin amount out of range"),
                            REJECT_INVALID, "coins-out-range");
        }
        if ((calcCoinAmount / kPercentBoost) != (dealItem.dealCoinAmount / kPercentBoost)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the dealCoinAmount not match"),
                            REJECT_INVALID, "deal-coin-amount-unmatch");
        }
        activeBuyOrder.totalDealCoinAmount += dealItem.dealCoinAmount;
        activeBuyOrder.totalDealAssetAmount += dealItem.dealAssetAmount;
        activeSellOrder.totalDealCoinAmount += dealItem.dealCoinAmount;
        activeSellOrder.totalDealAssetAmount += dealItem.dealAssetAmount;

        //9. check the order limit amount and get residual amount
        uint64_t buyResidualAmount = 0;
        uint64_t sellResidualAmount = 0;

        if (buyOrderData.orderType == ORDER_MARKET_PRICE) {
            uint64_t limitCoinAmount = buyOrderData.coinAmount;
            if (limitCoinAmount < activeBuyOrder.totalDealCoinAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal coin amount exceed the limit coin amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitCoinAmount - activeBuyOrder.totalDealCoinAmount;
        } else {
            uint64_t limitAssetAmount = buyOrderData.assetAmount;
            if (limitAssetAmount < activeBuyOrder.totalDealAssetAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitAssetAmount - activeBuyOrder.totalDealAssetAmount;
        }

        { // get and check sell order residualAmount
            uint64_t limitAssetAmount = sellOrderData.assetAmount;
            if (limitAssetAmount < activeSellOrder.totalDealAssetAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            sellResidualAmount = limitAssetAmount - activeSellOrder.totalDealAssetAmount;
        }

        // 10. calc deal fees
        uint64_t buyerReceivedAssets = dealItem.dealAssetAmount;
        uint64_t sellerReceivedCoins = dealItem.dealCoinAmount;
        if (activeBuyOrder.generateType == USER_GEN_ORDER) {
            uint64_t dealAssetFee = dealItem.dealAssetAmount * kDefaultDexDealFeeRatio / kPercentBoost;
            buyerReceivedAssets = dealItem.dealAssetAmount - dealAssetFee;
            // TODO: add dealAssetFee and dealAssetFee to current tx feeMap
        }
        if (activeSellOrder.generateType == USER_GEN_ORDER) {
            uint64_t dealCoinFee = dealItem.dealCoinAmount * kDefaultDexDealFeeRatio / kPercentBoost;
            sellerReceivedCoins = dealItem.dealCoinAmount - dealCoinFee;
            // TODO: add dealCoinFee and dealAssetFee to current tx feeMap
        }

        // 11. operate account

        if (!buyOrderAccount.MinusDEXFrozenCoin(buyOrderData.coinType, dealItem.dealCoinAmount)             // - minus buyer's coins
            || !buyOrderAccount.OperateBalance(buyOrderData.assetType, ADD_VALUE, buyerReceivedAssets)      // + add buyer's assets
            || !sellOrderAccount.OperateBalance(sellOrderData.coinType, ADD_VALUE, sellerReceivedCoins)     // + add seller's coin
            || !sellOrderAccount.MinusDEXFrozenCoin(sellOrderData.assetType, dealItem.dealAssetAmount)) {   // - minus seller's assets
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, operate coins or assets failed"),
                            REJECT_INVALID, "operate-acount-failed");
        }

        // 12. check order fulfiled or save residual amount
        if (buyResidualAmount == 0) { // buy order fulfilled
            if (buyOrderData.orderType == ORDER_LIMIT_PRICE) {
                if (buyOrderData.coinAmount > activeBuyOrder.totalDealCoinAmount) {
                    uint64_t residualCoinAmount = buyOrderData.coinAmount - activeBuyOrder.totalDealCoinAmount;
                    buyOrderAccount.UnFreezeDexCoin(buyOrderData.coinType, residualCoinAmount);
                } else {
                    assert(buyOrderData.coinAmount == activeBuyOrder.totalDealCoinAmount);
                }
            }
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.buyOrderId, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.ModifyActiveOrder(dealItem.buyOrderId, activeBuyOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (sellResidualAmount == 0) { // sell order fulfilled
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.sellOrderId, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.ModifyActiveOrder(dealItem.sellOrderId, activeSellOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (!cw.accountCache.SetAccount(buyOrderData.userRegId, buyOrderAccount)
            || !cw.accountCache.SetAccount(buyOrderData.userRegId, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, write account info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }

    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    cw.txUndo.txid = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSettleTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    if (!cw.dexCache.UndoActiveOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSettleTx::UndoExecuteTx, undo active orders for all deal item failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    auto rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        const CAccountLog &accountLog = *rIterAccountLog;
        CAccount account;
        CUserID userId = accountLog.keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::UndoExecuteTx, read account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(accountLog)) {
            return state.DoS(100,
                            ERRORMSG("CDEXSettleTx::UndoExecuteTx, undo operate account failed"),
                            UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::UndoExecuteTx, write account info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}
