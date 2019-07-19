// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ACCOUNTS_ASSET_H
#define ACCOUNTS_ASSET_H

#include "crypto/hash.h"
#include "account.h"
#include "config/txbase.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

class CReceipt: uint8_t {
public:
    TxType      txType;
    CUserID     fromUid;
    CUserID     toUid;
    CoinType    sendCoinType;
    uint64_t    sendCoinAmount;

public:
    CReceipt(TxType &txTypeIn, CUserID &receipientUidIn, CoinOpType opTypeIn,
            CoinType &coinTypeIn, uint64_t &coinAmountIn) :
            txType(txTypeIn), receipientUid(receipientUidIn), opType(opTypeIn),
            coinType(coinTypeIn), coinAmount(coinAmountIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t &) txType);
        READWRITE(receipientUid);
        READWRITE((uint8_t &) opType);
        READWRITE((uint8_t &) coinType);
        READWRITE(VARINT(coinAmount));)
};

#endif //ACCOUNTS_ASSET_H