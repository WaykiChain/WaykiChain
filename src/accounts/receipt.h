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

class CReceipt {
public:
    TxType      txType;
    uint256     fromAddress;
    uint256     toAddress;
    CoinType    transferCoinType;
    uint64_t    transferCoinAmount;

public:
    CReceipt(TxType &txTypeIn, uint256 &fromAddressIn, uint256 &toAddressIn,
            CoinType transferCoinTypeIn, uint64_t &transferCoinAmountIn) :
            txType(txTypeIn), fromAddress(fromAddressIn), toAddress(toAddressIn),
            transferCoinType(transferCoinTypeIn), transferCoinAmount(transferCoinAmountIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(txType);
        READWRITE(fromAddress);
        READWRITE(toAddress);
        READWRITE(transferCoinType);
        READWRITE(VARINT(transferCoinAmount));)
};

#endif //ACCOUNTS_ASSET_H