// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dextx.h"
#include "configuration.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// class CDEXBuyOrderTx

string CDEXBuyOrderTx::ToString(CAccountCache &view) {

}

Object CDEXBuyOrderTx::ToJson(const CAccountCache &view) const {

}

bool CDEXBuyOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {

}
bool CDEXBuyOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    if (COINT_TYPE_SET.count(buyCoinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::CheckTx, invalid buyCoinType"), REJECT_INVALID,
                         "bad-buyCoinType");
    } 

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXBuyOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    if (!srcAcct.OperateDexOrder(buyCoinType, buyAmount)) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, {txUid})) return false;

    return true;
}

bool CDEXBuyOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {
    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyID;
    if (!cw.accountCache.GetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::UndoExecuteTx, read account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    if (!account.UndoOperateAccount(accountLog)) {
        return state.DoS(100,
                         ERRORMSG("CDEXBuyOrderTx::UndoExecuteTx, undo operate account failed"),
                         UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::UndoExecuteTx, write account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// class CDEXSellOrderTx

string CDEXSellOrderTx::ToString(CAccountCache &view) {

}

Object CDEXSellOrderTx::ToJson(const CAccountCache &view) const {

}

bool CDEXSellOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {

}
bool CDEXSellOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    if (COINT_TYPE_SET.count(sellCoinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, invalid sellCoinType"), REJECT_INVALID,
                         "bad-sellCoinType");
    } 

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CDEXSellOrderTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    if (!srcAcct.OperateDexOrder(sellCoinType, sellAmount)) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, {txUid})) return false;

    return true;
}

bool CDEXSellOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {
    if (cw.txUndo.accountLogs.size() != 1) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, accountLogs size != 1"),
                         READ_ACCOUNT_FAIL, "bad-accountLogs-size");
    }
    const CAccountLog &accountLog = cw.txUndo.accountLogs.at(0);
    CAccount account;
    CUserID userId = accountLog.keyID;
    if (!cw.accountCache.GetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, read account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    if (!account.UndoOperateAccount(accountLog)) {
        return state.DoS(100,
                         ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, undo operate account failed"),
                         UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(userId, account)) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, write account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}
