// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "cointransfertx.h"

#include "main.h"

/**################################ Base Coin (WICC) Transfer ########################################**/
bool CBaseCoinTransferTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;
    IMPLEMENT_CHECK_TX_MEMO;

    IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid);

    if (coin_amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, dust amount, %llu < %llu", coin_amount,
                         DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    return true;
}

bool CBaseCoinTransferTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    shared_ptr<CAccount> spDestAccount = nullptr;
    {
        CAccount *pDestAccount = nullptr;
        if (txAccount.IsSelfUid(toUid)) {
            pDestAccount = &txAccount; // transfer to self account
        } else {
            spDestAccount = make_shared<CAccount>();
            if (!cw.accountCache.GetAccount(toUid, *spDestAccount)) {
                if (toUid.is<CKeyID>()) { // first involved in transaction
                    spDestAccount->keyid = toUid.get<CKeyID>();
                } else {
                    return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get account info failed"),
                                    READ_ACCOUNT_FAIL, "bad-read-accountdb");
                }
            }
            pDestAccount = spDestAccount.get(); // transfer to other account
        }

        if (!txAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, coin_amount,
                                    ReceiptCode::TRANSFER_ACTUAL_COINS, receipts, pDestAccount))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, account has insufficient funds"),
                            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }
    if (spDestAccount && !cw.accountCache.SetAccount(toUid, *spDestAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account error, addr=%s",
                         spDestAccount->keyid.ToAddress()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

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
    IMPLEMENT_CHECK_TX_MEMO;

    if (transfers.empty() || transfers.size() > MAX_TRANSFER_SIZE) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, transfers is empty or too large count=%d than %d",
            transfers.size(), MAX_TRANSFER_SIZE),
                        REJECT_INVALID, "invalid-transfers");
    }

    for (size_t i = 0; i < transfers.size(); i++) {
        if (transfers[i].to_uid.IsEmpty()) {
            return state.DoS(100, ERRORMSG("%s, to_uid can not be empty", __FUNCTION__),
                            REJECT_INVALID, "invalid-toUid");
        }
        if (!cw.assetCache.CheckAsset(transfers[i].coin_symbol))
            return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, transfers[%d], invalid coin_symbol=%s", i,
                            transfers[i].coin_symbol), REJECT_INVALID, "invalid-coin-symbol");

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

    return true;
}

bool CCoinTransferTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    for (size_t i = 0; i < transfers.size(); i++) {
        const auto &transfer = transfers[i];
        const CUserID &toUid = transfer.to_uid;
        uint64_t actualCoinsToSend = transfer.coin_amount;

        // process WUSD transaction risk-reverse fees
        if (transfer.coin_symbol == SYMB::WUSD) {  // if transferring WUSD, must pay friction fees to the risk reserve
            uint64_t riskReserveFeeRatio;
            if (!cw.sysParamCache.GetParam(TRANSFER_SCOIN_RESERVE_FEE_RATIO, riskReserveFeeRatio))
                return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d], read TRANSFER_SCOIN_RESERVE_FEE_RATIO error", i),
                                READ_SYS_PARAM_FAIL, "bad-read-sysparamdb");

            uint64_t reserveFeeScoins = transfer.coin_amount * riskReserveFeeRatio / RATIO_BOOST;
            if (reserveFeeScoins > 0) {
                actualCoinsToSend -= reserveFeeScoins;

                CAccount fcoinGenesisAccount;
                if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d],"
                                                " read fcoinGenesisUid %s account info error",
                                                i, SysCfg().GetFcoinGenesisRegId().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
                }

                if (!txAccount.OperateBalance(SYMB::WUSD, SUB_FREE, reserveFeeScoins, ReceiptCode::TRANSFER_FEE_TO_RESERVE, receipts, &fcoinGenesisAccount)) {
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, add scoins to fcoin genesis account failed"),
                                     UPDATE_ACCOUNT_FAIL, "failed-add-scoins");
                }
                if (!cw.accountCache.SaveAccount(fcoinGenesisAccount)) {
                    return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d],"
                                    " update fcoinGenesisAccount info error", i),
                                    UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
                }
            }
        }

        shared_ptr<CAccount> spDestAccount = nullptr;
        {
            CAccount *pDestAccount = nullptr;
            if (txAccount.IsSelfUid(toUid)) {
                pDestAccount = &txAccount; // transfer to self account
            } else {
                spDestAccount = make_shared<CAccount>();
                if (!cw.accountCache.GetAccount(toUid, *spDestAccount)) {
                    // first-time involved in transacion
                    if (toUid.is<CKeyID>()) {
                        spDestAccount->keyid = toUid.get<CKeyID>();
                    } else if (toUid.is<CPubKey>()) {
                        spDestAccount->keyid = toUid.get<CPubKey>().GetKeyId();
                    } else {
                        return context.pState->DoS(100, ERRORMSG("%s, the toUid=%s account does not exist",
                                TX_ERR_TITLE, toUid.ToString()), REJECT_INVALID, "account-not-exist");
                    }
                }
                // register account, must be only one dest
                if ( transfers.size() == 1 && toUid.is<CPubKey>() && !spDestAccount->IsRegistered()) {
                    spDestAccount->regid = CRegID(context.height, context.index); // generate new regid for the account
                }
                pDestAccount = spDestAccount.get(); // transfer to other account
            }

            if (!txAccount.OperateBalance(transfer.coin_symbol, SUB_FREE, actualCoinsToSend, ReceiptCode::TRANSFER_ACTUAL_COINS, receipts, pDestAccount))
                return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, transfers[%d], failed to add coins in toUid %s account", i,
                            toUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "failed-sub-coins");
        }

        if (spDestAccount && !cw.accountCache.SaveAccount(*spDestAccount))
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, write dest addr %s account info error",
                        toUid.ToDebugString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

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
