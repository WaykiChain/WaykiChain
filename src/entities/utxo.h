// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_UTXO_H
#define ENTITIES_UTXO_H

#include <string>
#include <cstdint>
#include "id.h"

using namespace std;

class HTLCCondition {
public:
    uint256   secret_hash = uint256();      //when not empty, hash = double-sha256(PubKey, secret, valid_height)
                                            // usually the secret is transmitted to the target user thru a spearate channel, E.g. email
    uint64_t collect_timeout = 0;           //afer timeout height it can be reclaimed by the Tx originator
                                            //timeout_height = block_height + lock_duration + timeout
                                            //when 0, it means always open to collect
    bool is_null;                           // mem only: no persistence!

public:
    HTLCCondition(): is_null(true) {}
    HTLCCondition(uint256 &secretHash, uint64_t collectTimeout): 
        secret_hash(secretHash), collect_timeout(collectTimeout), is_null(false) {}

    string ToString() const { 
        return strprintf("secretHash=%s, collectTimeout=%d", secret_hash.ToString(), collect_timeout);
    }

    Object ToJson() const {
        Object result;
        result.push_back(Pair("secret_hash", secret_hash.ToString()));
        result.push_back(Pair("collect_timeout", collect_timeout));
        return result;
    }

    bool IsEmpty() const {
        return secret_hash.IsEmpty() && collect_timeout == 0;
    }

    void SetEmpty() {
        secret_hash.SetEmpty();
        collect_timeout = 0;
    }
public: 
    IMPLEMENT_SERIALIZE(
        READWRITE(secret_hash);
        READWRITE(VARINT(collect_timeout));
    )
};

class UTXOEntity {
public:
    TokenSymbol coin_symbol = SYMB::WICC;
    uint64_t coin_amount = 0;               //must be greater than 0
    CUserID  to_uid;                        //when empty, anyone who produces the secret can spend it
    uint64_t lock_duration = 0;             // only after (block_height + lock_duration) coin amount can be spent.
    HTLCCondition htlc_cond;                // applied during spending time

    bool is_null;                           // mem only: no persistence!

public:
    UTXOEntity(): is_null(true) {}
    UTXOEntity(TokenSymbol &coinSymbol, uint64_t coinAmount, CUserID &toUid, 
        uint64_t lockDuration, HTLCCondition &htlcIn): 
        coin_symbol(coinSymbol), coin_amount(coinAmount), to_uid(toUid),
        lock_duration(lockDuration), htlc_cond(htlcIn), is_null(false) {}

    bool IsEmpty() const {
        return coin_symbol.empty() && coin_amount == 0 && to_uid.IsEmpty() && lock_duration == 0 
            && htlc_cond.IsEmpty(); 
    }

    void SetEmpty() {
        coin_symbol = "";
        coin_amount = 0;
        to_uid.SetEmpty();
        lock_duration = 0; 
        htlc_cond.SetEmpty(); 
    }

    string ToString() { 
        return strprintf("coinSymbol=%s, coinAmount=%d, toUid=%s, lockDuration=%d, htcl_cond=%s", 
                        coin_symbol, coin_amount, to_uid.ToDebugString(), lock_duration, htlc_cond.ToString());
    }

    Object ToJson() const {
        Object result;
        result.push_back(Pair("coin_symbol", coin_symbol));
        result.push_back(Pair("coin_amount", coin_amount));
        result.push_back(Pair("to_uid", to_uid.ToString()));
        result.push_back(Pair("lock_duration", lock_duration));

        if (!htlc_cond.is_null) {
            Array htlcArray;
            htlcArray.push_back(htlc_cond.ToJson());
            result.push_back(Pair("htlc", htlcArray));
        }
        return result;
    }

public: 
    IMPLEMENT_SERIALIZE(
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(to_uid);
        READWRITE(VARINT(lock_duration));
        READWRITE(htlc_cond);
    )
};

#endif  // ENTITIES_UTXO_H