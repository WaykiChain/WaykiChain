// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_RECEIPTDB_H
#define PERSIST_RECEIPTDB_H

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
/*  CDBScalarValueCache     prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    // best blockHash
    CDBScalarValueCache< dbk::BEST_BLOCKHASH,     CReceipt>        txReceiptCache;
     // <KeyID -> Account>
    CDBMultiValueCache< dbk::KEYID_ACCOUNT,        CKeyID,       CReceipt >       keyId2AccountCache;
};

#endif // PERSIST_RECEIPTDB_H