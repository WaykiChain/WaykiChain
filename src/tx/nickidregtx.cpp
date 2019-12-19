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
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_FEE;


    if(cw.accountCache.HaveAccount(CNickID(nickId))){
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickId is exist"), REJECT_INVALID, "nickid-exist");
    }

    try{
        if (nickId.size() != 12)
            return state.DoS(100,ERRORMSG("Nickname length must be 12, but %s length = %d", nickId, nickId.size()),
                             REJECT_INVALID, "bad_nickid_length");
        if(wasm::name(nickId).value == 0){
            return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickid is invalid,for zero"), REJECT_INVALID,
                             "bad-nickid");
        }
    }catch (const wasm::exception& e ){
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, nickid is invalid"), REJECT_INVALID,
                         "bad-nickid");
    }

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;

}


bool CNickIdRegisterTx::ExecuteTx(CTxExecuteContext &context) {

    IMPLEMENT_DEFINE_CW_STATE;

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, read source txuid %s account info error",
                                       txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!GenerateRegID(context, account)) {
        return false;
    }

    if(!account.nickid.IsEmpty()){
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, the account have nickid already!", txUid.ToString()),
                         UPDATE_ACCOUNT_FAIL, "nickid-exist");
    }


    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CNickIdRegisterTx::ExecuteTx, insufficient funds in account, txUid=%s",
                                       txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }

    CNickID  nick = CNickID(wasm::string_to_name(nickId.c_str()));
    account.nickid       = nick;
    if (!cw.accountCache.SaveAccount(account) || !cw.accountCache.SetNickId(account, context.height))
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
    result.push_back(Pair("txUid",         txUid.ToString()));
    result.push_back(Pair("nick_name",      nickId));

    return result;
}
