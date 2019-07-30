// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "cointransfertx.h"

#include "main.h"

/**################################ Base Coin (WICC) Transfer ########################################**/
bool CBaseCoinTransferTx::CheckTx(int height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid.type());

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, account unregistered"),
                         REJECT_INVALID, "bad-account-unregistered");

    if (srcAccount.GetToken(SYMB::WICC).free_amount < llFees + bcoins) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, account balance insufficient"),
                         REJECT_INVALID, "account-balance-insufficient");
    }

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CBaseCoinTransferTx::ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    CAccount desAcct;
    bool generateRegID = false;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        if (txUid.type() == typeid(CPubKey)) {
            srcAcct.owner_pubkey = txUid.get<CPubKey>();

            CRegID regId;
            // If the source account does NOT have CRegID, need to generate a new CRegID.
            if (!cw.accountCache.GetRegId(txUid, regId)) {
                srcAcct.regid = CRegID(height, index);
                generateRegID = true;
            }
        }
    }

    uint64_t minusValue = llFees + bcoins;
    if (!srcAcct.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (generateRegID) {
        if (!cw.accountCache.SaveAccount(srcAcct))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyid), srcAcct))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    if (!cw.accountCache.GetAccount(toUid, desAcct)) {
        if (toUid.type() == typeid(CKeyID)) {  // Target account does NOT have CRegID
            desAcct.keyid    = toUid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAcct.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, bcoins)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(toUid, desAcct))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account error, kyeId=%s",
                         desAcct.keyid.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-save-account");

    if (!SaveTxAddresses(height, index, cw, state, {txUid, toUid}))
        return false;

    return true;
}

string CBaseCoinTransferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, bcoins=%ld, llFees=%ld, memo=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), bcoins, llFees,
        HexStr(memo), nValidHeight);
}

Object CBaseCoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID desKeyId;
    accountCache.GetKeyId(toUid, desKeyId);
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)
    result.push_back(Pair("to_uid",    toUid.ToString()));
    result.push_back(Pair("to_addr",   desKeyId.ToAddress()));
    result.push_back(Pair("transer_bcoins", bcoins));
    result.push_back(Pair("memo",      HexStr(memo)));

    return result;
}

bool CBaseCoinTransferTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);

    CKeyID desKeyId;
    if (!cw.accountCache.GetKeyId(toUid, desKeyId))
        return false;

    keyIds.insert(desKeyId);

    return true;
}

/**################################ Universal Coin Transfer ########################################**/

bool CCoinTransferTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    // TODO: fees in WICC/WGRT/WUSD
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid.type());

    if (!kCoinTypeSet.count(coin_symbol) || !kCoinTypeSet.count(fee_symbol)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, invalid coin or fee type"),
                        REJECT_INVALID, "invalid-coin-type");
    }

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    IMPLEMENT_CHECK_TX_SIGNATURE(srcAccount.owner_pubkey);

    return true;
}

bool CCoinTransferTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, insufficient bcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-bcoins");
    }

    if (!srcAccount.OperateBalance(coin_symbol, SUB_FREE, coin_amount)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, insufficient coins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coins");
    }

    if (!cw.accountCache.SaveAccount(srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(toUid, desAccount)) { // Target account has NO CAccount(first involved in transacion)
        if (toUid.type() == typeid(CKeyID)) {  // Target account does NOT have CRegID
            desAccount.keyid    = toUid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    uint64_t actualCoinsToSend = coin_amount;
    if (coin_symbol == SYMB::WUSD) { //if transferring WUSD, must pay 0.01% to the risk reserve
        CAccount fcoinGenesisAccount;
        if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, read fcoinGenesisUid %s account info error"),
                            READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        uint64_t riskReserveFeeRatio;
        if (!cw.sysParamCache.GetParam(SCOIN_RESERVE_FEE_RATIO, riskReserveFeeRatio)) {
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, read SCOIN_RESERVE_FEE_RATIO error"),
                             READ_SYS_PARAM_FAIL, "bad-read-sysparamdb");
        }
        uint64_t reserveFeeScoins = coin_amount * riskReserveFeeRatio / kPercentBoost;
        actualCoinsToSend -= reserveFeeScoins;

        fcoinGenesisAccount.OperateBalance(SYMB::WUSD, ADD_FREE, reserveFeeScoins);
        if (!cw.accountCache.SaveAccount(fcoinGenesisAccount))
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, update fcoinGenesisAccount info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
    }
    if (!desAccount.OperateBalance(coin_symbol, ADD_FREE, actualCoinsToSend)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, failed to add coins in toUid %s account", toUid.ToString()),
                        UPDATE_ACCOUNT_FAIL, "failed-add-coins");
    }

    if (!cw.accountCache.SaveAccount(desAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, write dest addr %s account info error", toUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!SaveTxAddresses(height, index, cw, state, {txUid, toUid}))
        return false;

    return true;
}

string CCoinTransferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, coin_symbol=%s, coin_amount=%ld, fee_symbol=%s, llFees=%ld, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), coin_symbol, coin_amount,
        fee_symbol, llFees, nValidHeight);
}

Object CCoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID desKeyId;
    accountCache.GetKeyId(toUid, desKeyId);
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)
    result.push_back(Pair("to_uid",      toUid.ToString()));
    result.push_back(Pair("to_addr",     desKeyId.ToAddress()));
    result.push_back(Pair("coin_symbol", coin_symbol));
    result.push_back(Pair("coin_amount", coin_amount));
    result.push_back(Pair("fee_symbol",  fee_symbol));
    result.push_back(Pair("memo",        HexStr(memo)));

    return result;
}

bool CCoinTransferTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);
    CKeyID desKeyId;
    if (!cw.accountCache.GetKeyId(toUid, desKeyId))
        return false;

    keyIds.insert(desKeyId);

    return true;
}