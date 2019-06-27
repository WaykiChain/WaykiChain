// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "mulsigtx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "persistence/contractdb.h"
#include "util.h"
#include "main.h"
#include "miner/miner.h"
#include "version.h"

string CSignaturePair::ToString() const {
    string str = strprintf("regId=%s, signature=%s", regId.ToString(),
                           HexStr(signature.begin(), signature.end()));
    return str;
}

Object CSignaturePair::ToJson() const {
    Object obj;
    obj.push_back(Pair("regid", regId.ToString()));
    obj.push_back(Pair("signature", HexStr(signature.begin(), signature.end())));

    return obj;
}

string CMulsigTx::ToString(CAccountDBCache &view) {
    string desId;
    if (desUserId.type() == typeid(CKeyID)) {
        desId = desUserId.get<CKeyID>().ToString();
    } else if (desUserId.type() == typeid(CRegID)) {
        desId = desUserId.get<CRegID>().ToString();
    }

    string signatures;
    signatures += "signatures: ";
    for (const auto &item : signaturePairs) {
        signatures += strprintf("%s, ", item.ToString());
    }
    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, required=%d, %s, desId=%s, bcoins=%ld, llFees=%ld, "
        "memo=%s,  nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, required, signatures, desId,
        bcoins, llFees, HexStr(memo), nValidHeight);

    return str;
}

Object CMulsigTx::ToJson(const CAccountDBCache &accountView) const {
    Object result;
    CAccountDBCache view(accountView);

    auto GetRegIdString = [&](CUserID const &userId) {
        if (userId.type() == typeid(CRegID)) {
            return userId.get<CRegID>().ToString();
        }
        return string("");
    };

    CKeyID desKeyId;
    view.GetKeyId(desUserId, desKeyId);

    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("tx_type", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("required_sigs", required));
    Array signatureArray;
    CAccount account;
    std::set<CPubKey> pubKeys;
    for (const auto &item : signaturePairs) {
        signatureArray.push_back(item.ToJson());
        if (!view.GetAccount(item.regId, account)) {
            LogPrint("ERROR", "CMulsigTx::ToJson, failed to get account info: %s\n",
                     item.regId.ToString());
            continue;
        }
        pubKeys.insert(account.pubKey);
    }
    CMulsigScript script;
    script.SetMultisig(required, pubKeys);
    CKeyID scriptId = script.GetID();

    result.push_back(Pair("addr", scriptId.ToAddress()));
    result.push_back(Pair("signatures", signatureArray));
    result.push_back(Pair("dest_regid", GetRegIdString(desUserId)));
    result.push_back(Pair("dest_addr", desKeyId.ToAddress()));
    result.push_back(Pair("money", bcoins));
    result.push_back(Pair("fees", llFees));
    result.push_back(Pair("memo", HexStr(memo)));
    result.push_back(Pair("valid_height", nValidHeight));

    return result;
}

bool CMulsigTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    for (const auto &item : signaturePairs) {
        if (!cw.accountCache.GetKeyId(CUserID(item.regId), keyId)) return false;
        keyIds.insert(keyId);
    }

    CKeyID desKeyId;
    if (!cw.accountCache.GetKeyId(desUserId, desKeyId)) return false;
    keyIds.insert(desKeyId);

    return true;
}

bool CMulsigTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    CAccount desAcct;
    bool generateRegID = false;

    if (!cw.accountCache.GetAccount(CUserID(keyId), srcAcct)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        CRegID regId;
        // If the source account does NOT have CRegID, need to generate a new CRegID.
        if (!cw.accountCache.GetRegId(CUserID(keyId), regId)) {
            srcAcct.regId = CRegID(nHeight, nIndex);
            generateRegID = true;
        }
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees + bcoins;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, minusValue)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (generateRegID) {
        if (!cw.accountCache.SaveAccount(srcAcct))
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    uint64_t addValue = bcoins;
    if (!cw.accountCache.GetAccount(desUserId, desAcct)) {
        if (desUserId.type() == typeid(CKeyID)) {  // target account does NOT have CRegID
            desAcct.keyId    = desUserId.get<CKeyID>();
            desAcctLog.keyId = desAcct.keyId;
        } else {
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    } else {  // target account has NO CAccount(first involved in transacion)
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateBalance(CoinType::WICC, ADD_VALUE, addValue)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(desUserId, desAcct))
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account error, kyeId=%s",
                         desAcct.keyId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.accountLogs.push_back(desAcctLog);
    cw.txUndo.txHash = GetHash();

    vector<CUserID> uids;
    for (const auto &item : signaturePairs) {
        uids.push_back(CUserID(item.regId));
    }
    uids.push_back(desUserId);

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, uids)) return false;

    return true;
}

bool CMulsigTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;

        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CMulsigTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CMulsigTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (account.IsEmptyValue() && account.regId.IsEmpty()) {
            cw.accountCache.EraseAccountByKeyId(userId);
        } else if (account.regId == CRegID(nHeight, nIndex)) {
            // If the CRegID was generated by this MULSIG_TX, need to remove CRegID.
            CPubKey empPubKey;
            account.pubKey      = empPubKey;
            account.minerPubKey = empPubKey;
            account.regId.Clean();

            if (!cw.accountCache.SetAccount(userId, account)) {
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }

            cw.accountCache.EraseKeyId(CRegID(nHeight, nIndex));
        } else {
            if (!cw.accountCache.SetAccount(userId, account)) {
                return state.DoS(100,
                                 ERRORMSG("CMulsigTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }
    }

    return true;
}

bool CMulsigTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;

    if (required < 1 || required > signaturePairs.size()) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, required keys invalid"),
                         REJECT_INVALID, "required-keys-invalid");
    }

    if (signaturePairs.size() > kMultisigNumberLimit) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, signature's number out of range"),
                         REJECT_INVALID, "signature-number-out-of-range");
    }

    if ((desUserId.type() != typeid(CRegID)) && (desUserId.type() != typeid(CKeyID)))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, desaddr type error"), REJECT_INVALID,
                         "desaddr-type-error");

    CAccount account;
    set<CPubKey> pubKeys;
    uint256 sighash = ComputeSignatureHash();
    uint8_t valid   = 0;
    for (const auto &item : signaturePairs) {
        if (!cw.accountCache.GetAccount(item.regId, account))
            return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, account: %s, read account failed",
                            item.regId.ToString()), REJECT_INVALID, "bad-getaccount");

        if (!item.signature.empty()) {
            if (!CheckSignatureSize(item.signature)) {
                return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, account: %s, signature size invalid",
                                item.regId.ToString()), REJECT_INVALID, "bad-tx-sig-size");
            }

            if (!VerifySignature(sighash, item.signature, account.pubKey)) {
                return state.DoS(100,
                                 ERRORMSG("CMulsigTx::CheckTx, account: %s, VerifySignature failed",
                                          item.regId.ToString()),
                                 REJECT_INVALID, "bad-signscript-check");
            } else {
                ++valid;
            }
        }

        pubKeys.insert(account.pubKey);
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
