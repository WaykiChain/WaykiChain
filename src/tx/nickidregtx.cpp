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
#include "vm/wasm/types/name.hpp"

bool CNickIdRegisterTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if(cw.accountCache.HasAccount(CNickID(nickId))){
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickId is exist"), REJECT_INVALID, "nickid-exist");
    }

    try{
        if (nickId.size() != 12)
            return state.DoS(100,ERRORMSG("Nickname length must be 12, but %s length = %d", nickId, nickId.size()),
                             REJECT_INVALID, "bad_nickid_length");
        // string prefix = "wasmio";
        // if (strcmp(nickId.c_str(),prefix.c_str()) != 0 ){
        //     return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickid is can't be start with 'wasmio'"), REJECT_INVALID,
        //                      "bad-nickid");
        // }

        string prefix = "wasmio";
        if (nickId.find(prefix) == 0 ){
            return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickid is can't be start with 'wasmio'"), REJECT_INVALID,
                             "bad-nickid");
        }

        if(wasm::name(nickId).value == 0)
            return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickid is invalid,for zero"), REJECT_INVALID,
                             "bad-nickid");

    } catch (const wasm_chain::exception &e) {
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickid is invalid"), REJECT_INVALID,
                         "bad-nickid");
    }

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    return true;
}


bool CNickIdRegisterTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (!txAccount.nickid.IsEmpty())
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, the account have nickid already!", txUid.ToString()),
                         UPDATE_ACCOUNT_FAIL, "nickid-exist");

    CNickID  nick = CNickID(wasm::string_to_name(nickId.c_str()));
    txAccount.nickid  = nick;
    if (!cw.accountCache.SetNickId(txAccount, context.height))
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, write source addr %s account info error",
                                       nick.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}



string CNickIdRegisterTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, nickId=%s, llFees=%ld, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, nickId, llFees,
                     txUid.ToString(), valid_height);
}

Object CNickIdRegisterTx::ToJson(const CAccountDBCache &accountCache) const {

    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("nick_name",      nickId));

    return result;
}
