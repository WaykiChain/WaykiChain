// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "accountregtx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "commons/util/util.h"
#include "config/version.h"
#include "main.h"
#include "persistence/contractdb.h"
#include "vm/luavm/luavmrunenv.h"
#include "miner/miner.h"

bool CAccountRegisterTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    if (!txUid.is<CPubKey>())
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, userId must be CPubKey"), REJECT_INVALID,
                         "uid-type-error");

    if (!txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, register tx public key is invalid"),
                         REJECT_INVALID, "bad-tx-publickey");

    CKeyID keyId = txUid.get<CPubKey>().GetKeyId();
    if (txAccount.HasOwnerPubKey())
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, keyId %s duplicate register", keyId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "duplicate-register-account");

    if (!miner_uid.is<CPubKey>() && !miner_uid.is<CNullID>())
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, minerId must be CPubKey or CNullID"),
                         REJECT_INVALID, "minerUid-type-error");

    if (miner_uid.is<CPubKey>()) {
        const auto &minerPubkey = miner_uid.get<CPubKey>();
        if (!minerPubkey.IsFullyValid())
            return state.DoS(100, ERRORMSG("CAccountRegisterTx::ExecuteTx, minerPubKey:%s Is Invalid",
                            minerPubkey.ToString()), UPDATE_ACCOUNT_FAIL, "MinerPKey Is Invalid");
    }
    return true;
}

//useful for cold mining
bool CAccountRegisterTx::ExecuteTx(CTxExecuteContext &context) {

    if (miner_uid.is<CPubKey>()) {
        txAccount.miner_pubkey = miner_uid.get<CPubKey>();
    }

    return true;
}

string CAccountRegisterTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.get<CPubKey>().ToString(), llFees,
                     txUid.get<CPubKey>().GetKeyId().ToAddress(), valid_height);
}

Object CAccountRegisterTx::ToJson(const CAccountDBCache &accountCache) const {
    assert(txUid.is<CPubKey>());

    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("pubkey",         txUid.ToString()));
    result.push_back(Pair("miner_pubkey",   miner_uid.ToString()));

    return result;
}
