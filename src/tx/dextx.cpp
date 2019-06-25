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
    activeBuyOrder.residualAmount = assetAmount;
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
    activeSellOrder.residualAmount = assetAmount;
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
    activeBuyOrder.residualAmount = coinAmount;
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
    activeSellOrder.residualAmount = assetAmount;
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
5. check coin type match
    buyOrder.coinType == sellOrder.coinType
6. check asset type match
    buyOrder.assetType == sellOrder.assetType
7. check price is right in valid height block
8. check price match
    a. limit type <-> limit type
        I.   dealPrice <= buyOrder.bidPrice
        II.  dealPrice >= sellOrder.askPrice
    b. limit type <-> market type
        I.   dealPrice == buyOrder.bidPrice
    c. market type <-> limit type
        I.   dealPrice == sellOrder.askPrice
9. get and check the deal detail
    a. buy order
        dealCoinAmount = dealAmount * dealPrice
        dealAssetAmount = dealAmount
        activeBuyOrder.totalDealCoinAmount  += dealCoinAmount
        activeBuyOrder.totalDealAssetAmount += dealAssetAmount
        if market price order {
            limitCoinAmount = buyOrder.limitAmount
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
        fulfilled = (residualAmount == 0)
    b. sell order
        dealAssetAmount = dealAmount
        activeSellOrder.totalDealAssetAmount += dealAssetAmount
        limitAssetAmount = sellOrder.limitAmount
        check: limitAssetAmount >= activeBuyOrder.totalDealAssetAmount
        residualAmount = limitAssetAmount - dealCoinAmount
        fulfilled = (residualAmount == 0)

10. operate account
    a. buyerFrozenCoins     -= dealCoinAmount
    b. buyerAssets          += dealAssetAmount
    c. sellerCoins          += dealCoinAmount
    d. sellerFrozenAssets   -= dealAssetAmount

11. check order fulfiled or save residual amount
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
        CDEXActiveSellOrder sellOrderInfo;
        if (!cw.dexCache.GetActiveSellOrder(dealItem.sellOrderId, sellOrderInfo)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, get sell order by tx cord failed"),
                            REJECT_INVALID, "bad-get-sell-order");
        }
        // 2. get order detail data:
        CDEXBuyOrder buyOrderData;
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
        CDEXBuyOrder sellOrderData;
        if (activeBuyOrder.generateType == USER_GEN_ORDER) {
            shared_ptr<CDEXSellLimitOrderTx> pSellOrderTx;
            if(!ReadTxFromDisk(sellOrderInfo.txCord, pSellOrderTx)) {
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
        if (!cw.accountCache.GetAccount(dealItem.buyOrderId, buyOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read buy order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(buyOrderAccount));
        CAccount sellOrderAccount;
        if (!cw.accountCache.GetAccount(dealItem.sellOrderId, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, read sell order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(sellOrderAccount));

        // 4. check order type match
        if (buyOrderData.orderType == ORDER_MARKET_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, not support for order type pair(market, market)"),
                            REJECT_INVALID, "bad-order-match");
        }
        // 5. check coin type match
        if (buyOrderData.coinType != sellOrderData.coinType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, coin type not match"),
                            REJECT_INVALID, "bad-order-match");
        }
        // 6. check asset type match
        if (buyOrderData.assetType != sellOrderData.assetType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, asset type not match"),
                            REJECT_INVALID, "bad-order-match");
        }

        // 7. check price is right in valid height block
        // TODO: ...

        // 8. check price match
        if (buyOrderData.orderType == ORDER_LIMIT_PRICE && sellOrderData.orderType == ORDER_LIMIT_PRICE) {
            if (dealItem.dealPrice > buyOrderData.price < dealItem.dealPrice 
                || dealItem.dealPrice < sellOrderData.price ) {
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
        }

        // 9. get buy/sell coin/asset amount
        // check deal amount is enough
        if (dealItem.dealAmount > activeBuyOrder.residualAmount || dealItem.dealAmount > sellOrderInfo.residualAmount) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, deal amount exceed the remaining amount of buy/sell order"),
                            REJECT_INVALID, "bad-price-match");
        }

        uint64_t dealCoin = dealItem.dealAmount * dealItem.dealPrice;
        // settle sell/buy order's coin
        if (   !sellOrderAccount.MinusDEXFrozenCoin(pSellOrderTx->assetType, dealItem.dealAmount) // minus seller's assets
            || !buyOrderAccount.OperateBalance(pBuyOrderTx->assetType, ADD_VALUE, dealCoin)       // add buyer's assets
            || !buyOrderAccount.MinusDEXFrozenCoin(pBuyOrderTx->assetType, dealItem.dealAmount)   // minus buyer's coins
            || !sellOrderAccount.OperateBalance(pBuyOrderTx->coinType, ADD_VALUE, dealCoin) ){    // add seller's coin
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, operate coins or assets failed"),
                            REJECT_INVALID, "bad-operate-match");
        }

        if (!cw.accountCache.SetAccount(pBuyOrderTx->txUid, buyOrderAccount)
            || !cw.accountCache.SetAccount(pBuyOrderTx->txUid, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellLimitOrderTx::UndoExecuteTx, write account info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }

        // TODO: get accounts of each item

        // TODO: switch order id :
        //case common sell/buy id:
            // TODO: get order id -> order info
        // case sys sell/buy id:
            // TODO: get sys order id -> order info


        // TODO: add tx related ids
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
