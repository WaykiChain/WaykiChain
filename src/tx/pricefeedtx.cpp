// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeedtx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/pricefeeddb.h"
#include "tx.h"
#include "commons/util/util.h"
#include "config/version.h"

bool CPriceFeedTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;

    if (price_points.size() == 0 || price_points.size() > 2) {  // FIXME: hardcode here
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, price points number not within 1..2"), REJECT_INVALID,
                         "bad-tx-pricepoint-size-error");
    }

    for (const auto &pricePoint : price_points) {
        const CoinPricePair &coinPrice = pricePoint.coin_price_pair;
        if (std::get<0>(coinPrice) != SYMB::WICC && std::get<0>(coinPrice) != SYMB::WGRT) {
            return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, coin type should be WICC or WGRT"), REJECT_INVALID,
                             "bad-tx-coin-type");
        }

        if (std::get<1>(coinPrice) != SYMB::USD) {
            return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, currency type should be USD"), REJECT_INVALID,
                             "bad-tx-currency-type");
        }

        const uint64_t &price = pricePoint.price;
        if (price == 0) {
            return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, invalid price"), REJECT_INVALID,
                             "bad-tx-invalid-price");
        }
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);
    return true;
}

bool CPriceFeedTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    CRegID sendRegId = txUid.get<CRegID>();
    if (!cw.delegateCache.IsActiveDelegate(sendRegId.ToString())) { // must be a delegate
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, txUid %s account is not a delegate error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-isn't-delegate");
    }

    uint64_t stakedAmountMin;
    if (!cw.sysParamCache.GetParam(PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN, stakedAmountMin)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, read PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN error",
                        txUid.ToString()), READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }
    CAccountToken accountToken = account.GetToken(SYMB::WICC);
    if (accountToken.staked_amount < stakedAmountMin * COIN) // must stake enough bcoins to be a price feeder
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, Staked Bcoins insufficient(%llu vs %llu) by txUid %s account error",
                        accountToken.voted_amount, stakedAmountMin, txUid.ToString()),
                        PRICE_FEED_FAIL, "account-staked-boins-insufficient");

    if (!account.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, deduct fee from account failed ,regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    // update the price feed cache accordingly
    if (!cw.ppCache.AddBlockPricePointInBatch(context.height, txUid.get<CRegID>(), price_points)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, txUid %s account duplicated price feed exits",
                        txUid.ToString()), PRICE_FEED_FAIL, "duplicated-pricefeed");
    }

    return true;
}

string CPriceFeedTx::ToString(CAccountDBCache &accountCache) {
    string str;
    for (auto pp : price_points) {
        str += pp.ToString() + ", ";
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, price_points=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees, str, valid_height);
}

Object CPriceFeedTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    Array pricePointArray;
    for (const auto &pp : price_points) {
        pricePointArray.push_back(pp.ToJson());
    }
    result.push_back(Pair("price_points", pricePointArray));

    return result;
}
