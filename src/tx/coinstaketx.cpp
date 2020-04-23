// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinstaketx.h"

#include "config/configuration.h"
#include "main.h"
#include "persistence/assetdb.h"

bool CCoinStakeTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (stake_type != BalanceOpType::STAKE && stake_type != BalanceOpType::UNSTAKE)
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, invalid stakeType"), REJECT_INVALID, "bad-stake-type");

    if (!cw.assetCache.CheckAsset(coin_symbol))
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, invalid %s", coin_symbol), REJECT_INVALID,
                         "bad-coin-symbol");

    if (coin_amount == 0 || !CheckCoinRange(coin_symbol, coin_amount))
        return state.DoS(100, ERRORMSG("CCoinStakeTx::CheckTx, coinsToStake out of range"), REJECT_INVALID,
                         "bad-tx-coins-outofrange");

    return true;
}

bool CCoinStakeTx::ExecuteTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    if (!sp_tx_account->OperateBalance(coin_symbol, stake_type, coin_amount, ReceiptType::COIN_STAKE, receipts))
        return state.DoS(100, ERRORMSG("CCoinStakeTx::ExecuteTx, insufficient coins to stake in txUid(%s)",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coin-amount");

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
