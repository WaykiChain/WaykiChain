// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "cointransfertx.h"

#include "main.h"

bool CCoinTransferTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    // TODO: fees in WICC/WGRT/WUSD
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid.type());

     // TODO: check range
    if (kCoinTypeMapName.count(coinType) == 0 || kCoinTypeMapName.count(feesCoinType) == 0) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, invalid coin type"), REJECT_INVALID,
                         "invalid-coin-type");
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

bool CCoinTransferTx::ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    if (!srcAccount.OperateBalance(feesCoinType, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, insufficient bcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-bcoins");
    }

    if (!srcAccount.OperateBalance(coinType, MINUS_VALUE, coins)) {
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

    uint64_t actualCoinsToSend = coins;
    if (coinType == CoinType::WUSD) { //if transferring WUSD, must pay 0.01% to the risk reserve
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
        uint64_t reserveFeeScoins = coins * riskReserveFeeRatio / kPercentBoost;
        actualCoinsToSend -= reserveFeeScoins;

        fcoinGenesisAccount.OperateBalance("WUSD", ADD_FREE, reserveFeeScoins);
        if (!cw.accountCache.SaveAccount(fcoinGenesisAccount))
            return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, update fcoinGenesisAccount info error"),
                            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
    }
    if (!desAccount.OperateBalance(coinType, ADD_FREE, actualCoinsToSend)) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, failed to add coins in toUid %s account", toUid.ToString()),
                        UPDATE_ACCOUNT_FAIL, "failed-add-coins");
    }

    if (!cw.accountCache.SaveAccount(desAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::ExecuteTx, write dest addr %s account info error", toUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid, toUid}))
        return false;

    return true;
}

string CCoinTransferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, coins=%ld, coinType=%s, llFees=%ld, feesCoinType=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), coins,
        GetCoinTypeName(coinType), llFees, GetCoinTypeName(feesCoinType), nValidHeight);
}

Object CCoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID srcKeyId;
    accountCache.GetKeyId(txUid, srcKeyId);

    CKeyID desKeyId;
    accountCache.GetKeyId(toUid, desKeyId);

    result.push_back(Pair("txid",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("tx_addr",            srcKeyId.ToAddress()));
    result.push_back(Pair("to_uid",             toUid.ToString()));
    result.push_back(Pair("to_addr",            desKeyId.ToAddress()));
    result.push_back(Pair("coins",              coins));
    result.push_back(Pair("coin_type",          GetCoinTypeName(coinType)));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("fees_coin_type",     GetCoinTypeName(feesCoinType)));
    result.push_back(Pair("valid_height",       nValidHeight));
    result.push_back(Pair("memo",               HexStr(memo)));

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