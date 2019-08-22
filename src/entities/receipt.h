// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_RECEIPT_H
#define ENTITIES_RECEIPT_H

#include "config/txbase.h"
#include "crypto/hash.h"
#include "entities/asset.h"
#include "entities/id.h"
#include "json/json_spirit_utils.h"

class CReceipt {
public:
    TxType      tx_type;
    CUserID     from_uid;
    CUserID     to_uid;
    TokenSymbol coin_symbol;
    uint64_t    coin_amount;

public:
    CReceipt() {}

    CReceipt(const TxType txType, const CUserID &fromUid, const CUserID &toUid, const TokenSymbol &coinSymbol,
             const uint64_t coinAmount)
        : tx_type(txType), from_uid(fromUid), to_uid(toUid), coin_symbol(coinSymbol), coin_amount(coinAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &)tx_type);
        READWRITE(from_uid);
        READWRITE(to_uid);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
    )

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(Pair("tx_type",       std::get<0>(kTxFeeTable.at(tx_type))));
        obj.push_back(Pair("from_uid",      from_uid.ToString()));
        obj.push_back(Pair("to_uid",        to_uid.ToString()));
        obj.push_back(Pair("coin_symbol",   coin_symbol));
        obj.push_back(Pair("coin_amount",   coin_amount));

        return obj;
    }
};

#endif //ENTITIES_RECEIPT_H