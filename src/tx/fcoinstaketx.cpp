// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcoinstaketx.h"

#include "configuration.h"
#include "main.h"

bool CFcoinStakeTx::CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, tx fee out of range"),
                        REJECT_INVALID, "bad-tx-fee-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, tx fee smaller than MinTxFee"),
                        REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    if (!CheckFundCoinMoneyRange(fcoinsToStake)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, fcoinsToStake out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-toolarge");
    }
    if (!CheckSignatureSize(signature)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, tx signature size invalid"),
                        REJECT_INVALID, "bad-tx-sig-size");
    }

    // check signature script
    uint256 sighash = ComputeSignatureHash();
    if (!CheckSignScript(sighash, signature, txUid.get<CPubKey>()))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, tx signature error "),
                        REJECT_INVALID, "bad-tx-signature");

    return true;
}

bool CFcoinStakeTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                    int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {

    CAccount account;
    if (!view.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    // check if account has sufficient fcoins to be a price feeder
    if (account.fcoins < kDefaultPriceFeedFcoinsMin)
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, not sufficient fcoins(%d) in account (%s)",
                        account.fcoins, txUid.ToString()), FCOIN_STAKE_FAIL, "not-sufficiect-fcoins");

    CAccountLog acctLog(account);
    // update the price state accordingly:
    int64_t fcoins = account.stakedFcoins + fcoinsToStake;
    if (!CheckFundCoinMoneyRange(fcoins)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, fcoinsToStake out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-toolarge");
    }
    account.stakedFcoins = fcoins;

    if (!view.SaveAccountInfo(account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, write source addr %s account info error",
                        account.regID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    txundo.vAccountLog.push_back(acctLog);
    txundo.txHash = GetHash();
    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(txUid, sendKeyId))
            return ERRORMSG("CAccountRegisterTx::ExecuteTx, get keyid by userId error!");

        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CFcoinStakeTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                    CScriptDBViewCache &scriptDB) {
    //TODO
    return true;

}


Object CFcoinStakeTx::ToJson(const CAccountViewCache &AccountView) const {
  //TODO
  return Object();
}

bool CFcoinStakeTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    //TODO
    return true;
}
