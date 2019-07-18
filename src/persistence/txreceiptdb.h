// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_RECEIPTDB_H
#define PERSIST_RECEIPTDB_H

#include "accounts/account.h"
#include "accounts/id.h"
#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"

#include <map>
#include <set>

#include <vector>

using namespace std;

class CTxReceiptDBCache {
public:
    CTxReceiptDBCache(CDBAccess *pDbAccess) : executeFailCache(pDbAccess){};

public:
    bool SetExecuteFail(const int32_t blockHeight, const uint256 txid, const uint8_t errorCode,
                        const string &errorMessage);
    bool GetExecuteFail(const int32_t blockHeight, vector<std::tuple<uint256, uint8_t, string> > &result);

    void Flush();

private:
/*  CDBScalarValueCache  prefixType             key                 value                        variable      */
/*  -------------------- --------------------- ------------------  ---------------------------  -------------- */
    // [prefix]{height}{txid} --> {error code, error message}
    CDBMultiValueCache<dbk::TX_EXECUTE_FAIL,    string,            std::pair<uint8_t, string> > executeFailCache;
};

#endif // PERSIST_RECEIPTDB_H