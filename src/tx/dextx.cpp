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
    return ""; //TODO: ...
}

Object CDEXBuyOrderTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXBuyOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
}
bool CDEXBuyOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    if (COINT_TYPE_SET.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (COINT_TYPE_SET.count(assetType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::CheckTx, invalid assetType"), REJECT_INVALID,
                         "bad-assetType");
    }

    if (coinType == assetType) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::CheckTx, coinType can not equal to assetType"), REJECT_INVALID,
                         "bad-coinType-assetType");
    }
    // TODO: check amount range?
    // TODO: check bidPrice range?

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
    // should freeze user's coin for buying the asset
    if (!srcAcct.FreezeDexCoin(coinType, buyAmount * bidPrice)) {
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXBuyOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    // TODO: save order id -> order info

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXBuyOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    // TODO: undo order id -> order info

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
    return ""; //TODO: ...
}

Object CDEXSellOrderTx::ToJson(const CAccountCache &view) const {
    return Object(); // TODO: ...
}

bool CDEXSellOrderTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return false; // TODO: ...
}

bool CDEXSellOrderTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (COINT_TYPE_SET.count(coinType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, invalid coinType"), REJECT_INVALID,
                         "bad-coinType");
    }

    if (COINT_TYPE_SET.count(assetType) == 0) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, invalid assetType"), REJECT_INVALID,
                         "bad-assetType");
    }

    if (coinType == assetType) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, coinType can not equal to assetType"), REJECT_INVALID,
                         "bad-coinType-assetType");
    }
    // TODO: check amount range?
    // TODO: check bidPrice range?

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    IMPLEMENT_CHECK_TX_SIGNATURE(srcAccount.pubKey);

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
    // freeze asset of user for selling.
    if (!srcAcct.FreezeDexAsset(assetType, sellAmount)) {
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-dex-order-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyID), srcAcct))
        return state.DoS(100, ERRORMSG("CDEXSellOrderTx::ExecuteTx, set account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    // TODO: save order id -> order info

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CDEXSellOrderTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw,
                                   CValidationState &state) {

    // TODO: undo order id -> order info

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


/* execute process for settle tx
1. get and check active order
    a. get and check active buy order
    b. get and check active sell order
2. get order detail:
    a. user_gen_order, get order tx
        then get order detail from tx obj
    b. sys_gen_order,  get sys order from dexdb cache
        then get order detail from sys order obj
3. get account of order
    a. get buyOrderAccount
    b. get sellOrderAccount
4. check order exchange type
    not support: market type <-> market type
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
    C. market type <-> limit type
        I.   dealPrice == sellOrder.askPrice    
9. get buy/sell coin/asset amount
    a. limit type
        I. buy order
            residualAmount is assetAmount
            check: dealAmount <= residualAmount
            buyCoinAmount = dealAmount*dealPrice // buyer pay coin amount to seller
            buyAssetAmount = dealAmount
            residualAmount = residualAmount - buyAssetAmount
            

        II. sell order
            residualAmount is assetAmount
            check: dealAmount <= residualAmount
            sellCoinAmount = dealAmount*dealPrice // seller get coin amount from buyer
            sellAssetAmount = dealAmount
            residualAmount = residualAmount - buyAssetAmount
    b. market type
        I. buy order
            residualAmount is coinAmount
            dealAmount * dealPrice <= residualAmount
            buyCoinAmount = dealAmount*dealPrice // buyer pay coin amount to seller
            buyAssetAmount = dealAmount
            residualAmount = residualAmount - buyCoinAmount
            if residualAmount < dealPrice { // special handle
                buyCoinAmount = residualAmount
                residualAmount = 0
            }
        II. sell order
            residualAmount is assetAmount
            dealAmount <= residualAmount
            sellCoinAmount = dealAmount*dealPrice // seller get coin amount from buyer
            sellAssetAmount = dealAmount
            residualAmount = residualAmount - buyAssetAmount

10. operate account
    a. minus buyer's buyCoinAmount
    b. add buyer's buyAssetAmount
    c. add seller's sellCoinAmount
    d. minus seller's sellCoinAmount

11. save residual amount
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
        CDEXActiveBuyOrderInfo buyOrderInfo;
        if (!cw.dexCache.GetActiveBuyOrder(dealItem.buyOrderTxCord, buyOrderInfo)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, get buy order by tx cord failed"),
                            REJECT_INVALID, "bad-get-buy-order");
        }
        CDEXActiveSellOrderInfo sellOrderInfo;
        if (!cw.dexCache.GetActiveSellOrder(dealItem.sellOrderTxCord, sellOrderInfo)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, get sell order by tx cord failed"),
                            REJECT_INVALID, "bad-get-sell-order");
        }
        // TODO: should check the order is sys order?
        shared_ptr<CDEXBuyOrderTx> pBuyOrderTx;
        if(!ReadTxFromDisk(dealItem.buyOrderTxCord, pBuyOrderTx)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read buy order tx by tx cord failed"),
                            REJECT_INVALID, "bad-read-tx");
        }
        shared_ptr<CDEXSellOrderTx> pSellOrderTx;
        if(!ReadTxFromDisk(dealItem.sellOrderTxCord, pSellOrderTx)) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, read sell order tx by tx cord failed"),
                            REJECT_INVALID, "bad-read-tx");
        }

        CAccount buyOrderAccount;
        if (!cw.accountCache.GetAccount(pBuyOrderTx->txUid, buyOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, read buy order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(buyOrderAccount));

        CAccount sellOrderAccount;
        if (!cw.accountCache.GetAccount(pSellOrderTx->txUid, sellOrderAccount)) {
            return state.DoS(100, ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, read sell order account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        cw.txUndo.accountLogs.push_back(CAccountLog(sellOrderAccount));

        // check coin type and asset type are matched
        if (pBuyOrderTx->coinType != pSellOrderTx->coinType || pBuyOrderTx->assetType != pSellOrderTx->assetType) {
            return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, coin type or asset type not match"),
                            REJECT_INVALID, "bad-order-match");
        }

        //check price match
        if (pSellOrderTx->orderType == ORDER_LIMIT_PRICE && pBuyOrderTx->orderType == ORDER_LIMIT_PRICE) {
            if (pSellOrderTx->askPrice < dealItem.dealPrice || dealItem.dealPrice < pBuyOrderTx->bidPrice ) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the expected price not match"),
                                REJECT_INVALID, "bad-price-match");
            }
        } else if (pSellOrderTx->orderType == ORDER_LIMIT_PRICE && pBuyOrderTx->orderType == ORDER_MARKET_PRICE) {
            if (pSellOrderTx->askPrice != dealItem.dealPrice) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the expected price not match"),
                                REJECT_INVALID, "bad-price-match");
            }
        } else if (pSellOrderTx->orderType == ORDER_MARKET_PRICE && pBuyOrderTx->orderType == ORDER_LIMIT_PRICE) {
            if (pBuyOrderTx->bidPrice != dealItem.dealPrice) {
                return state.DoS(100, ERRORMSG("CDEXSettleTx::CheckTx, the expected price not match"),
                                REJECT_INVALID, "bad-price-match");
            }
        } else {
            assert(pSellOrderTx->orderType == ORDER_MARKET_PRICE && pBuyOrderTx->orderType == ORDER_MARKET_PRICE);
        }

        // check deal amount is enough
        if (dealItem.dealAmount > buyOrderInfo.residualAmount || dealItem.dealAmount > sellOrderInfo.residualAmount) {
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
            return state.DoS(100, ERRORMSG("CDEXSellOrderTx::UndoExecuteTx, write account info error"),
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
