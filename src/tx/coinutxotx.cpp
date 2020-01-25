// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinutxotx.h"
#include "main.h"
#include <string>
#include <cstdarg>

bool CCoinUTXOTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid);
    if (!CheckFee(context)) return false;

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    if (prior_utxo_txid == uint256()) { //first-time utxo
        if (utxo.is_null)
            return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, utxo is null!",
                                REJECT_INVALID, "utxo-is-null"));
        
        if (utxo.coin_amount == 0)
                return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, utxo.coin_amount is zero!",
                                REJECT_INVALID, "zero-utxo-coin-amount"));

    } else { //pointing to an existing prior utxo for consumption
        //load prior utxo
        CCoinUTXOTx priorUtxoTx;
        uint64_t priorTxBlockHeight;
        if (!context.pCw->txUtxoCache.GetUtxoTx(prior_utxo_txid, priorTxBlockHeight, priorUtxoTx)) {
            return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, load prior utxo error!",
                                REJECT_INVALID, "load-prior-utxo-err"));
        }

        if (!utxo.is_null) {
            if (utxo.coin_amount == 0)
                return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, utxo.coin_amount is zero!",
                                REJECT_INVALID, "zero-utxo-coin-amount"));

            // check if e prior UTXO is open to spend now
            if (context.height < priorTxBlockHeight + priorUtxoTx.utxo.lock_duration) {
                return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, prior utxo is being locked!",
                                REJECT_INVALID, "prior-utxo-locked"));
            }

            // check if e prior UTXO has sufficient fund for subsequent UTXO
            if (!utxo.is_null && utxo.coin_amount > priorUtxoTx.utxo.coin_amount) {
                return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, prior utxo fund insufficient!",
                                REJECT_INVALID, "prior-utxo-fund-insufficient"));
            }
        } else { //since utxo is null, it is for withdrawl only
            if (!priorUtxoTx.txUid.IsEmpty() && priorUtxoTx.utxo.to_uid != txUid) {
                return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, prior utxo toUid != txUid!",
                            REJECT_INVALID, "priro-utxo-wrong-txUid"));
            }

            if (priorUtxoTx.utxo.htlc_cond.secret_hash != uint256()) { //secret must be supplied
                //verify the height for collection
                string text = format("%s%s%d", priorUtxoTx.txUid.ToString(), prior_utxo_secret, priorUtxoTx.valid_height);
                // string hash = SHA256(SHA256(text.c_str(), sizeof(text), NULL)); FIXME!!!
                uint256 hash;
                if (hash != priorUtxoTx.utxo.htlc_cond.secret_hash) {
                    return state.DoS(100, ERRORMSG("CCoinUTXOTx::CheckTx, supplied wrong secret to prior utxo",
                                REJECT_INVALID, "wrong-secret-to-prior-utxo"));
                }
            }
            
        }
    }

    CPubKey pubKey = (txUid.is<CPubKey>() ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CCoinUTXOTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinUTXOTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!GenerateRegID(context, srcAccount)) {
        return false;
    }

    if (!srcAccount.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCoinUTXOTx::ExecuteTx, insufficient coin_amount in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coin_amount");
    }

    vector<CReceipt> receipts;

   

    if (!cw.accountCache.SaveAccount(srcAccount))
        return state.DoS(100, ERRORMSG("CCoinUTXOTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!receipts.empty() && !cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");

    return true;
}

string CCoinUTXOTx::ToString(CAccountDBCache &accountCache) {
    string coinUtxoStr = "";

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, "
        "valid_height=%d, transfers=[%s], memo=%s",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees,
        valid_height, coinUtxoStr, HexStr(memo));
}

Object CCoinUTXOTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    result.push_back(Pair("memo",        memo));

    return result;
}
