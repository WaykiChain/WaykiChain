// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mulsigtx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "persistence/contractdb.h"
#include "commons/util.h"
#include "main.h"
#include "miner/miner.h"
#include "config/version.h"

bool CMulsigTx::CheckTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;

    if (required < 1 || required > signaturePairs.size()) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, required keys invalid"),
                         REJECT_INVALID, "required-keys-invalid");
    }

    if (signaturePairs.size() > MAX_MULSIG_NUMBER) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, signature's number out of range"),
                         REJECT_INVALID, "signature-number-out-of-range");
    }

    if ((to_uid.type() != typeid(CRegID)) && (to_uid.type() != typeid(CKeyID)))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, desaddr type error"), REJECT_INVALID,
                         "desaddr-type-error");

    CAccount account;
    set<CPubKey> pubKeys;
    uint256 sighash = ComputeSignatureHash();
    uint8_t valid   = 0;
    for (const auto &item : signaturePairs) {
        if (!cw.accountCache.GetAccount(item.regid, account))
            return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, account: %s, read account failed",
                            item.regid.ToString()), REJECT_INVALID, "bad-getaccount");

        if (!item.signature.empty()) {
            if (!CheckSignatureSize(item.signature)) {
                return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, account: %s, signature size invalid",
                                item.regid.ToString()), REJECT_INVALID, "bad-tx-sig-size");
            }

            if (!VerifySignature(sighash, item.signature, account.owner_pubkey)) {
                return state.DoS(100,
                                 ERRORMSG("CMulsigTx::CheckTx, account: %s, VerifySignature failed",
                                          item.regid.ToString()),
                                 REJECT_INVALID, "bad-signscript-check");
            } else {
                ++valid;
            }
        }

        pubKeys.insert(account.owner_pubkey);
    }

    if (pubKeys.size() != signaturePairs.size()) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, duplicated account"), REJECT_INVALID,
                         "duplicated-account");
    }

    if (valid < required) {
        return state.DoS(
            100,
            ERRORMSG("CMulsigTx::CheckTx, not enough valid signatures, %u vs %u", valid, required),
            REJECT_INVALID, "not-enough-valid-signatures");
    }

    CMulsigScript script;
    script.SetMultisig(required, pubKeys);
    keyId = script.GetID();

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(CUserID(keyId), srcAccount))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, read multisig account: %s failed", keyId.ToAddress()),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}

bool CMulsigTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    CAccount srcAccount;
    CAccount desAccount;

    if (!cw.accountCache.GetAccount(CUserID(keyId), srcAccount)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, srcAccount)) {
        return false;
    }

    uint64_t minusValue = llFees + bcoins;
    if (!srcAccount.OperateBalance(SYMB::WICC, SUB_FREE, minusValue)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    uint64_t addValue = bcoins;
    if (!cw.accountCache.GetAccount(to_uid, desAccount)) {
        if (to_uid.type() == typeid(CKeyID)) {  // first involved in transaction
            desAccount.keyid = to_uid.get<CKeyID>();
        } else {
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    }

    if (!desAccount.OperateBalance(SYMB::WICC, ADD_FREE, addValue)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(to_uid, desAccount))
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account error, kyeId=%s",
                         desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    return true;
}

bool CMulsigTx::GenerateRegID(CTxExecuteContext &context, CAccount &account) {
    CRegID regId;
    if (context.pCw->accountCache.GetRegId(CUserID(keyId), regId)) {
        // account has regid already, return
        return true;
    }

    // generate a new regid for the account
    account.regid = CRegID(context.height, context.index);
    if (!context.pCw->accountCache.SaveAccount(account))
        return context.pState->DoS(100, ERRORMSG("CMulsigTx::GenerateRegID, save account info error"), WRITE_ACCOUNT_FAIL,
                         "bad-write-accountdb");

    return true;
}

string CSignaturePair::ToString() const {
    return strprintf("regId=%s, signature=%s", regid.ToString(), HexStr(signature.begin(), signature.end()));
}

Object CSignaturePair::ToJson() const {
    Object obj;
    obj.push_back(Pair("regid",     regid.ToString()));
    obj.push_back(Pair("signature", HexStr(signature.begin(), signature.end())));

    return obj;
}

string CMulsigTx::ToString(CAccountDBCache &accountCache) {
    string signatures;
    signatures += "signatures: ";
    for (const auto &item : signaturePairs) {
        signatures += strprintf("%s, ", item.ToString());
    }

    return strprintf(
        "txType=%s, hash=%s, ver=%d, required=%d, %s, desId=%s, bcoins=%ld, llFees=%llu, memo=%s,  valid_height=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, required, signatures, to_uid.ToString(), bcoins, llFees,
        HexStr(memo), valid_height);
}

Object CMulsigTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID desKeyId;
    accountCache.GetKeyId(to_uid, desKeyId);

    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("required_sigs",  required));
    Array signatureArray;
    CAccount account;
    std::set<CPubKey> pubKeys;
    for (const auto &item : signaturePairs) {
        signatureArray.push_back(item.ToJson());
        if (!accountCache.GetAccount(item.regid, account)) {
            LogPrint("ERROR", "CMulsigTx::ToJson, failed to get account info: %s\n",
                     item.regid.ToString());
            continue;
        }
        pubKeys.insert(account.owner_pubkey);
    }
    CMulsigScript script;
    script.SetMultisig(required, pubKeys);
    CKeyID scriptId = script.GetID();

    result.push_back(Pair("addr",           scriptId.ToAddress()));
    result.push_back(Pair("signatures",     signatureArray));
    result.push_back(Pair("to_uid",         to_uid.ToString()));
    result.push_back(Pair("to_addr",        desKeyId.ToAddress()));
    result.push_back(Pair("money",          bcoins));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("memo",           memo));
    result.push_back(Pair("valid_height",   valid_height));

    return result;
}

bool CMulsigTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    for (const auto &item : signaturePairs) {
        if (!cw.accountCache.GetKeyId(CUserID(item.regid), keyId))
            return false;
        keyIds.insert(keyId);
    }

    return true;
}