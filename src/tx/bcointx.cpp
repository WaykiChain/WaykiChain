// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "bcointx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "persistence/contractdb.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

string CBaseCoinTransferTx::ToString(CAccountViewCache &view) const {
    string srcId;
    if (txUid.type() == typeid(CPubKey)) {
        srcId = txUid.get<CPubKey>().ToString();
    } else if (txUid.type() == typeid(CRegID)) {
        srcId = txUid.get<CRegID>().ToString();
    }

    string desId;
    if (toUid.type() == typeid(CKeyID)) {
        desId = toUid.get<CKeyID>().ToString();
    } else if (toUid.type() == typeid(CRegID)) {
        desId = toUid.get<CRegID>().ToString();
    }

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, srcId=%s, desId=%s, bcoins=%ld, llFees=%ld, memo=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString().c_str(), nVersion, srcId.c_str(), desId.c_str(),
        bcoins, llFees, HexStr(memo).c_str(), nValidHeight);

    return str;
}

Object CBaseCoinTransferTx::ToJson(const CAccountViewCache &AccountView) const {
    Object result;
    CAccountViewCache view(AccountView);

    auto GetRegIdString = [&](CUserID const &userId) {
        if (userId.type() == typeid(CRegID))
            return userId.get<CRegID>().ToString();
        return string("");
    };

    CKeyID srcKeyId, desKeyId;
    view.GetKeyId(txUid, srcKeyId);
    view.GetKeyId(toUid, desKeyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("regid",          GetRegIdString(txUid)));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("dest_regid",     GetRegIdString(toUid)));
    result.push_back(Pair("dest_addr",      desKeyId.ToAddress()));
    result.push_back(Pair("money",          bcoins));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("memo",           HexStr(memo)));
    result.push_back(Pair("valid_height",   nValidHeight));

    return result;
}

bool CBaseCoinTransferTx::GetInvolvedKeyIds(set<CKeyID> &vAddr, CAccountViewCache &view,
                           CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(txUid, keyId))
        return false;

    vAddr.insert(keyId);
    CKeyID desKeyId;
    if (!view.GetKeyId(toUid, desKeyId))
        return false;

    vAddr.insert(desKeyId);
    return true;
}

bool CBaseCoinTransferTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                          CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                          CScriptDBViewCache &scriptDB) {
    CAccount srcAcct;
    CAccount desAcct;
    bool generateRegID = false;

    if (!view.GetAccount(txUid, srcAcct))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    else {
        if (txUid.type() == typeid(CPubKey)) {
            srcAcct.pubKey = txUid.get<CPubKey>();

            CRegID regId;
            // If the source account does NOT have CRegID, need to generate a new CRegID.
            if (!view.GetRegId(txUid, regId)) {
                srcAcct.regID = CRegID(nHeight, nIndex);
                generateRegID = true;
            }
        }
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees + bcoins;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, minusValue)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    }

    if (generateRegID) {
        if (!view.SaveAccountInfo(srcAcct))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!view.SetAccount(CUserID(srcAcct.keyID), srcAcct))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    if (!view.GetAccount(toUid, desAcct)) {
        if (toUid.type() == typeid(CKeyID)) {  // target account does NOT have CRegID
            desAcct.keyID    = toUid.get<CKeyID>();
            desAcctLog.keyID = desAcct.keyID;
        } else {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    } else {  // target account has NO CAccount(first involved in transacion)
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateBalance(CoinType::WICC, ADD_VALUE, bcoins)) {
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!view.SetAccount(toUid, desAcct))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::ExecuteTx, save account error, kyeId=%s",
                         desAcct.keyID.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-save-account");

    txundo.vAccountLog.push_back(srcAcctLog);
    txundo.vAccountLog.push_back(desAcctLog);
    txundo.txHash = GetHash();

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        CKeyID revKeyId;
        if (!view.GetKeyId(txUid, sendKeyId))
            return ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get keyid by txUid error!");

        if (!view.GetKeyId(toUid, revKeyId))
            return ERRORMSG("CBaseCoinTransferTx::ExecuteTx, get keyid by toUid error!");

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);

        if (!scriptDB.SetTxHashByAddress(revKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CBaseCoinTransferTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                              CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                              CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (account.IsEmptyValue() &&
            (!account.pubKey.IsFullyValid() || account.pubKey.GetKeyId() != account.keyID)) {
            view.EraseAccountByKeyId(userId);

        } else if (account.regID == CRegID(nHeight, nIndex)) {
            // If the CRegID was generated by this BCOIN_TRANSFER_TX, need to remove CRegID.
            CPubKey empPubKey;
            account.pubKey      = empPubKey;
            account.minerPubKey = empPubKey;
            account.regID.Clean();
            if (!view.SetAccount(userId, account)) {
                return state.DoS(100,
                                 ERRORMSG("CBaseCoinTransferTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }

            view.EraseKeyId(CRegID(nHeight, nIndex));
        } else {
            if (!view.SetAccount(userId, account)) {
                return state.DoS(100,
                                 ERRORMSG("CBaseCoinTransferTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::UndoExecuteTx, undo scriptdb data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    return true;
}

bool CBaseCoinTransferTx::CheckTx(CValidationState &state, CAccountViewCache &view,
                        CScriptDBViewCache &scriptDB) {
    if (memo.size() > kCommonTxMemoMaxSize)
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, memo's size too large"), REJECT_INVALID,
                         "memo-size-toolarge");

    if ((txUid.type() != typeid(CRegID)) && (txUid.type() != typeid(CPubKey)))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, srcaddr type error"), REJECT_INVALID,
                         "srcaddr-type-error");

    if ((toUid.type() != typeid(CRegID)) && (toUid.type() != typeid(CKeyID)))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, desaddr type error"), REJECT_INVALID,
                         "desaddr-type-error");

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-commontx-publickey");

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, tx fees out of money range"),
                         REJECT_INVALID, "bad-appeal-fees-toolarge");

    if (!CheckMinTxFee(llFees))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, tx fees smaller than MinTxFee"),
                         REJECT_INVALID, "bad-tx-fees-toosmall");

    CAccount srcAccount;
    if (!view.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if ((txUid.type() == typeid(CRegID)) && !srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, account pubkey not registered"),
                         REJECT_INVALID, "bad-account-unregistered");

    if (!CheckSignatureSize(signature))
            return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, signature size invalid"),
                             REJECT_INVALID, "bad-tx-sig-size");

    uint256 sighash = ComputeSignatureHash();
    CPubKey pubKey =
        txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey;
    if (!CheckSignScript(sighash, signature, pubKey))
        return state.DoS(100, ERRORMSG("CBaseCoinTransferTx::CheckTx, CheckSignScript failed"),
                         REJECT_INVALID, "bad-signscript-check");

    return true;
}
