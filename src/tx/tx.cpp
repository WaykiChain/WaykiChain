#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <algorithm>

#include "tx.h"

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include "commons/serialize.h"
#include "txdb.h"
#include "crypto/hash.h"
#include "util.h"
#include "database.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "core.h"
#include "miner/miner.h"
#include "version.h"

using namespace json_spirit;

const string COperVoteFund::voteOperTypeArray[3] = {"NULL_OPER", "ADD_FUND", "MINUS_FUND"};

string GetTxType(unsigned char txType) {
    auto it = kTxTypeMap.find(txType);
    if (it != kTxTypeMap.end())
        return it->second;
    else
        return "";
}


bool CBaseTx::IsValidHeight(int nCurrHeight, int nTxCacheHeight) const {
    if(BLOCK_REWARD_TX == nTxType)
        return true;

    if (nValidHeight > nCurrHeight + nTxCacheHeight / 2)
        return false;

    if (nValidHeight < nCurrHeight - nTxCacheHeight / 2)
        return false;

    return true;
}

uint64_t CBaseTx::GetFuel(int nfuelRate) {
    uint64_t llFuel = ceil(nRunStep/100.0f) * nfuelRate;
    if (CONTRACT_DEPLOY_TX == nTxType) {
        if (llFuel < 1 * COIN) {
            llFuel = 1 * COIN;
        }
    }
    return llFuel;
}

int CBaseTx::GetFuelRate(CScriptDBViewCache &scriptDB) {
    if (nFuelRate > 0)
        return nFuelRate;

    CDiskTxPos postx;
    if (scriptDB.ReadTxIndex(GetHash(), postx)) {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        CBlockHeader header;
        try {
            file >> header;
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
        nFuelRate = header.GetFuelRate();
    } else {
        nFuelRate = GetElementForBurn(chainActive.Tip());
    }

    return nFuelRate;
}

// check the fees must be more than nMinTxFee
bool CBaseTx::CheckMinTxFee(const uint64_t llFees) const {
    if (GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2 )
        return llFees >= nMinTxFee;

    return true;
}

// transactions should check the signagure size before verifying signature
bool CBaseTx::CheckSignatureSize(const vector<unsigned char> &signature) const {
    return signature.size() > 0 && signature.size() < MAX_BLOCK_SIGNATURE_SIZE;
}

string CBaseTx::ToString(CAccountViewCache &view) const {
    string str = strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString().c_str(), nVersion, txUid.get<CPubKey>().ToString(),
        llFees, txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight);

    return str;
}

string CTxUndo::ToString() const {
    vector<CAccountLog>::const_iterator iterLog = vAccountLog.begin();
    string strTxHash("txHash:");
    strTxHash += txHash.GetHex();
    strTxHash += "\n";
    string str("  list account Log:\n");
    for (; iterLog != vAccountLog.end(); ++iterLog) {
        str += iterLog->ToString();
    }
    strTxHash += str;
    vector<CScriptDBOperLog>::const_iterator iterDbLog = vScriptOperLog.begin();
    string strDbLog(" list script db Log:\n");
    for (; iterDbLog !=  vScriptOperLog.end(); ++iterDbLog) {
        strDbLog += iterDbLog->ToString();
    }
    strTxHash += strDbLog;
    return strTxHash;
}

bool CTxUndo::GetAccountOperLog(const CKeyID &keyId, CAccountLog &accountLog) {
    vector<CAccountLog>::iterator iterLog = vAccountLog.begin();
    for (; iterLog != vAccountLog.end(); ++iterLog) {
        if (iterLog->keyID == keyId) {
            accountLog = *iterLog;
            return true;
        }
    }
    return false;
}

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

string CMulsigTx::ToString(CAccountViewCache &view) const {
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

Object CMulsigTx::ToJson(const CAccountViewCache &AccountView) const {
    Object result;
    CAccountViewCache view(AccountView);

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

bool CMulsigTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view,
                           CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    for (const auto &item : signaturePairs) {
        if (!view.GetKeyId(CUserID(item.regId), keyId)) return false;
        vAddr.insert(keyId);
    }

    CKeyID desKeyId;
    if (!view.GetKeyId(desUserId, desKeyId)) return false;
    vAddr.insert(desKeyId);

    return true;
}

bool CMulsigTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                          CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                          CScriptDBViewCache &scriptDB) {
    CAccount srcAcct;
    CAccount desAcct;
    bool generateRegID = false;

    if (!view.GetAccount(CUserID(keyId), srcAcct)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        CRegID regId;
        // If the source account does NOT have CRegID, need to generate a new CRegID.
        if (!view.GetRegId(CUserID(keyId), regId)) {
            srcAcct.regID = CRegID(nHeight, nIndex);
            generateRegID = true;
        }
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees + bcoins;
    if (!srcAcct.OperateAccount(MINUS_FREE, minusValue, nHeight))
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (generateRegID) {
        if (!view.SaveAccountInfo(srcAcct.regID, srcAcct.keyID, srcAcct))
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!view.SetAccount(CUserID(srcAcct.keyID), srcAcct))
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    uint64_t addValue = bcoins;
    if (!view.GetAccount(desUserId, desAcct)) {
        if (desUserId.type() == typeid(CKeyID)) {  // target account does NOT have CRegID
            desAcct.keyID    = desUserId.get<CKeyID>();
            desAcctLog.keyID = desAcct.keyID;
        } else {
            return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    } else {  // target account has NO CAccount(first involved in transacion)
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateAccount(ADD_FREE, addValue, nHeight))
        return state.DoS(100, ERRORMSG("CMulsigTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");

    if (!view.SetAccount(desUserId, desAcct))
        return state.DoS(100,
                         ERRORMSG("CMulsigTx::ExecuteTx, save account error, kyeId=%s",
                                  desAcct.keyID.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-save-account");

    txundo.vAccountLog.push_back(srcAcctLog);
    txundo.vAccountLog.push_back(desAcctLog);
    txundo.txHash = GetHash();

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        CKeyID revKeyId;

        for (const auto &item : signaturePairs) {
            if (!view.GetKeyId(CUserID(item.regId), sendKeyId))
                return ERRORMSG("CBaseCoinTransferTx::CMulsigTx, get keyid by srcUserId error!");

            if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                             operAddressToTxLog))
                return false;
            txundo.vScriptOperLog.push_back(operAddressToTxLog);
        }

        if (!view.GetKeyId(desUserId, revKeyId))
            return ERRORMSG("CBaseCoinTransferTx::CMulsigTx, get keyid by desUserId error!");

        if (!scriptDB.SetTxHashByAddress(revKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CMulsigTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                              CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                              CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;

        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CMulsigTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CMulsigTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (account.IsEmptyValue() && account.regID.IsEmpty()) {
            view.EraseAccount(userId);
        } else if (account.regID == CRegID(nHeight, nIndex)) {
            // If the CRegID was generated by this MULSIG_TX, need to remove CRegID.
            CPubKey empPubKey;
            account.pubKey      = empPubKey;
            account.minerPubKey = empPubKey;
            account.regID.Clean();

            if (!view.SetAccount(userId, account)) {
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }

            view.EraseId(CRegID(nHeight, nIndex));
        } else {
            if (!view.SetAccount(userId, account)) {
                return state.DoS(100,
                                 ERRORMSG("CMulsigTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CMulsigTx::UndoExecuteTx, undo scriptdb data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    return true;
}

bool CMulsigTx::CheckTx(CValidationState &state, CAccountViewCache &view,
                          CScriptDBViewCache &scriptDB) {
    if (memo.size() > kCommonTxMemoMaxSize)
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, memo's size too large"),
                         REJECT_INVALID, "memo-size-toolarge");

    if (required < 1 || required > signaturePairs.size()) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, required keys invalid"),
                         REJECT_INVALID, "required-keys-invalid");
    }

    if (signaturePairs.size() > kMultisigNumberThreshold) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, signature's number out of range"),
                         REJECT_INVALID, "signature-number-out-of-range");
    }

    if ((desUserId.type() != typeid(CRegID)) && (desUserId.type() != typeid(CKeyID)))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, desaddr type error"), REJECT_INVALID,
                         "desaddr-type-error");

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, tx fees out of money range"),
                         REJECT_INVALID, "bad-appeal-fees-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, tx fees smaller than MinTxFee"),
                         REJECT_INVALID, "bad-tx-fees-toosmall");
    }

    CAccount account;
    set<CPubKey> pubKeys;
    uint256 sighash = SignatureHash();
    uint8_t valid   = 0;
    for (const auto &item : signaturePairs) {
        if (!view.GetAccount(item.regId, account))
            return state.DoS(100,
                             ERRORMSG("CMulsigTx::CheckTx, account: %s, read account failed",
                                      item.regId.ToString()),
                             REJECT_INVALID, "bad-getaccount");

        if (!item.signature.empty()) {
            if (!CheckSignatureSize(item.signature)) {
                return state.DoS(100,
                                 ERRORMSG("CMulsigTx::CheckTx, account: %s, signature size invalid",
                                          item.regId.ToString()),
                                 REJECT_INVALID, "bad-tx-sig-size");
            }

            if (!CheckSignScript(sighash, item.signature, account.pubKey)) {
                return state.DoS(100,
                                 ERRORMSG("CMulsigTx::CheckTx, account: %s, CheckSignScript failed",
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
    if (!view.GetAccount(CUserID(keyId), srcAccount))
        return state.DoS(100, ERRORMSG("CMulsigTx::CheckTx, read multisig account: %s failed", keyId.ToAddress()),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");

    return true;
}
