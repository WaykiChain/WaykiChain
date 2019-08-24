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

static CUserID nullId;

class CReceipt {
public:
    CUserID     from_uid;
    CUserID     to_uid;
    TokenSymbol coin_symbol;
    uint64_t    coin_amount;
    string      memo;

public:
    CReceipt() {}

    CReceipt(const CUserID &fromUid, const CUserID &toUid, const TokenSymbol &coinSymbol, const uint64_t coinAmount,
             const string &memoIn)
        : from_uid(fromUid), to_uid(toUid), coin_symbol(coinSymbol), coin_amount(coinAmount), memo(memoIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(from_uid);
        READWRITE(to_uid);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(memo);
    )

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(Pair("from_uid",      from_uid.ToString()));
        obj.push_back(Pair("to_uid",        to_uid.ToString()));
        obj.push_back(Pair("coin_symbol",   coin_symbol));
        obj.push_back(Pair("coin_amount",   coin_amount));
        obj.push_back(Pair("memo",          memo));

        return obj;
    }
};

#endif  // ENTITIES_RECEIPT_H