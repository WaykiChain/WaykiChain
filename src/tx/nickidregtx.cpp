// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "nickidregtx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "commons/util/util.h"
#include "config/version.h"
#include "main.h"
#include "persistence/contractdb.h"
#include "vm/luavm/luavmrunenv.h"
#include "miner/miner.h"

bool CNickIdRegisterTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;
    CCacheWrapper &cw = *context.pCw;

    IMPLEMENT_CHECK_TX_FEE;


    if(cw.accountCache.HaveAccount(CNickID(nickId))){
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickId is exist"), REJECT_INVALID, "nickid-exist");
    }

    if (txUid.type() != typeid(CPubKey))
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, userId must be CPubKey"), REJECT_INVALID,
                         "uid-type-error");
    if (!txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, register tx public key is invalid"),
                         REJECT_INVALID, "bad-tx-publickey");

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());

    return true;
}


bool CNickIdRegisterTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount account;
    CKeyID keyId = txUid.get<CPubKey>().GetKeyId();
    CNickID  nick = CNickID(nickId,(uint32_t)context.height);

    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, read source keyId %s account info error",
                                       keyId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!account.HaveOwnerPubKey()) {
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, keyId %s don't have pubKey", keyId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "pubkey-not-exist");
    }

    if(!account.nickid.IsEmpty()){
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, the account have nickid already!", keyId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "nickid-exist");
    }
    account.nickid       = nick;
    account.owner_pubkey = txUid.get<CPubKey>();

    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, insufficient funds in account, keyid=%s",
                                       keyId.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, write source addr %s account info error",
                                       nick.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}

string CNickIdRegisterTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, nickId=%s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.get<CPubKey>().ToString(),nickId,llFees,
                     txUid.get<CPubKey>().GetKeyId().ToAddress(), valid_height);
}

Object CNickIdRegisterTx::ToJson(const CAccountDBCache &accountCache) const {
    assert(txUid.type() == typeid(CPubKey));

    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("pubkey",         txUid.ToString()));
    result.push_back(Pair("nick_name",      nickId));

    return result;
}
