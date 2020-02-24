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
    std::vector<UnsignedCharArray> signatures; //m signatures, each of which corresponds to redeemscript signature

    CMultiSignAddressCondIn(uint8_t mIn, uint8_t nIn, std::vector<CUserID> &uidsIn, std::vector<UnsignedCharArray> &signaturesIn): 
        CUtxoCond(UtxoCondType::P2MA), m(mIn),n(nIn),uids(uidsIn),signatures(signaturesIn) {};

    uint160 GetRedeemScriptHash() {
        string redeemScript = strprintf("%u8%s%u8", m, uids, n);
        return Hash160(redeemScript); //redeemScriptHash = RIPEMD160(SHA256(redeemScript): TODO doublecheck hash algorithm
    }

    bool VerifyMultiSig(const TxID &prevUtxoTxId, uint16_t prevUtxoTxVoutIndex, const CUserID &txUid) { 
        if (signatures.size < m)
            return false;
        
        string redeemScript = strprintf("u8%s%u8%s", m, uids, n);
        string content = strprintf("%s%u16%s%s", prevUtxoTxId.ToString(), prevUtxoTxVoutIndex, txUid.ToString(), redeemScript);
        uint256 hash = Hash256(content);

        int verifyPassNum = 0;
        for (const auto signature : signatures) {
            for (const auto uid : uids) {
                if (VerifySignature(hash, signature, uid.get<CPubKey>())) {
                    verifyPassNum++;
                    break;
                }
            }
        }
        bool verified = (verifyPassNum >= m);

        return verified;
    }

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

#endif  // ENTITIES_UTXO_H