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

    CKeyID srcKeyId;
    if(!view.GetKeyId(txUid, srcKeyId)) { assert(false && "GetKeyId() failed"); }

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   nValidHeight));

    result.push_back(Pair("coin_type",      GetCoinTypeName(coinType)));
    result.push_back(Pair("asset_type",     GetCoinTypeName(assetType)));
    result.push_back(Pair("asset_amount",   assetAmount));
    result.push_back(Pair("price",          bidPrice));
    return result;
}

bool CDEXBuyLimitOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    if (kCoinTypeMapName.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (kCoinTypeMapName.count(assetType) == 0) {
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

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey );
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
    CDEXActiveOrder buyActiveOrder;
    buyActiveOrder.generateType = USER_GEN_ORDER;
    buyActiveOrder.totalDealCoinAmount = 0;
    buyActiveOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, buyActiveOrder, cw.txUndo.dbOpLogMap)) {
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

void CDEXBuyLimitOrderTx::GetOrderDetail(CDEXOrderDetail &orderDetail) {
    assert(txUid.type() == typeid(CRegID));
    orderDetail.userRegId = txUid.get<CRegID>();
    orderDetail.orderType = ORDER_LIMIT_PRICE;     //!< order type
    orderDetail.direction = ORDER_BUY;
    orderDetail.coinType = coinType;      //!< coin type
    orderDetail.assetType = assetType;     //!< asset type
    orderDetail.coinAmount = bidPrice * assetAmount;    //!< amount of coin to buy asset
    orderDetail.assetAmount = assetAmount;   //!< amount of asset to buy/sell
    orderDetail.price = bidPrice;         //!< price in coinType want to buy asset
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

    CKeyID srcKeyId;
    if(!view.GetKeyId(txUid, srcKeyId)) { assert(false && "GetKeyId() failed"); }

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   nValidHeight));

    result.push_back(Pair("coin_type",      GetCoinTypeName(coinType)));
    result.push_back(Pair("asset_type",     GetCoinTypeName(assetType)));
    result.push_back(Pair("asset_amount",   assetAmount));
    result.push_back(Pair("price",          askPrice));
    return result;
}

bool CDEXSellLimitOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (kCoinTypeMapName.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (kCoinTypeMapName.count(assetType) == 0) {
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

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey );
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

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
    CDEXActiveOrder sellActiveOrder;
    sellActiveOrder.generateType = USER_GEN_ORDER;
    sellActiveOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, sellActiveOrder, cw.txUndo.dbOpLogMap)) {
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

void CDEXSellLimitOrderTx::GetOrderDetail(CDEXOrderDetail &orderDetail) {
    assert(txUid.type() == typeid(CRegID));
    orderDetail.userRegId = txUid.get<CRegID>();
    orderDetail.orderType = ORDER_LIMIT_PRICE;     //!< order type
    orderDetail.direction = ORDER_SELL;
    orderDetail.coinType = coinType;      //!< coin type
    orderDetail.assetType = assetType;     //!< asset type
    orderDetail.coinAmount = askPrice * assetAmount;    //!< amount of coin to buy asset
    orderDetail.assetAmount = assetAmount;   //!< amount of asset to buy/sell
    orderDetail.price = askPrice;         //!< price in coinType want to buy asset
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

    CKeyID srcKeyId;
    if(!view.GetKeyId(txUid, srcKeyId)) { assert(false && "GetKeyId() failed"); }

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   nValidHeight));

    result.push_back(Pair("coin_type",      GetCoinTypeName(coinType)));
    result.push_back(Pair("asset_type",     GetCoinTypeName(assetType)));
    result.push_back(Pair("coin_amount",    coinAmount));
    return result;
}

bool CDEXBuyMarketOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    if (kCoinTypeMapName.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (kCoinTypeMapName.count(assetType) == 0) {
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
    CDEXActiveOrder buyActiveOrder;
    buyActiveOrder.generateType = USER_GEN_ORDER;
    buyActiveOrder.totalDealCoinAmount = 0;
    buyActiveOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, buyActiveOrder, cw.txUndo.dbOpLogMap)) {
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

void CDEXBuyMarketOrderTx::GetOrderDetail(CDEXOrderDetail &orderDetail) {
    assert(txUid.type() == typeid(CRegID));
    orderDetail.userRegId = txUid.get<CRegID>();
    orderDetail.orderType = ORDER_MARKET_PRICE;     //!< order type
    orderDetail.direction = ORDER_BUY;
    orderDetail.coinType = coinType;      //!< coin type
    orderDetail.assetType = assetType;     //!< asset type
    orderDetail.coinAmount = coinAmount;    //!< amount of coin to buy asset
    orderDetail.assetAmount = 0;          //!< unknown assetAmount in order
    orderDetail.price = 0;                //!< unknown price in order
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

    CKeyID srcKeyId;
    if(!view.GetKeyId(txUid, srcKeyId)) { assert(false && "GetKeyId() failed"); }

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   nValidHeight));

    result.push_back(Pair("coin_type",      GetCoinTypeName(coinType)));
    result.push_back(Pair("asset_type",     GetCoinTypeName(assetType)));
    result.push_back(Pair("asset_amount",   assetAmount));
    return result;
}

bool CDEXSellMarketOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    if (kCoinTypeMapName.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (kCoinTypeMapName.count(assetType) == 0) {
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
    CDEXActiveOrder sellActiveOrder;
    sellActiveOrder.generateType = USER_GEN_ORDER;
    sellActiveOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.CreateActiveOrder(txHash, sellActiveOrder, cw.txUndo.dbOpLogMap)) {
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

void CDEXSellMarketOrderTx::GetOrderDetail(CDEXOrderDetail &orderDetail) {
    assert(txUid.type() == typeid(CRegID));
    orderDetail.userRegId = txUid.get<CRegID>();
    orderDetail.orderType = ORDER_MARKET_PRICE;     //!< order type
    orderDetail.direction = ORDER_SELL;
    orderDetail.coinType = coinType;          //!< coin type
    orderDetail.assetType = assetType;        //!< asset type
    orderDetail.coinAmount = 0;               //!< unknown coinAmount in order
    orderDetail.assetAmount = assetAmount;    //!< asset amount want to sell
    orderDetail.price = 0;                    //!< unknown price in order
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
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

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

    CDEXOrderDetail orderDetail;
    shared_ptr<CDEXOrderBaseTx> pOrderTx;
    if(!ReadTxFromDisk(activeOrder.txCord, pOrderTx)) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, read order tx by tx cord failed"),
                        REJECT_INVALID, "bad-read-tx");
    }
    pOrderTx->GetOrderDetail(orderDetail);
    assert(txUid.type() == typeid(CRegID));

    if (txUid.get<CRegID>() != orderDetail.userRegId) {
        return state.DoS(100, ERRORMSG("CDEXCancelOrderTx::ExecuteTx, can not cancel other user's order tx"),
                        REJECT_INVALID, "user-unmatch");
    }

    CoinType frozenType;
    uint64_t frozenAmount = 0;
    if (orderDetail.direction == ORDER_BUY) {
        frozenType = orderDetail.coinType;
        frozenAmount = orderDetail.coinAmount - activeOrder.totalDealCoinAmount;
    } else {
        assert(orderDetail.direction == ORDER_SELL);
        frozenType = orderDetail.assetType;
        frozenAmount = orderDetail.assetAmount - activeOrder.totalDealAssetAmount;
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
// struct CDEXSettleTx
class CDEXDealOrder {
public:
    CDEXActiveOrder activeOrder;
    CDEXOrderDetail orderDetail;
};

static bool GetDealOrder(const uint256 &txid, const OrderDirection direction, CCacheWrapper &cw,
                          CDEXDealOrder &dealOrder) {
    if (!cw.dexCache.GetActiveOrder(txid, dealOrder.activeOrder)) {
        return ERRORMSG("GetDealOrder, get active failed! txid=%s", txid.ToString());
    }

    if (dealOrder.activeOrder.generateType == USER_GEN_ORDER) {
        shared_ptr<CDEXOrderBaseTx> pBuyOrderTx;
        if(!ReadTxFromDisk(dealOrder.activeOrder.txCord, pBuyOrderTx)) {
            return ERRORMSG("GetDealOrder, read order tx from disk failed! txcord=%s", dealOrder.activeOrder.txCord.ToString());
        }
        assert(txid == pBuyOrderTx->GetHash());
        pBuyOrderTx->GetOrderDetail(dealOrder.orderDetail);
    } else {
        assert(dealOrder.activeOrder.generateType == SYSTEM_GEN_ORDER);
        CDEXSysOrder sysBuyOrder;
        if (!cw.dexCache.GetSysOrder(txid, sysBuyOrder)) {
            return ERRORMSG("GetDealOrder, get sys order from db failed! txid=%s", txid.ToString());
        }
        sysBuyOrder.GetOrderDetail(dealOrder.orderDetail);
    }
    if (dealOrder.orderDetail.direction != direction) {
        return ERRORMSG("GetDealOrder, unexpected direction of order tx! direction=%s", typeid(direction).name());
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

    if (!CBaseTx::GetInvolvedKeyIds(cw, keyIds)) return false;

    for (auto dealItem : dealItems) {
        CDEXDealOrder buyDealOrder;
        if (!GetDealOrder(dealItem.buyOrderId, ORDER_BUY, cw, buyDealOrder)) {
            return ERRORMSG("CDEXSettleTx::GetInvolvedKeyIds, get buy deal order failed! txid=%s", dealItem.buyOrderId.ToString());
        }
        if (!AddInvolvedKeyIds({buyDealOrder.orderDetail.userRegId}, cw, keyIds)) return false;

        CDEXDealOrder sellDealOrder;
        if (!GetDealOrder(dealItem.sellOrderId, ORDER_SELL, cw, sellDealOrder)) {
            return ERRORMSG("CDEXSettleTx::GetInvolvedKeyIds, get sell deal order detail failed! txid=%s", dealItem.sellOrderId.ToString());
        }
        if (!AddInvolvedKeyIds({sellDealOrder.orderDetail.userRegId}, cw, keyIds)) return false;
    }

    return true;
}

bool CDEXSettleTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
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
    if (txUid.type() == typeid(CRegID) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = ( txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey );
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
    buyOrder.coinType == sellOrder.coinType
4. check asset type match
    buyOrder.assetType == sellOrder.assetType
5. check price is right in valid height block
6. check price match
    a. limit type <-> limit type
        I.   dealPrice <= buyOrder.price
        II.  dealPrice >= sellOrder.price
    b. limit type <-> market type
        I.   dealPrice == buyOrder.price
    c. market type <-> limit type
        I.   dealPrice == sellOrder.price
    d. market type <-> market type
        no limit
7. check and operate deal amount
    a. check: dealCoinAmount == CalcCoinAmount(dealAssetAmount, price)
    b. else check: (dealCoinAmount / 10000) == (CalcCoinAmount(dealAssetAmount, price) / 10000)
    c. operate total deal:
        buyActiveOrder.totalDealCoinAmount  += dealCoinAmount
        buyActiveOrder.totalDealAssetAmount += dealAssetAmount
        sellActiveOrder.totalDealCoinAmount  += dealCoinAmount
        sellActiveOrder.totalDealAssetAmount += dealAssetAmount
8. check the order limit amount and get residual amount
    a. buy order
        if market price order {
            limitCoinAmount = buyOrder.coinAmount
            check: limitCoinAmount >= buyActiveOrder.totalDealCoinAmount
            residualAmount = limitCoinAmount - buyActiveOrder.totalDealCoinAmount
        } else { //limit price order
            limitAssetAmount = buyOrder.assetAmount
            check: limitAssetAmount >= buyActiveOrder.totalDealAssetAmount
            residualAmount = limitAssetAmount - buyActiveOrder.totalDealAssetAmount
        }
    b. sell order
        limitAssetAmount = sellOrder.limitAmount
        check: limitAssetAmount >= sellActiveOrder.totalDealAssetAmount
        residualAmount = limitAssetAmount - dealCoinAmount
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
10. operate account
    a. buyerFrozenCoins     -= dealCoinAmount
    b. buyerAssets          += dealAssetAmount - dealAssetFee
    c. sellerCoins          += dealCoinAmount - dealCoinFee
    d. sellerFrozenAssets   -= dealAssetAmount
11. check order fulfiled or save residual amount
    a. buy order
        if buy order is fulfilled {
            if buy limit order {
                residualCoinAmount = buyOrder.coinAmount - buyActiveOrder.totalDealCoinAmount
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
        //1. get and check buyDealOrder and sellDealOrder
        CDEXDealOrder buyDealOrder;
        if (!GetDealOrder(dealItem.buyOrderId, ORDER_BUY, cw, buyDealOrder)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, get buy deal order failed! txid=%s",
                            dealItem.buyOrderId.ToString()), REJECT_INVALID, "get-deal-order-failed");
        }
        CDEXDealOrder sellDealOrder;
        if (!GetDealOrder(dealItem.sellOrderId, ORDER_SELL, cw, sellDealOrder)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, get sell deal order failed! txid=%s",
                            dealItem.buyOrderId.ToString()), REJECT_INVALID, "get-deal-order-failed");
        }

        CDEXActiveOrder &buyActiveOrder = buyDealOrder.activeOrder;
        CDEXOrderDetail &buyOrderDetail = buyDealOrder.orderDetail;

        CDEXActiveOrder &sellActiveOrder = sellDealOrder.activeOrder;
        CDEXOrderDetail &sellOrderDetail = sellDealOrder.orderDetail;

        // 2. get account of order
        CAccount buyOrderAccount;
        if (!cw.accountCache.GetAccount(buyOrderDetail.userRegId, buyOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read buy order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(buyOrderAccount));
        CAccount sellOrderAccount;
        if (!cw.accountCache.GetAccount(sellOrderDetail.userRegId, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read sell order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(sellOrderAccount));

        // 3. check coin type match
        if (buyOrderDetail.coinType != sellOrderDetail.coinType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, coin type not match"),
                            REJECT_INVALID, "bad-order-match");
        }
        // 4. check asset type match
        if (buyOrderDetail.assetType != sellOrderDetail.assetType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, asset type not match"),
                            REJECT_INVALID, "bad-order-match");
        }

        // 5. check price is right in valid height block
        // TODO: ...

        // 6. check price match
        if (buyOrderDetail.orderType == ORDER_LIMIT_PRICE && sellOrderDetail.orderType == ORDER_LIMIT_PRICE) {
            if ( buyOrderDetail.price < dealItem.dealPrice
                || sellOrderDetail.price > dealItem.dealPrice ) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrderDetail.orderType == ORDER_LIMIT_PRICE && sellOrderDetail.orderType == ORDER_MARKET_PRICE) {
            if (dealItem.dealPrice != buyOrderDetail.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatch");
            }
        } else if (buyOrderDetail.orderType == ORDER_MARKET_PRICE && sellOrderDetail.orderType == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice != sellOrderDetail.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the expected price not match"),
                                REJECT_INVALID, "deal-price-unmatch");
            }
        } else {
            assert(buyOrderDetail.orderType == ORDER_MARKET_PRICE && sellOrderDetail.orderType == ORDER_MARKET_PRICE);
            // no limit
        }

        // 7. check and operate deal amount
        uint64_t calcCoinAmount = 0;
        if (!CDEXOrderBaseTx::CalcCoinAmount(dealItem.dealAssetAmount, dealItem.dealPrice, calcCoinAmount)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the calculated coin amount out of range"),
                            REJECT_INVALID, "coins-out-range");
        }
        if ((calcCoinAmount / kPercentBoost) != (dealItem.dealCoinAmount / kPercentBoost)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the dealCoinAmount not match"),
                            REJECT_INVALID, "deal-coin-amount-unmatch");
        }
        buyActiveOrder.totalDealCoinAmount += dealItem.dealCoinAmount;
        buyActiveOrder.totalDealAssetAmount += dealItem.dealAssetAmount;
        sellActiveOrder.totalDealCoinAmount += dealItem.dealCoinAmount;
        sellActiveOrder.totalDealAssetAmount += dealItem.dealAssetAmount;

        //8. check the order limit amount and get residual amount
        uint64_t buyResidualAmount = 0;
        uint64_t sellResidualAmount = 0;

        if (buyOrderDetail.orderType == ORDER_MARKET_PRICE) {
            uint64_t limitCoinAmount = buyOrderDetail.coinAmount;
            if (limitCoinAmount < buyActiveOrder.totalDealCoinAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal coin amount exceed the limit coin amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitCoinAmount - buyActiveOrder.totalDealCoinAmount;
        } else {
            uint64_t limitAssetAmount = buyOrderDetail.assetAmount;
            if (limitAssetAmount < buyActiveOrder.totalDealAssetAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitAssetAmount - buyActiveOrder.totalDealAssetAmount;
        }

        { // get and check sell order residualAmount
            uint64_t limitAssetAmount = sellOrderDetail.assetAmount;
            if (limitAssetAmount < sellActiveOrder.totalDealAssetAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            sellResidualAmount = limitAssetAmount - sellActiveOrder.totalDealAssetAmount;
        }

        // 9. calc deal fees
        // 9.1) buyer spends WUSD to get assets
        uint64_t buyerReceivedAssets = dealItem.dealAssetAmount;
        if (buyActiveOrder.generateType == USER_GEN_ORDER) {
            uint64_t dealAssetFee = dealItem.dealAssetAmount * kDefaultDexDealFeeRatio / kPercentBoost;
            buyerReceivedAssets = dealItem.dealAssetAmount - dealAssetFee;
            assert (dealItem.orderDetail.coinType == WICC || dealItem.orderDetail.coinType == WGRT);
            switch (dealItem.orderDetail.assetType) {
                case WICC: srcAcct.bcoins += dealAssetFee; break;
                case WGRT: srcAcct.fcoins += dealAssetFee; break;
                default: break; //unlikely
            }
        }
        //9.2 seller sells assets to get WUSD
        uint64_t sellerReceivedCoins = dealItem.dealCoinAmount;
        if (sellActiveOrder.generateType == USER_GEN_ORDER) {
            uint64_t dealCoinFee = dealItem.dealCoinAmount * kDefaultDexDealFeeRatio / kPercentBoost;
            sellerReceivedCoins = dealItem.dealCoinAmount - dealCoinFee;
            assert (dealItem.orderDetail.coinType == WUSD);
            srcAcct.scoins += dealCoinFee;
        }

        // 10. operate account
        if (!buyOrderAccount.MinusDEXFrozenCoin(buyOrderDetail.coinType, dealItem.dealCoinAmount)             // - minus buyer's coins
            || !buyOrderAccount.OperateBalance(buyOrderDetail.assetType, ADD_VALUE, buyerReceivedAssets)      // + add buyer's assets
            || !sellOrderAccount.OperateBalance(sellOrderDetail.coinType, ADD_VALUE, sellerReceivedCoins)     // + add seller's coin
            || !sellOrderAccount.MinusDEXFrozenCoin(sellOrderDetail.assetType, dealItem.dealAssetAmount)) {   // - minus seller's assets
            return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, operate coins or assets failed"),
                            REJECT_INVALID, "operate-acount-failed");
        }

        // 11. check order fulfiled or save residual amount
        if (buyResidualAmount == 0) { // buy order fulfilled
            if (buyOrderDetail.orderType == ORDER_LIMIT_PRICE) {
                if (buyOrderDetail.coinAmount > buyActiveOrder.totalDealCoinAmount) {
                    uint64_t residualCoinAmount = buyOrderDetail.coinAmount - buyActiveOrder.totalDealCoinAmount;
                    buyOrderAccount.UnFreezeDexCoin(buyOrderDetail.coinType, residualCoinAmount);
                } else {
                    assert(buyOrderDetail.coinAmount == buyActiveOrder.totalDealCoinAmount);
                }
            }
            // erase active order
            if (!cw.dexCache.EraseActiveOrder(dealItem.buyOrderId, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.ModifyActiveOrder(dealItem.buyOrderId, buyActiveOrder, cw.txUndo.dbOpLogMap)) {
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
            if (!cw.dexCache.ModifyActiveOrder(dealItem.sellOrderId, sellActiveOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        }

        if (!cw.accountCache.SetAccount(buyOrderDetail.userRegId, buyOrderAccount)
            || !cw.accountCache.SetAccount(buyOrderDetail.userRegId, sellOrderAccount)) {
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
