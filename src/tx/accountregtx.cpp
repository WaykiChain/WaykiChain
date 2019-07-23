// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "accountregtx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "commons/util.h"
#include "main.h"
#include "persistence/contractdb.h"
#include "vm/luavm/vmrunenv.h"
#include "miner/miner.h"
#include "config/version.h"

bool CAccountRegisterTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;

    if (txUid.type() != typeid(CPubKey))
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, userId must be CPubKey"),
            REJECT_INVALID, "uid-type-error");

    if ((minerUid.type() != typeid(CPubKey)) && (minerUid.type() != typeid(CNullID)))
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, minerId must be CPubKey or CNullID"),
            REJECT_INVALID, "minerUid-type-error");

    if (!txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, register tx public key is invalid"),
            REJECT_INVALID, "bad-tx-publickey");

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());

    return true;
}

bool CAccountRegisterTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    CRegID regId(nHeight, nIndex);
    CKeyID keyId = txUid.get<CPubKey>().GetKeyId();
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, read source keyId %s account info error",
            keyId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    account.regid = regId;
    account.keyid = keyId;

    if (account.owner_pubkey.IsFullyValid() && account.owner_pubkey.GetKeyId() == keyId)
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, read source keyId %s duplicate register",
            keyId.ToString()), UPDATE_ACCOUNT_FAIL, "duplicate-register-account");

    account.owner_pubkey = txUid.get<CPubKey>();
    if (!account.OperateBalance("WICC", BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, insufficient funds in account, keyid=%s",
                        keyId.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    if (typeid(CPubKey) == minerUid.type()) {
        account.miner_pubkey = minerUid.get<CPubKey>();
        if (account.miner_pubkey.IsValid() && !account.miner_pubkey.IsFullyValid()) {
            return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, minerPubKey:%s Is Invalid",
                account.miner_pubkey.ToString()), UPDATE_ACCOUNT_FAIL, "MinerPKey Is Invalid");
        }
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, write source addr %s account info error",
            regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

   if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CAccountRegisterTx::GetInvolvedKeyIds(CCacheWrapper & cw, set<CKeyID> &keyIds) {
    if (!txUid.get<CPubKey>().IsFullyValid())
        return false;

    keyIds.insert(txUid.get<CPubKey>().GetKeyId());
    return true;
}

string CAccountRegisterTx::ToString(CAccountDBCache &view) {
    return strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.get<CPubKey>().ToString(), llFees,
                     txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight);
}

Object CAccountRegisterTx::ToJson(const CAccountDBCache &AccountView) const {
    assert(txUid.type() == typeid(CPubKey));
    string address = txUid.get<CPubKey>().GetKeyId().ToAddress();
    string userPubKey = txUid.ToString();
    string userMinerPubKey = minerUid.ToString();

    Object result;
    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("addr",           address));
    result.push_back(Pair("pubkey",         userPubKey));
    result.push_back(Pair("miner_pubkey",   userMinerPubKey));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   nValidHeight));
    return result;
}
