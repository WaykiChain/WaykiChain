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
    TxType      tx_type;
    CUserID     from_uid;
    CUserID     to_uid;
    TokenSymbol coin_symbol;
    uint64_t    send_amount;

public:
    CReceipt() {};

    CReceipt(TxType txType, CUserID &fromUid, CUserID &toUid, TokenSymbol coinSymbol, uint64_t sendAmount)
        : tx_type(txType), from_uid(fromUid), to_uid(toUid), coin_symbol(coinSymbol), send_amount(sendAmount){};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &)tx_type);
        READWRITE(from_uid);
        READWRITE(to_uid);
        READWRITE((uint8_t &)coin_symbol);
        READWRITE(VARINT(send_amount));
    )
};

#endif //ENTITIES_RECEIPT_H