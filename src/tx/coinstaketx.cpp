// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinstaketx.h"

#include "config/configuration.h"
#include "main.h"

bool CCoinStakeTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());

    if (stake_type != BalanceOpType::STAKE && stake_type != BalanceOpType::UNSTAKE) {
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, invalid stakeType"), REJECT_INVALID, "bad-stake-type");
    }

    // TODO: use issued asset registry in future to replace below hard-coding
    if (coin_symbol != SYMB::WICC && coin_symbol != SYMB::WUSD && coin_symbol != SYMB::WGRT) {
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, invalid coin_symbol"), REJECT_INVALID,
                         "bad-coin-symbol");
    }

    if (coin_amount == 0 || !CheckCoinRange(coin_symbol, coin_amount)) {
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, coinsToStake out of range"), REJECT_INVALID,
                         "bad-tx-coins-outofrange");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, read txUid %s account info error", txUid.ToString()),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : account.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CCoinStakeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CCoinStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), UCOIN_STAKE_FAIL, "bad-read-accountdb");

    if (!GenerateRegID(context, account)) {
        return false;
    }

    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCoinStakeTx::ExecuteTx, insufficient coins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coins");
    }

    if (!account.OperateBalance(coin_symbol, stake_type, coin_amount)) {
        return state.DoS(100, ERRORMSG("CCoinStakeTx::ExecuteTx, insufficient coins to stake in txUid(%s)",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coin-amount");
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CCoinStakeTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}

string CCoinStakeTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, stake_type=%s, coin_amount=%lu, fee_symbol=%s, llFees=%llu, "
        "valid_height=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), GetBalanceOpTypeName(stake_type),
        coin_amount, fee_symbol, llFees, valid_height);
}

Object CCoinStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("stake_type",     GetBalanceOpTypeName(stake_type)));
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("coin_amount",    coin_amount));

    return result;
}
