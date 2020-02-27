// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_UTXO_H
#define ENTITIES_UTXO_H

#include <string>
#include <cstdint>
#include "id.h"
#include "persistence/dbaccess.h"

using namespace std;

enum UtxoCondType : uint8_t {
    NULL_UTXOCOND_TYPE = 0,
    //input cond types
    IP2SA              = 11,   //pay 2 single addr
    IP2MA              = 12,   //pay 2 multisign addr
    IP2PH              = 13,   //pay to password hash

    //output cond types
    OP2SA              = 111,
    OP2MA              = 112,
    OP2PH              = 113,
    OCLAIM_LOCK        = 114,
    ORECLAIM_LOCK      = 115,
};

class CUtxoCondStorageBean;

struct CUtxoCond {
    UtxoCondType cond_type = NULL_UTXOCOND_TYPE;

    CUtxoCond() {}
    CUtxoCond(UtxoCondType condType): cond_type(condType) {};

    virtual ~CUtxoCond(){};

    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }
};

struct CUtxoInput {
    TxID prev_utxo_txid;
    uint16_t prev_utxo_vout_index = 0;
    std::vector<CUtxoCondStorageBean> conds; //needs to meet all conditions set in previous utxo vout

    CUtxoInput() {}   //empty instance

    CUtxoInput(TxID &prevUtxoTxId, uint16_t prevUtxoOutIndex, std::vector<CUtxoCondStorageBean> &condsIn) :
        prev_utxo_txid(prevUtxoTxId), prev_utxo_vout_index(prevUtxoOutIndex), conds(condsIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(prev_utxo_txid);
        READWRITE(VARINT(prev_utxo_vout_index));
        READWRITE(conds);
    )

    std::string ToString() const {
        return strprintf("prev_utxo_txid=%s, prev_utxo_vout_index=%d, conds=%s", prev_utxo_txid.ToString(),
                        prev_utxo_vout_index, db_util::ToString(conds));
    }
};
struct CUtxoOutput {
    uint64_t coin_amount = 0;
    std::vector<CUtxoCondStorageBean> conds;

    CUtxoOutput() {};  //empty instance

    CUtxoOutput(uint64_t &coinAmount, std::vector<CUtxoCondStorageBean> &condsIn) :
        coin_amount(coinAmount), conds(condsIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(coin_amount));
        READWRITE(conds);
    )

    std::string ToString() const {
        return strprintf("coin_amount=%llu, conds=%s", coin_amount, db_util::ToString(conds));
    }
};

//////////////////////////////////////////////////
struct CSingleAddressCondIn : CUtxoCond {
    //uid to be covered in BaseTx
    CSingleAddressCondIn() : CUtxoCond(UtxoCondType::IP2SA) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
    )

    std::string ToString() { return "cond_type=\"IP2SA\""; }
};
struct CSingleAddressCondOut : CUtxoCond {
    CUserID  uid;

    CSingleAddressCondOut() {};
    CSingleAddressCondOut(CUserID &uidIn) : CUtxoCond(UtxoCondType::OP2SA), uid(uidIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(uid);
    )

    std::string ToString() { return strprintf("cond_type=\"OP2SA\", uid=%s", uid.ToString()); }
};

//////////////////////////////////////////////////
struct CMultiSignAddressCondIn : CUtxoCond {
    uint8_t m = 0;
    uint8_t n = 0; // m <= n
    std::vector<CUserID> uids;
    std::vector<UnsignedCharArray> signatures; //m signatures, each of which corresponds to redeemscript signature

    CMultiSignAddressCondIn() {};
    CMultiSignAddressCondIn(uint8_t mIn, uint8_t nIn, std::vector<CUserID> &uidsIn, std::vector<UnsignedCharArray> &signaturesIn):
        CUtxoCond(UtxoCondType::IP2MA), m(mIn),n(nIn),uids(uidsIn),signatures(signaturesIn) {};

    uint160 GetRedeemScriptHash() {
        string redeemScript = strprintf("%u%s%u", m, db_util::ToString(uids), n);
        return Hash160(redeemScript); //redeemScriptHash = RIPEMD160(SHA256(redeemScript): TODO doublecheck hash algorithm
    }

    bool VerifySignature(const uint256 &sigHash, const std::vector<uint8_t> &signature, const CPubKey &pubKey) {
        // if (signatureCache.Get(sigHash, signature, pubKey))
        //     return true;

        if (!pubKey.Verify(sigHash, signature))
            return false;

        // signatureCache.Set(sigHash, signature, pubKey);
        return true;
    }

    bool VerifyMultiSig(const TxID &prevUtxoTxId, uint16_t prevUtxoTxVoutIndex, const CUserID &txUid) {
        if (signatures.size() < m)
            return false;

        string redeemScript = strprintf("u%s%s%u", m, db_util::ToString(uids), n);

        CHashWriter ss(SER_GETHASH, CLIENT_VERSION);
        ss << prevUtxoTxId.ToString() << prevUtxoTxVoutIndex << txUid.ToString() << redeemScript;

        int verifyPassNum = 0;
        for (const auto signature : signatures) {
            for (const auto uid : uids) {
                if (VerifySignature(ss.GetHash(), signature, uid.get<CPubKey>())) {
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

    std::string ToString() {
        return strprintf("cond_type=\"IP2MA\", m=%d, n=%d, uids=%s, signatures=[omitted]",
                        m, n, db_util::ToString(uids));
    }
};
struct CMultiSignAddressCondOut : CUtxoCond {
    CUserID uid;

    CMultiSignAddressCondOut() {};
    CMultiSignAddressCondOut(CUserID &uidIn) : CUtxoCond(UtxoCondType::OP2MA), uid(uidIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(uid);
    )

    std::string ToString() { return strprintf("cond_type=\"OP2MA\",uid=\"%s\"", uid.ToString()); }

};

//////////////////////////////////////////////////
struct CPasswordHashLockCondIn : CUtxoCond {
    std::string password; //no greater than 256 chars

    CPasswordHashLockCondIn() {};
    CPasswordHashLockCondIn(string& passwordIn) : CUtxoCond(UtxoCondType::IP2PH), password(passwordIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(password);
    )

    std::string ToString() { return strprintf("cond_type=\"IP2PH\",password=\"%s\"", password); }
};
struct CPasswordHashLockCondOut: CUtxoCond {
    bool password_proof_required;
    uint256 password_hash; //hashed with salt

    CPasswordHashLockCondOut(): CUtxoCond(UtxoCondType::OP2PH), password_proof_required(false), password_hash(uint256()) {};
    CPasswordHashLockCondOut(bool passwordProofRequired, uint256& passwordHash) : 
        password_proof_required(passwordProofRequired), CUtxoCond(UtxoCondType::OP2PH), password_hash(passwordHash) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(password_proof_required);
        READWRITE((uint8_t&) cond_type);
        READWRITE(password_hash);
    )

    std::string ToString() { 
        return strprintf("cond_type=\"OP2PH\",password_proof_required=%d, password_hash=\"%s\"", 
                        password_proof_required, password_hash.ToString());
    }
};
//////////////////////////////////////////////////
struct CClaimLockCondOut : CUtxoCond {
    uint64_t height = 0;

    CClaimLockCondOut() {};
    CClaimLockCondOut(uint64_t &heightIn): CUtxoCond(UtxoCondType::OCLAIM_LOCK), height(heightIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(height));
    )

    std::string ToString() { return strprintf("cond_type=\"OCLAIM_LOCK\", height=%llu", height); }
};

struct CReClaimLockCondOut : CUtxoCond {
    uint64_t height = 0;

    CReClaimLockCondOut() {};
    CReClaimLockCondOut(uint64_t &heightIn): CUtxoCond(UtxoCondType::ORECLAIM_LOCK), height(heightIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(height));
    )

    std::string ToString() { return strprintf("cond_type=\"ORECLAIM_LOCK\", height=%llu", height); }
};

struct CUtxoCondStorageBean {
    std::shared_ptr<CUtxoCond> utxoCondPtr = nullptr;

    CUtxoCondStorageBean() {};
    CUtxoCondStorageBean( std::shared_ptr<CUtxoCond> ptr): utxoCondPtr(ptr) {}

    bool IsEmpty() const { return utxoCondPtr == nullptr; }
    void SetEmpty() { utxoCondPtr = nullptr; }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        if(IsEmpty())
            return 1 ;

        return (*utxoCondPtr).GetSerializeSize(nType, nVersion) + 1 ;
    }

    template <typename Stream>
    void Serialize(Stream &os, int nType, int nVersion) const {

        uint8_t utxoCondType = UtxoCondType::NULL_UTXOCOND_TYPE;

        if(!IsEmpty())
            utxoCondType = utxoCondPtr->cond_type;

        uint8_t pt = (uint8_t&)utxoCondType;

        ::Serialize(os, pt, nType, nVersion);

        if(IsEmpty())
            return ;

        switch (utxoCondPtr->cond_type) {
            case UtxoCondType::IP2SA:
                ::Serialize(os, *((CSingleAddressCondIn *) (utxoCondPtr.get())), nType, nVersion);
                break;
            case UtxoCondType::IP2MA:
                ::Serialize(os, *((CMultiSignAddressCondIn *) (utxoCondPtr.get())), nType, nVersion);
                break;
            case UtxoCondType::IP2PH:
                ::Serialize(os, *((CPasswordHashLockCondIn *) (utxoCondPtr.get())), nType, nVersion);
                break;

            case UtxoCondType::OP2SA:
                ::Serialize(os, *((CSingleAddressCondOut *) (utxoCondPtr.get())), nType, nVersion);
                break;
            case UtxoCondType::OP2MA:
                ::Serialize(os, *((CMultiSignAddressCondOut *) (utxoCondPtr.get())), nType, nVersion);
                break;
            case UtxoCondType::OP2PH:
                ::Serialize(os, *((CPasswordHashLockCondOut *) (utxoCondPtr.get())), nType, nVersion);
                break;
            case UtxoCondType::OCLAIM_LOCK:
                ::Serialize(os, *((CClaimLockCondOut *) (utxoCondPtr.get())), nType, nVersion);
                break;
            case UtxoCondType::ORECLAIM_LOCK:
                ::Serialize(os, *((CReClaimLockCondOut *) (utxoCondPtr.get())), nType, nVersion);
                break;
            default:
                throw ios_base::failure(strprintf("Serialize: utxoCondType(%d) error.", utxoCondPtr->cond_type));
        }

    }

    template <typename Stream>
    void Unserialize(Stream &is, int nType, int nVersion) {

        uint8_t utxoCondType = UtxoCondType::NULL_UTXOCOND_TYPE;
        is.read((char *)&(utxoCondType), sizeof(utxoCondType));
        UtxoCondType condType = (UtxoCondType) utxoCondType;
        if(condType == UtxoCondType::NULL_UTXOCOND_TYPE)
            return;

        switch(condType) {
            case UtxoCondType::IP2SA: {
                utxoCondPtr = std::make_shared<CSingleAddressCondIn>();
                ::Unserialize(is, *((CSingleAddressCondIn *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::IP2MA: {
                utxoCondPtr = std::make_shared<CMultiSignAddressCondIn>();
                ::Unserialize(is, *((CMultiSignAddressCondIn *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::IP2PH: {
                utxoCondPtr = std::make_shared<CPasswordHashLockCondIn>();
                ::Unserialize(is, *((CPasswordHashLockCondIn *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OP2SA: {
                utxoCondPtr = std::make_shared<CSingleAddressCondOut>();
                ::Unserialize(is, *((CSingleAddressCondIn *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OP2MA: {
                utxoCondPtr = std::make_shared<CMultiSignAddressCondOut>();
                ::Unserialize(is, *((CMultiSignAddressCondIn *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OP2PH: {
                utxoCondPtr = std::make_shared<CPasswordHashLockCondOut>();
                ::Unserialize(is, *((CPasswordHashLockCondIn *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OCLAIM_LOCK: {
                utxoCondPtr = std::make_shared<CClaimLockCondOut>();
                ::Unserialize(is, *((CClaimLockCondOut *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::ORECLAIM_LOCK: {
                utxoCondPtr = std::make_shared<CReClaimLockCondOut>();
                ::Unserialize(is, *((CReClaimLockCondOut *)(utxoCondPtr.get())), nType, nVersion);
                break;
            }

            default:
                throw ios_base::failure(strprintf("Unserialize: nTxType(%d) error.", utxoCondType));
        }

        utxoCondPtr->cond_type = condType;
    }

    std::string ToString() const {
        switch (utxoCondPtr->cond_type) {
            case UtxoCondType::IP2SA:
                return ((CSingleAddressCondIn *) utxoCondPtr.get())->ToString();
            case UtxoCondType::IP2MA:
                return ((CMultiSignAddressCondIn *) utxoCondPtr.get())->ToString();
            case UtxoCondType::IP2PH:
                return ((CPasswordHashLockCondIn *) utxoCondPtr.get())->ToString();
            case UtxoCondType::OP2SA:
                return ((CSingleAddressCondOut *) utxoCondPtr.get())->ToString();
            case UtxoCondType::OP2MA:
                return ((CMultiSignAddressCondOut *) utxoCondPtr.get())->ToString();
            case UtxoCondType::OP2PH:
                return ((CPasswordHashLockCondOut *) utxoCondPtr.get())->ToString();
            case UtxoCondType::OCLAIM_LOCK:
                return ((CClaimLockCondOut *) utxoCondPtr.get())->ToString();
            case UtxoCondType::ORECLAIM_LOCK:
                return ((CReClaimLockCondOut *) utxoCondPtr.get())->ToString();
            default:
                throw ios_base::failure(strprintf("ToString: utxoCondType(%d) error.", utxoCondPtr->cond_type));
        }
    }

};
#endif  // ENTITIES_UTXO_H