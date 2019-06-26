// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"

#include "configuration.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyLimitOrderTx

string CDEXBuyLimitOrderTx::ToString(CAccountCache &view) {
    return ""; //TODO: ...
}

Object CDEXBuyLimitOrderTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXBuyLimitOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
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
    uint64_t coinAmount = assetAmount * bidPrice;
    if (!srcAcct.FreezeDexCoin(coinType, coinAmount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveBuyOrder activeBuyOrder;
    activeBuyOrder.generateType = USER_GEN_ORDER;
    activeBuyOrder.totalDealCoinAmount = 0;
    activeBuyOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.SetActiveBuyOrder(txHash, activeBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::ExecuteTx, set active buy order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXBuyLimitOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {


    if (!cw.dexCache.UndoActiveBuyOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, undo active buy order failed"),
                         UPDATE_ACCOUNT_FAIL, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXBuyLimitOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyID;
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


///////////////////////////////////////////////////////////////////////////////
// class CDEXSellLimitOrderTx

string CDEXSellLimitOrderTx::ToString(CAccountCache &view) {
    return ""; //TODO: ...
}

Object CDEXSellLimitOrderTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXSellLimitOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
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

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveSellOrder activeSellOrder;
    activeSellOrder.generateType = USER_GEN_ORDER;
    activeSellOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.SetActiveSellOrder(txHash, activeSellOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, set active sell order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSellLimitOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!cw.dexCache.UndoActiveSellOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, undo active sell order failed"),
                         UPDATE_ACCOUNT_FAIL, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyID;
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

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyMarketOrderTx

string CDEXBuyMarketOrderTx::ToString(CAccountCache &view) {
    return ""; //TODO: ...
}

Object CDEXBuyMarketOrderTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXBuyMarketOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
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

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    const uint256 &txHash = GetHash();
    CDEXActiveBuyOrder activeBuyOrder;
    activeBuyOrder.generateType = USER_GEN_ORDER;
    activeBuyOrder.totalDealCoinAmount = 0;
    activeBuyOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.SetActiveBuyOrder(txHash, activeBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::ExecuteTx, set active buy order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXBuyMarketOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!cw.dexCache.UndoActiveBuyOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, undo active buy order failed"),
                         UPDATE_ACCOUNT_FAIL, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXBuyMarketOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyID;
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

///////////////////////////////////////////////////////////////////////////////
// class CDEXSellMarketOrderTx

string CDEXSellMarketOrderTx::ToString(CAccountCache &view) {
    return ""; //TODO: ...
}

Object CDEXSellMarketOrderTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXSellMarketOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
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
    if (!srcAcct.FreezeDexCoin(coinType, assetAmount)) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    
    const uint256 &txHash = GetHash();
    CDEXActiveSellOrder activeSellOrder;
    activeSellOrder.generateType = USER_GEN_ORDER;
    activeSellOrder.totalDealAssetAmount = 0;
    if (!cw.dexCache.SetActiveSellOrder(txHash, activeSellOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::ExecuteTx, set active sell order failed"),
                         WRITE_ACCOUNT_FAIL, "bad-write-dexdb");
    }

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = txHash;

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSellMarketOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    if (!cw.dexCache.UndoActiveSellOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, undo active sell order failed"),
                         UPDATE_ACCOUNT_FAIL, "bad-undo-data");
    }

    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXSellMarketOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyID;
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


///////////////////////////////////////////////////////////////////////////////
// class CDEXSettleTx
string CDEXSettleTx::ToString(CAccountCache &view) {
    return ""; //TODO: ...
}

Object CDEXSettleTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXSettleTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
}

bool CDEXSettleTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");
    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");
    // TODO: check Settler's uid must be the authorized user

    IMPLEMENT_CHECK_TX_SIGNATURE(srcAccount.pubKey);

    return true;
}


/* process flow for settle tx
1. get and check active order
    a. get and check activeBuyOrder
    b. get and check activeSellOrder
2. get order data:
    a. if order is USER_ORDER: 
        step 1. get and check order tx object from block store
        step 2. get order data from tx object
    b. if order is SYS_ORDER:  
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
8. get and check the deal amount details

    dealCoinAmount = dealAmount * dealPrice
    dealAssetAmount = dealAmount
    a. buy order
        activeBuyOrder.totalDealCoinAmount  += dealCoinAmount
        activeBuyOrder.totalDealAssetAmount += dealAssetAmount
        if market price order {
            limitCoinAmount = buyOrder.coinAmount
            check: limitCoinAmount >= activeBuyOrder.totalDealCoinAmount
            residualAmount = limitCoinAmount - dealCoinAmount
            if residualAmount < dealPrice { 
                // the residual coin amount is not enough to buy an asset. 
                dealCoinAmount += residualAmount
                activeBuyOrder.totalDealCoinAmount += residualAmount
                assert(residualAmount == totalDealCoinAmount)
                residualAmount = 0
            }
        } else { //limit price order
            limitAssetAmount = buyOrder.limitAmount
            check: limitAssetAmount >= activeBuyOrder.totalDealAssetAmount
            residualAmount = limitAssetAmount - dealCoinAmount
        }
    b. sell order
        dealAssetAmount = dealAmount
        activeSellOrder.totalDealAssetAmount += dealAssetAmount
        limitAssetAmount = sellOrder.limitAmount
        check: limitAssetAmount >= activeBuyOrder.totalDealAssetAmount
        residualAmount = limitAssetAmount - dealCoinAmount
    
    fulfilled = (residualAmount == 0)

9. operate account
    a. buyerFrozenCoins     -= dealCoinAmount
    b. buyerAssets          += dealAssetAmount
    c. sellerCoins          += dealCoinAmount
    d. sellerFrozenAssets   -= dealAssetAmount

10. check order fulfiled or save residual amount
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
        CDEXActiveBuyOrder activeBuyOrder;
        if (!cw.dexCache.GetActiveBuyOrder(dealItem.buyOrderId, activeBuyOrder)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, get buy order by tx cord failed"),
                            REJECT_INVALID, "bad-get-buy-order");
        }
        CDEXActiveSellOrder activeSellOrder;
        if (!cw.dexCache.GetActiveSellOrder(dealItem.sellOrderId, activeSellOrder)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, get sell order by tx cord failed"),
                            REJECT_INVALID, "bad-get-sell-order");
        }
        // 2. get order detail data:
        CDEXOrderData buyOrderData;
        if (activeBuyOrder.generateType == USER_GEN_ORDER) {
            shared_ptr<CDEXOrderBaseTx> pBuyOrderTx;
            if(!ReadTxFromDisk(activeBuyOrder.txCord, pBuyOrderTx)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read buy order tx by tx cord failed"),
                                REJECT_INVALID, "bad-read-tx");
            }
            pBuyOrderTx->GetOrderData(buyOrderData);
        } else {
            assert(activeBuyOrder.generateType == SYSTEM_GEN_ORDER);
            // TODO: get order data from system gen order
        }
        CDEXOrderData sellOrderData;
        if (activeBuyOrder.generateType == USER_GEN_ORDER) {
            shared_ptr<CDEXSellLimitOrderTx> pSellOrderTx;
            if(!ReadTxFromDisk(activeSellOrder.txCord, pSellOrderTx)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read sell order tx by tx cord failed"),
                                REJECT_INVALID, "bad-read-tx");
            }
            pSellOrderTx->GetOrderData(sellOrderData);
        } else {
            assert(activeBuyOrder.generateType == SYSTEM_GEN_ORDER);
            // TODO: get order data from system gen order
        }

        // 3. get account of order
        CAccount buyOrderAccount;
        if (!cw.accountCache.GetAccount(buyOrderData.uid, buyOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read buy order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(buyOrderAccount));
        CAccount sellOrderAccount;
        if (!cw.accountCache.GetAccount(sellOrderData.uid, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read sell order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(sellOrderAccount));

        // 4. check coin type match
        if (buyOrderData.coinType != sellOrderData.coinType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, coin type not match"),
                            REJECT_INVALID, "bad-order-match");
        }
        // 5. check asset type match
        if (buyOrderData.assetType != sellOrderData.assetType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, asset type not match"),
                            REJECT_INVALID, "bad-order-match");
        }

        // 6. check price is right in valid height block
        // TODO: ...

        // 7. check price match
        if (buyOrderData.orderType == ORDER_LIMIT_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE) {
            if ( buyOrderData.price < dealItem.dealPrice 
                || sellOrderData.price > dealItem.dealPrice ) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the expected price not match"),
                                REJECT_INVALID, "bad-price-match");
            }
        } else if (buyOrderData.orderType == ORDER_LIMIT_PRICE && sellOrderData.orderType == ORDER_MARKET_PRICE) {
            if (dealItem.dealPrice != buyOrderData.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the expected price not match"),
                                REJECT_INVALID, "bad-price-match");
            }
        } else if (buyOrderData.orderType == ORDER_MARKET_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice != sellOrderData.price) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the expected price not match"),
                                REJECT_INVALID, "bad-price-match");
            }
        } else {
            assert(buyOrderData.orderType == ORDER_LIMIT_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE);
            // no limit
        }

        // 8. get and check the deal amount details
        // check deal amount is enough
        uint64_t dealCoinAmount = dealItem.dealAmount * dealItem.dealPrice;
        uint64_t dealAssetAmount = dealItem.dealAmount;
        uint64_t buyResidualAmount = 0;
        uint64_t sellResidualAmount = 0;

        activeBuyOrder.totalDealCoinAmount += dealCoinAmount;
        activeBuyOrder.totalDealAssetAmount += dealAssetAmount;
        if (buyOrderData.orderType == ORDER_MARKET_PRICE) {
            uint64_t limitCoinAmount = buyOrderData.coinAmount;
            if (limitCoinAmount < activeBuyOrder.totalDealCoinAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the deal coin amount exceed the limit coin amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitCoinAmount - activeBuyOrder.totalDealCoinAmount;
            // TODO: handle: small residual coin amount is not enough to buy one asset. 
        } else {
            uint64_t limitAssetAmount = buyOrderData.assetAmount;
            if (limitAssetAmount < activeBuyOrder.totalDealAssetAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            buyResidualAmount = limitAssetAmount - activeBuyOrder.totalDealAssetAmount;
            // TODO: handle: small residual asset amount is not enough to sell for one coin. 
        }

        { // get and check sell order residualAmount
            activeSellOrder.totalDealAssetAmount += dealAssetAmount;
            uint64_t limitAssetAmount = sellOrderData.assetAmount;
            if (limitAssetAmount < activeSellOrder.totalDealAssetAmount) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the deal asset amount exceed the limit asset amount of buy order"),
                                REJECT_INVALID, "exceeded-deal-amount");
            }
            sellResidualAmount = limitAssetAmount - activeBuyOrder.totalDealAssetAmount;
            // TODO: handle: small residual asset amount is not enough to sell for one coin. 
        }

        // settle sell/buy order's coin
        if (   !buyOrderAccount.MinusDEXFrozenCoin(buyOrderData.coinType, dealCoinAmount)           // - minus buyer's coins
            || !buyOrderAccount.OperateBalance(buyOrderData.assetType, ADD_VALUE, dealAssetAmount)  // + add buyer's assets
            || !sellOrderAccount.OperateBalance(sellOrderData.coinType, ADD_VALUE, dealCoinAmount)  // + add seller's coin
            || !sellOrderAccount.MinusDEXFrozenCoin(sellOrderData.assetType, dealAssetAmount)) {    // - minus seller's assets
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, operate coins or assets failed"),
                            REJECT_INVALID, "operate-acount-failed");
        }

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
            if (!cw.dexCache.EraseActiveBuyOrder(dealItem.buyOrderId, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.SetActiveBuyOrder(dealItem.buyOrderId, activeBuyOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, erase active buy order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }            
        }

        if (sellResidualAmount == 0) { // sell order fulfilled
            // erase active order
            if (!cw.dexCache.EraseActiveSellOrder(dealItem.sellOrderId, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }
        } else {
            if (!cw.dexCache.SetActiveSellOrder(dealItem.sellOrderId, activeSellOrder, cw.txUndo.dbOpLogMap)) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, erase active sell order failed"),
                                    REJECT_INVALID, "write-dexdb-failed");
            }            
        }

        if (!cw.accountCache.SetAccount(buyOrderData.uid, buyOrderAccount)
            || !cw.accountCache.SetAccount(buyOrderData.uid, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, write account info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }

    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSettleTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSettleTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    // TODO: undo order id -> order info
    auto rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        const CAccountLog &accountLog = *rIterAccountLog;
        CAccount account;
        CUserID userId = accountLog.keyID;
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
