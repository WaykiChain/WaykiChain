// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_RECEIPT_H
#define ENTITIES_RECEIPT_H

#include "crypto/hash.h"
#include "account.h"
#include "config/txbase.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

class CReceipt {
public:
    TxType      txType;
    CUserID     fromUid;
    CUserID     toUid;
    CoinType    coinType;
    uint64_t    sendAmount;

public:
    CReceipt(TxType &txTypeIn, CUserID &fromUidIn, CUserID &toUidIn,
            CoinType &coinTypeIn, uint64_t &sendAmountIn) :
            txType(txTypeIn), fromUid(fromUidIn), toUid(toUidIn),
            coinType(coinTypeIn), sendAmount(sendAmountIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) txType);
        READWRITE(fromUid);
        READWRITE(toUid);
        READWRITE((uint8_t &) coinType);
        READWRITE(VARINT(sendAmount));)
};

#endif //ENTITIES_RECEIPT_H