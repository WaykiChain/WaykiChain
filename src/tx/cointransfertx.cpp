// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "cointransfertx.h"

#include "main.h"

/**################################ Base Coin (WICC) Transfer ########################################**/
bool CBaseCoinTransferTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid);
    IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid);
    if (!CheckFee(context)) return false;
    IMPLEMENT_CHECK_TX_MEMO;

    if (coin_amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, dust amount, %llu < %llu", coin_amount,
                         DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CBaseCoinTransferTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, srcAccount)) {
        return false;
    }

    uint64_t minusValue = llFees + coin_amount;
    if (!srcAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(toUid, desAccount)) {
        if (toUid.is<CKeyID>()) {  // first involved in transaction
            desAccount.keyid = toUid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, coin_amount)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(toUid, desAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account error, kyeId=%s",
                         desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    return true;
}

string CBaseCoinTransferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, coin_amount=%llu, llFees=%llu, memo=%s, valid_height=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), coin_amount, llFees,
        HexStr(memo), valid_height);
}

Object CBaseCoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    SingleTransfer transfer(toUid, SYMB::WICC, coin_amount);
    Array transferArray;
    transferArray.push_back(transfer.ToJson(accountCache));

    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("transfers",   transferArray));
    result.push_back(Pair("memo",        memo));

    return result;
}

bool CCoinTransferTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid);
    if (!CheckFee(context)) return false;

    if (transfers.empty() || transfers.size() > MAX_TRANSFER_SIZE) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, transfers is empty or too large count=%d than %d",
            transfers.size(), MAX_TRANSFER_SIZE),
                        REJECT_INVALID, "invalid-transfers");
    }

    for (size_t i = 0; i < transfers.size(); i++) {
        IMPLEMENT_CHECK_TX_REGID_OR_KEYID(transfers[i].to_uid);
        auto pSymbolErr = cw.assetCache.CheckTransferCoinSymbol(transfers[i].coin_symbol);
        if (pSymbolErr) {
            return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, transfers[%d], invalid coin_symbol=%s, %s",
                i, transfers[i].coin_symbol, *pSymbolErr), REJECT_INVALID, "invalid-coin-symbol");
        }

        if (transfers[i].coin_amount < DUST_AMOUNT_THRESHOLD)
            return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, transfers[%d], dust amount, %llu < %llu",
                i, transfers[i].coin_amount, DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");


        if (!CheckCoinRange(transfers[i].coin_symbol, transfers[i].coin_amount))
            return state.DoS(100,
                ERRORMSG("CCoinTransferTx::CheckTx, transfers[%d], coin_symbol=%s, coin_amount=%llu out of valid range",
                         i, transfers[i].coin_symbol, transfers[i].coin_amount), REJECT_DUST, "invalid-coin-amount");
    }

    uint64_t minFee;
    if (!GetTxMinFee(nTxType, context.height, fee_symbol, minFee)) { assert(false); /* has been check before */ }

    if (llFees < transfers.size() * minFee) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, tx fee too small (height: %d, fee symbol: %s, fee: %llu)",
                         context.height, fee_symbol, llFees), REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CCoinTransferTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!GenerateRegID(context, srcAccount)) {
        return false;
    }

    if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, insufficient coin_amount in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coin_amount");
    }

    vector<CReceipt> receipts;

    for (size_t i = 0; i < transfers.size(); i++) {
        const auto &transfer = transfers[i];

        if (!srcAccount.OperateBalance(transfer.coin_symbol, SUB_FREE, transfer.coin_amount)) {
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d], insufficient coins in txUid %s account",
                            i, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coins");
        }

        uint64_t actualCoinsToSend = transfer.coin_amount;
        if (transfer.coin_symbol == SYMB::WUSD) {  // if transferring WUSD, must pay friction fees to the risk reserve
            uint64_t riskReserveFeeRatio;
            if (!cw.sysParamCache.GetParam(SCOIN_RESERVE_FEE_RATIO, riskReserveFeeRatio)) {
                return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d], read SCOIN_RESERVE_FEE_RATIO error", i),
                                READ_SYS_PARAM_FAIL, "bad-read-sysparamdb");
            }
            uint64_t reserveFeeScoins = transfer.coin_amount * riskReserveFeeRatio / RATIO_BOOST;
            if (reserveFeeScoins > 0) {
                actualCoinsToSend -= reserveFeeScoins;

                CAccount fcoinGenesisAccount;
                if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d],"
                        " read fcoinGenesisUid %s account info error",
                        i, SysCfg().GetFcoinGenesisRegId().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
                }

                if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, ADD_FREE, reserveFeeScoins)) {
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, add scoins to fcoin genesis account failed"),
                                     UPDATE_ACCOUNT_FAIL, "failed-add-scoins");
                }
                if (!cw.accountCache.SaveAccount(fcoinGenesisAccount))
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d],"
                        " update fcoinGenesisAccount info error", i),
                        UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

                CUserID fcoinGenesisUid(fcoinGenesisAccount.regid);
                receipts.emplace_back(txUid, fcoinGenesisUid, SYMB::WUSD, reserveFeeScoins, ReceiptCode::TRANSFER_FEE_TO_RESERVE);
                receipts.emplace_back(txUid, transfer.to_uid, SYMB::WUSD, actualCoinsToSend, ReceiptCode::TRANSFER_ACTUAL_COINS);
            }
        }

        if (srcAccount.IsMyUid(transfer.to_uid)) {
            if (!srcAccount.OperateBalance(transfer.coin_symbol, ADD_FREE, actualCoinsToSend)) {
                return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d], failed to add coins in toUid %s account",
                    i, transfer.to_uid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "failed-add-coins");
            }
        } else {
            CAccount desAccount;
            if (!cw.accountCache.GetAccount(transfer.to_uid, desAccount)) { // first involved in transacion
                if (transfer.to_uid.is<CKeyID>()) {
                    desAccount = CAccount(transfer.to_uid.get<CKeyID>());
                } else {
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, get account info failed"),
                                    READ_ACCOUNT_FAIL, "bad-read-accountdb");
                }
            }

            if (!desAccount.OperateBalance(transfer.coin_symbol, ADD_FREE, actualCoinsToSend)) {
                return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d], failed to add coins in toUid %s account",
                    i, transfer.to_uid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "failed-add-coins");
            }

            if (!cw.accountCache.SaveAccount(desAccount))
                return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, write dest addr %s account info error",
                    transfer.to_uid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!cw.accountCache.SaveAccount(srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!receipts.empty() && !cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");

    return true;
}

string CCoinTransferTx::ToString(CAccountDBCache &accountCache) {
    string transferStr = "";
    for (const auto &transfer : transfers) {
        if (!transferStr.empty()) transferStr += ",";
        transferStr += strprintf("{%s}", transfer.ToString(accountCache));
    }

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, "
        "valid_height=%d, transfers=[%s], memo=%s",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees,
        valid_height, transferStr, HexStr(memo));
}

Object CCoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    Array transferArray;
    for (const auto &transfer : transfers) {
        transferArray.push_back(transfer.ToJson(accountCache));
    }

    result.push_back(Pair("transfers",   transferArray));
    result.push_back(Pair("memo",        memo));

    return result;
}
