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

enum UtxoCondType : uint8_t {
    NULL_TYPE       = 0,

    P2SA            = 11,   //pay 2 single addr
    P2MA            = 12,   //pay 2 multisign addr
    P2PH            = 13,   //pay to password hash

    CLAIM_LOCK      = 21,   //claim or spend lock by block height
    RECLAIM_LOCK    = 22,   //reclaim lock by block height
};

struct CUtxoCond {
    UtxoCondType cond_type;

    CUtxoCond(UtxoCondType condType): cond_type(condType) {};

    virtual ~CUtxoCond(){};
};

struct CUtxoInput {
    TxID prev_utxo_txid;
    uint16_t prev_utxo_out_index;
    std::vector<CUtxoCond> conds; //needs to meet all conditions set in previous utxo vout

    CUtxoInput();   //empty instance

    CUtxoInput(TxID &prevUtxoTxId, uint16_t prevUtxoOutIndex, std::vector<CUtxoCond> &condsIn) :
        prev_utxo_txid(prevUtxoTxId), prev_utxo_out_index(prevUtxoOutIndex), conds(condsIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(prev_utxo_txid);
        READWRITE(VARINT(prev_utxo_out_index));
        //FIXME!!
        // for (auto cond : conds) {
        //     switch (cond.cond_type) {
        //     case UtxoCondType::P2SA : READWRITE((CSingleAddressCondIn) cond); break;
        //     case UtxoCondType::P2MA : READWRITE((CMultiSignAddressCondIn) cond); break;
        //     case UtxoCondType::P2PH : READWRITE(CPasswordHashLockCondIn) cond); break;
        //     default: break;
        //     }
        // }
    )
};
struct CUtxoOutput {
    uint64_t coin_amount;
    std::vector<CUtxoCond> conds;

    CUtxoOutput(uint64_t &coinAmount, std::vector<CUtxoCond> &condsIn) : 
        conds(condsIn), coin_amount(coinAmount) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(coin_amount));
        //FIXME!!
        // for (auto cond : conds) {
        //     switch (cond.cond_type) {
        //     case UtxoCondType::P2SA : READWRITE((CSingleAddressCondOut) cond); break;
        //     case UtxoCondType::P2MA : READWRITE((CMultiSignAddressCondOut) cond); break;
        //     case UtxoCondType::P2PH : READWRITE(CPasswordHashLockCondOut) cond); break;
        //     case UtxoCondType::CLAIM_LOCK : READWRITE(CClaimLockCondOut) cond); break;
        //     case UtxoCondType::RECLAIM_LOCK : READWRITE(CReclaimLockCondOut) cond); break;
        //     default: break;
        //     }
        // }
    )
};

//////////////////////////////////////////////////
struct CSingleAddressCondIn : CUtxoCond {
    //uid to be covered in BaseTx
    CSingleAddressCondIn() : CUtxoCond(UtxoCondType::P2SA) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
    )
    
};
struct CSingleAddressCondOut : CUtxoCond {
    CUserID  uid;

    CSingleAddressCondOut(CUserID &uidIn) : CUtxoCond(UtxoCondType::P2SA), uid(uidIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(uid);
    )

};

//////////////////////////////////////////////////
struct CMultiSignAddressCondIn : CUtxoCond {
    uint8_t m;
    uint8_t n; // m <= n
    std::vector<CUserID> uids;
    std::vector<byte[]> signatures; //m signatures, each of which corresponds to redeemscript signature

    CMultiSignAddressCondIn(uint8_t mIn, uint8_t nIn, std::vector<CUserID> &uidsIn, std::vector<byte[]> &signaturesIn): 
        CUtxoCond(UtxoCondType::P2MA), m(mIn),n(nIn),uids(uidsIn),signatures(signaturesIn) {};

    uint256 GetRedeemScriptHash() {
        string redeemScript = strprintf("%u8%s%u8", m, uids, n);
        return Hash(redeemScript); //redeemScriptHash = RIPEMD160(SHA256(redeemScript): TODO doublecheck hash algorithm
    }

    // bool VerifySignature(string multiSignAddr) { 
    //     //TODO
    //     if (signatures.size < m)
    //         return false;
        
    //     uint256 redeemScriptHash = GetRedeemScriptHash();

    //     for (const auto signature : signatures) {

    //     }

    //     return true;
    // }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(m));
        READWRITE(VARINT(n));
        READWRITE(uids);
        READWRITE(signatures);
    )

};
struct CMultiSignAddressCondOut : CUtxoCond {
    CUserID uid;

    CMultiSignAddressCondOut(CUserID &uidIn) : CUtxoCond(UtxoCondType::P2MA), uid(uidIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(uid);
    )

};

//////////////////////////////////////////////////
struct CPasswordHashLockCondIn : CUtxoCond {
    std::string password; //no greater than 256 chars

    CPasswordHashLockCondIn(string& passwordIn) : CUtxoCond(UtxoCondType::P2PH), password(passwordIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(password);
    )

};
struct CPasswordHashLockCondOut: CUtxoCond {
    uint256 password_hash; //hashed with salt

    CPasswordHashLockCondOut(uint256& passwordHash): CUtxoCond(UtxoCondType::P2PH), password_hash(passwordHash) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(password_hash);
    )

};
//////////////////////////////////////////////////
struct CClaimLockCondOut : CUtxoCond {
    uint64_t height;

    CClaimLockCondOut(uint64_t &heightIn): CUtxoCond(UtxoCondType::CLAIM_LOCK), height(heightIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(height));
    )

};

struct CReClaimLockCondOut : CUtxoCond {
    uint64_t height;

    CReClaimLockCondOut(uint64_t &heightIn): CUtxoCond(UtxoCondType::RECLAIM_LOCK), height(heightIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(height));
    )

};



// class HTLCCondition {
// public:
//     uint256   secret_hash = uint256();      //when not empty, hash = double-sha256(PubKey, secret, valid_height)
//                                             // usually the secret is transmitted to the target user thru a spearate channel, E.g. email
//     uint64_t collect_timeout = 0;           //afer timeout height it can be reclaimed by the Tx originator
//                                             //timeout_height = block_height + lock_duration + timeout
//                                             //when 0, it means always open to collect
//     bool is_null;                           // mem only: no persistence!

// public:
//     HTLCCondition(): is_null(true) {}
//     HTLCCondition(uint256 &secretHash, uint64_t collectTimeout): 
//         secret_hash(secretHash), collect_timeout(collectTimeout), is_null(false) {}

//     string ToString() const { 
//         return strprintf("secretHash=%s, collectTimeout=%d", secret_hash.ToString(), collect_timeout);
//     }

//     Object ToJson() const {
//         Object result;
//         result.push_back(Pair("secret_hash", secret_hash.ToString()));
//         result.push_back(Pair("collect_timeout", collect_timeout));
//         return result;
//     }

//     bool IsEmpty() const {
//         return secret_hash.IsEmpty() && collect_timeout == 0;
//     }

//     void SetEmpty() {
//         secret_hash.SetEmpty();
//         collect_timeout = 0;
//     }
// public: 
//     IMPLEMENT_SERIALIZE(
//         READWRITE(secret_hash);
//         READWRITE(VARINT(collect_timeout));
//     )
// };

// class UTXOEntity {
// public:
//     TokenSymbol coin_symbol = SYMB::WICC;
//     uint64_t coin_amount = 0;               //must be greater than 0
//     CUserID  to_uid;                        //when empty, anyone who produces the secret can spend it
//     uint64_t lock_duration = 0;             // only after (block_height + lock_duration) coin amount can be spent.
//     HTLCCondition htlc_cond;                // applied during spending time

//     bool is_null;                           // mem only: no persistence!

// public:
//     UTXOEntity(): is_null(true) {}
//     UTXOEntity(TokenSymbol &coinSymbol, uint64_t coinAmount, CUserID &toUid, 
//         uint64_t lockDuration, HTLCCondition &htlcIn): 
//         coin_symbol(coinSymbol), coin_amount(coinAmount), to_uid(toUid),
//         lock_duration(lockDuration), htlc_cond(htlcIn), is_null(false) {}

//     bool IsEmpty() const {
//         return coin_symbol.empty() && coin_amount == 0 && to_uid.IsEmpty() && lock_duration == 0 
//             && htlc_cond.IsEmpty(); 
//     }

//     void SetEmpty() {
//         coin_symbol = "";
//         coin_amount = 0;
//         to_uid.SetEmpty();
//         lock_duration = 0; 
//         htlc_cond.SetEmpty(); 
//     }

//     string ToString() { 
//         return strprintf("coinSymbol=%s, coinAmount=%d, toUid=%s, lockDuration=%d, htcl_cond=%s", 
//                         coin_symbol, coin_amount, to_uid.ToDebugString(), lock_duration, htlc_cond.ToString());
//     }

//     Object ToJson() const {
//         Object result;
//         result.push_back(Pair("coin_symbol", coin_symbol));
//         result.push_back(Pair("coin_amount", coin_amount));
//         result.push_back(Pair("to_uid", to_uid.ToString()));
//         result.push_back(Pair("lock_duration", lock_duration));

//         if (!htlc_cond.is_null) {
//             Array htlcArray;
//             htlcArray.push_back(htlc_cond.ToJson());
//             result.push_back(Pair("htlc", htlcArray));
//         }
//         return result;
//     }

// public: 
//     IMPLEMENT_SERIALIZE(
//         READWRITE(coin_symbol);
//         READWRITE(VARINT(coin_amount));
//         READWRITE(to_uid);
//         READWRITE(VARINT(lock_duration));
//         READWRITE(htlc_cond);
//     )
// };

#endif  // ENTITIES_UTXO_H