// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mulsigtx.h"

#include "commons/serialize.h"
#include "commons/util/util.h"
#include "config/version.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"

bool CMulsigTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE
    if (!CheckFee(context)) return false;
    IMPLEMENT_CHECK_TX_MEMO;

    if (transfers.empty() || transfers.size() > MAX_TRANSFER_SIZE) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, transfers is empty or too large count=%d than %d",
            transfers.size(), MAX_TRANSFER_SIZE),
                        REJECT_INVALID, "invalid-transfers");
    }

    for (size_t i = 0; i < transfers.size(); i++) {
        IMPLEMENT_CHECK_TX_REGID_OR_KEYID(transfers[i].to_uid);
        if (!cw.assetCache.CheckAsset(transfers[i].coin_symbol))
            return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, transfers[%d], invalid coin_symbol=%s", i,
                            transfers[i].coin_symbol), REJECT_INVALID, "invalid-coin-symbol");

        if (transfers[i].coin_amount < DUST_AMOUNT_THRESHOLD)
            return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, transfers[%d], dust amount, %llu < %llu", i,
                            transfers[i].coin_amount, DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

        if (!CheckBaseCoinRange(transfers[i].coin_amount))
            return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, transfers[%d], coin_amount=%llu out of valid range", i,
                            transfers[i].coin_amount), REJECT_DUST, "invalid-coin-amount");
    }

    uint64_t minFee;
    if (!GetTxMinFee(nTxType, context.height, fee_symbol, minFee)) { assert(false); /* has been check before */ }

    if (llFees < transfers.size() * minFee) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, tx fee too small (height: %d, fee symbol: %s, fee: %llu)",
                         context.height, fee_symbol, llFees), REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    if (required < 1 || required > signaturePairs.size()) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, required keys invalid"), REJECT_INVALID,
                         "required-keys-invalid");
    }

    if (signaturePairs.size() > MAX_MULSIG_NUMBER) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, signature's number out of range"), REJECT_INVALID,
                         "signature-number-out-of-range");
    }

    CAccount account;
    set<CPubKey> pubKeys;
    uint256 sighash = GetHash();
    uint8_t valid   = 0;
    for (const auto &item : signaturePairs) {
        if (!cw.accountCache.GetAccount(item.regid, account))
            return state.DoS(100,
                             ERRORMSG("CMulsigTx::CheckTx, account: %s, read account failed", item.regid.ToString()),
                             REJECT_INVALID, "bad-getaccount");

        if (!item.signature.empty()) {
            if (!CheckSignatureSize(item.signature)) {
                return state.DoS(
                    100, ERRORMSG("CMulsigTx::CheckTx, account: %s, signature size invalid", item.regid.ToString()),
                    REJECT_INVALID, "bad-tx-sig-size");
            }

            if (!::VerifySignature(sighash, item.signature, account.owner_pubkey)) {
                return state.DoS(
                    100, ERRORMSG("CMulsigTx::CheckTx, account: %s, VerifySignature failed", item.regid.ToString()),
                    REJECT_INVALID, "bad-signscript-check");
            } else {
                ++valid;
            }
        }

        pubKeys.insert(account.owner_pubkey);
    }

    if (pubKeys.size() != signaturePairs.size()) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, duplicated account"), REJECT_INVALID, "duplicated-account");
    }

    if (valid < required) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, not enough valid signatures, %u vs %u", valid, required),
                         REJECT_INVALID, "not-enough-valid-signatures");
    }

    CMulsigScript script;
    script.SetMultisig(required, pubKeys);
    keyId = script.GetID();

    CAccount txAccount;
    if (!cw.accountCache.GetAccount(CUserID(keyId), txAccount))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, read multisig account: %s failed", keyId.ToAddress()),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}

bool CMulsigTx::ExecuteTx(CTxExecuteContext &context) {

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
        return context.pState->DoS(100, ERRORMSG("CMulsigTx::GenerateRegID, save account info error"),
                                   WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

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

    string transferStr = "";
    for (const auto &transfer : transfers) {
        if (!transferStr.empty()) transferStr += ",";
        transferStr += strprintf("{%s}", transfer.ToString(accountCache));
    }

    return strprintf(
        "txType=%s, hash=%s, ver=%d, required=%d, %s, fee_symbol=%s, llFees=%llu, valid_height=%d, transfers=[%s], "
        "memo=%s",
        GetTxType(nTxType), GetHash().ToString(), nVersion, required, signatures, fee_symbol, llFees, valid_height,
        HexStr(memo), transferStr);
}

Object CMulsigTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    Array signatureArray;
    CAccount account;
    std::set<CPubKey> pubKeys;
    for (const auto &item : signaturePairs) {
        signatureArray.push_back(item.ToJson());
        if (!accountCache.GetAccount(item.regid, account)) {
            LogPrint(BCLog::ERROR, "CMulsigTx::ToJson, failed to get account info: %s\n", item.regid.ToString());
            continue;
        }
        pubKeys.insert(account.owner_pubkey);
    }
    CMulsigScript script;
    script.SetMultisig(required, pubKeys);
    CKeyID scriptId = script.GetID();

    Array transferArray;
    for (const auto &transfer : transfers) {
        transferArray.push_back(transfer.ToJson(accountCache));
    }


    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("required_sigs",  required));
    result.push_back(Pair("signatures",     signatureArray));
    result.push_back(Pair("from_addr",      scriptId.ToAddress()));
    result.push_back(Pair("fee_symbol",     fee_symbol));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   valid_height));
    result.push_back(Pair("transfers",      transferArray));
    result.push_back(Pair("memo",           memo));

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