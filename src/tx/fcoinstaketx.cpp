// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcoinstaketx.h"

#include "config/configuration.h"
#include "main.h"

bool CFcoinStakeTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (stakeType != BalanceOpType::STAKE && stakeType != BalanceOpType::UNSTAKE) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, invalid stakeType"),
                        REJECT_INVALID, "bad-stake-type");
    }

    if (fcoinsToStake == 0 || !CheckFundCoinRange(fcoinsToStake)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, fcoinsToStake out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-outofrange");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);
    return true;
}

bool CFcoinStakeTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, insufficient bcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-bcoins");
    }

    if (stakeType == BalanceOpType::STAKE) { //stake fcoins
        if (!account.OperateBalance(SYMB::WGRT, BalanceOpType::STAKE, fcoinsToStake)) {
            return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, insufficient fcoins to stake in txUid(%s)",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-fcoins");
        }
    } else {                // <= 0, unstake fcoins
        if (!account.OperateBalance(SYMB::WGRT, BalanceOpType::UNSTAKE, fcoinsToStake)) {
            return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, insufficient staked fcoins in txUid(%s)",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-fcoins");
        }
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}

string CFcoinStakeTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, stakeType=%s, fcoinsToStake=%lu, llFees=%ld, valid_height=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), GetBalanceOpTypeName(stakeType),
        fcoinsToStake, llFees, valid_height);
}

Object CFcoinStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("stake_type",     GetBalanceOpTypeName(stakeType)));
    result.push_back(Pair("coin_symbol",    SYMB::WGRT));
    result.push_back(Pair("coin_amount",    fcoinsToStake));

    return result;
}
