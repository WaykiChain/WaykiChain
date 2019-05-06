// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php


#include "pricefeed.h"
#include "commons/serialize.h"
#include "tx.h"
#include "txdb.h"
#include "crypto/hash.h"
#include "util.h"
#include "database.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "core.h"
#include "miner/miner.h"
#include "version.h"

bool CPriceFeedTx::CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, tx fee out of range"),
            REJECT_INVALID, "bad-tx-fee-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    if (!CheckSignatureSize(signature)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, signature size invalid"),
            REJECT_INVALID, "bad-tx-sig-size");
    }

    // check signature script
    uint256 sighash = ComputeSignatureHash();
    if (!CheckSignScript(sighash, signature, txUid.get<CPubKey>()))
        return state.DoS(100, ERRORMSG("CAccountRegisterTx::CheckTx, register tx signature error "),
            REJECT_INVALID, "bad-regtx-signature");

    return true;
}

bool CPriceFeedTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                    int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {

}

bool CPriceFeedTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                    CScriptDBViewCache &scriptDB) {

}


Object CPriceFeedTx::ToJson(const CAccountViewCache &AccountView) const {

}

bool CPriceFeedTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

}