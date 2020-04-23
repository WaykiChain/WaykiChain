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
    sp_tx_account->perms_sum = 0;
    return true;
}