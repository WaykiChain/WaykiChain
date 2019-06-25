// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pricefeedtx.h"
#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

bool CPriceFeedTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {

    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (pricePoints.size() == 0 || pricePoints.size() > 3) { //FIXME: hardcode here
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, tx price points number not within 1..3"),
            REJECT_INVALID, "bad-tx-pricepoint-size-error");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    CRegID sendRegId;
    account.GetRegId(sendRegId);
    if (!cw.delegateCache.ExistDelegate(sendRegId.ToString())) { // must be a miner
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, txUid %s account is not a delegate error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-isn't-delegate");
    }

    if (account.stakedFcoins < kDefaultPriceFeedStakedFcoinsMin) // must stake enough fcoins
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, Staked Fcoins insufficient by txUid %s account error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-stakedfoins-insufficient");

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CPriceFeedTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    CAccountLog acctLog(account);  // save account state before modification
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, deduct fee from account failed ,regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    // update the price feed cache accordingly
    if (!cw.ppCache.AddBlockPricePointInBatch(nHeight, txUid.get<CRegID>(), pricePoints)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, txUid %s account duplicated price feed exits",
                        txUid.ToString()), PRICE_FEED_FAIL, "duplicated-pricefeed");
    }

    cw.txUndo.accountLogs.push_back(acctLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid}))
        return false;

    return true;
}

bool CPriceFeedTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CPriceFeedTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CPriceFeedTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CPriceFeedTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}

string CPriceFeedTx::ToString(CAccountCache &accountCache) {
    string str;
    for (auto pp : pricePoints) {
        str += pp.ToString() + ", ";
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, pricePoints=%s, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), llFees, str, nValidHeight);
}

Object CPriceFeedTx::ToJson(const CAccountCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    Array pricePointArray;
    for (const auto &pp : pricePoints) {
        pricePointArray.push_back(pp.ToJson());
    }

    result.push_back(Pair("tx_hash",        GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("tx_addr",        keyId.ToAddress()));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("price_points",   pricePointArray));
    result.push_back(Pair("valid_height",   nValidHeight));

    return result;
}

bool CPriceFeedTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    // TODO:
    return true;
}

bool CPriceFeedTx::GetTopPriceFeederList(CCacheWrapper &cw, vector<CAccount> &priceFeederAccts) {
    // TODO:
    return true;
}