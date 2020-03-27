// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include "accountpermscleartx.h"

#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "commons/util/util.h"
#include "config/version.h"
#include "main.h"

bool CAccountPermsClearTx::CheckTx(CTxExecuteContext &context) {

    return true;
}
bool CAccountPermsClearTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE

    CAccount acct;
    if (!cw.accountCache.GetAccount(txUid, acct))
        return state.DoS(100, ERRORMSG("%s(), get tx account info failed! uid=%s",
                __func__, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    if (!acct.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees, CReceipt(ReceiptCode::NULL)))
        return state.DoS(100, ERRORMSG("CAccountPermsClearTx::ExecuteTx, deduct fees from regId=%s failed,",
                                       txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    acct.perms_sum = 0;
    if (!cw.accountCache.SetAccount(txUid, acct))
        return state.DoS(100, ERRORMSG("%s(), save tx account info failed! txuid=%s",
                __func__, txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

    if (!receipts.empty() && !cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CGovCoinTransferProposal::ExecuteProposal, save receipts error, kyeId=%s",
                                       GetHash().ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-receipts");

    return true;
}