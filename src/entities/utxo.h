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
    virtual Object ToJson() const = 0;
    virtual std::string ToString() const = 0;
};

//////////////////////////////////////////////////
struct CSingleAddressCondIn : CUtxoCond {
    //uid to be covered in BaseTx
    CSingleAddressCondIn() : CUtxoCond(UtxoCondType::IP2SA) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
    )

    Object ToJson() const {
        Object o;
        o.push_back(Pair("cond_type","IP2SA"));
        return o;
    }

    std::string ToString() const override { return "cond_type=\"IP2SA\""; }
};
struct CSingleAddressCondOut : CUtxoCond {
    CUserID  uid;

    CSingleAddressCondOut() {};
    CSingleAddressCondOut(CUserID &uidIn) : CUtxoCond(UtxoCondType::OP2SA), uid(uidIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(uid);
    )
    Object ToJson() const {
        Object o;
        o.push_back(Pair("cond_type","OP2SA"));
        o.push_back(Pair("uid", uid.ToString()));
        return o;
    }

    std::string ToString() const override { return strprintf("cond_type=\"OP2SA\", uid=%s", uid.ToString()); }
};

//////////////////////////////////////////////////
struct CMultiSignAddressCondIn : CUtxoCond {
    uint8_t m = 0;
    uint8_t n = 0; // m <= n
    std::vector<CUserID> uids; //a list of uids (CRegID only) from users who enaged in a multisign process
    std::vector<UnsignedCharArray> signatures; //m signatures, each of which corresponds to redeemscript signature

    CMultiSignAddressCondIn() {};
    CMultiSignAddressCondIn(uint8_t mIn, uint8_t nIn, std::vector<CUserID> &uidsIn, std::vector<UnsignedCharArray> &signaturesIn):
        CUtxoCond(UtxoCondType::IP2MA), m(mIn),n(nIn), uids(uidsIn), signatures(signaturesIn) {};

    bool VerifySignature(const uint256 &sigHash, const std::vector<uint8_t> &signature, const CPubKey &pubKey) {
        // if (signatureCache.Get(sigHash, signature, pubKey))
        //     return true;

        if (!pubKey.Verify(sigHash, signature))
            return false;

        // signatureCache.Set(sigHash, signature, pubKey);
        return true;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(m));
        READWRITE(VARINT(n));
        READWRITE(uids);
        READWRITE(signatures);
    )

    Object ToJson() const override {
        Object o;
        o.push_back(Pair("cond_type","IP2MA"));
        o.push_back(Pair("m", m));
        o.push_back(Pair("n", n));
        Array usArr;
        for(auto u : uids)
            usArr.push_back(u.ToString());
        o.push_back(Pair("uids", usArr));
        Array signArr;
        for(auto s: signatures)
            signArr.push_back(HexStr(s));
        o.push_back(Pair("signatures", signArr));

        return o;
    }

    std::string ToString() const override {
        return strprintf("cond_type=\"IP2MA\", m=%d, n=%d, uids=%s, signatures=[omitted]",
                        m, n, db_util::ToString(uids));
    }
};
struct CMultiSignAddressCondOut : CUtxoCond {
    CKeyID dest_multisign_keyid;    //KeyID = Hash160(redeemScript)

    CMultiSignAddressCondOut() {};
    CMultiSignAddressCondOut(CKeyID destMultisignKeyId) : CUtxoCond(UtxoCondType::OP2MA), dest_multisign_keyid(destMultisignKeyId) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(dest_multisign_keyid);
    )

    Object ToJson() const override {
        Object o;
        o.push_back(Pair("cond_type", "OP2MA"));
        o.push_back(Pair("multisign_addr", dest_multisign_keyid.ToAddress()));
        return o;
    }
    std::string ToString() const override { return strprintf("cond_type=\"OP2MA\", dest_multisign_keyid=\"%s\"", dest_multisign_keyid.ToString()); }

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
    Object ToJson() const override {
        Object o;
        o.push_back(Pair("cond_type", "IP2PH"));
        o.push_back(Pair("password", password));
        return o;
    }

    std::string ToString() const override { return strprintf("cond_type=\"IP2PH\",password=\"%s\"", password); }
};
struct CPasswordHashLockCondOut: CUtxoCond {
    bool password_proof_required;
    uint256 password_hash; //hashed with salt

    CPasswordHashLockCondOut(): CUtxoCond(UtxoCondType::OP2PH), password_proof_required(false), password_hash(uint256()) {};
    CPasswordHashLockCondOut(bool passwordProofRequired, uint256& passwordHash) : CUtxoCond(UtxoCondType::OP2PH),
        password_proof_required(passwordProofRequired), password_hash(passwordHash) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(password_proof_required);
        READWRITE((uint8_t&) cond_type);
        READWRITE(password_hash);
    )

    Object ToJson() const override {
        Object o;
        o.push_back(Pair("cond_type", "OP2PH"));
        o.push_back(Pair("password_proof_required", password_proof_required));
        o.push_back(Pair("password_hash", password_hash.ToString()));
        return o;
    }


    std::string ToString() const override {
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

    Object ToJson() const override {
        Object o;
        o.push_back(Pair("cond_type", "OCLAIM_LOCK"));
        o.push_back(Pair("height", height));
        return o;
    }

    std::string ToString() const override { return strprintf("cond_type=\"OCLAIM_LOCK\", height=%llu", height); }
};

struct CReClaimLockCondOut : CUtxoCond {
    uint64_t height = 0;

    CReClaimLockCondOut() {};
    CReClaimLockCondOut(uint64_t &heightIn): CUtxoCond(UtxoCondType::ORECLAIM_LOCK), height(heightIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&) cond_type);
        READWRITE(VARINT(height));
    )


    Object ToJson() const override {
        Object o;
        o.push_back(Pair("cond_type", "ORECLAIM_LOCK"));
        o.push_back(Pair("height", height));
        return o;
    }
    std::string ToString() const override { return strprintf("cond_type=\"ORECLAIM_LOCK\", height=%llu", height); }
};

struct CUtxoCondStorageBean {
    std::shared_ptr<CUtxoCond> sp_utxo_cond = nullptr;

    CUtxoCondStorageBean() {};
    CUtxoCondStorageBean( std::shared_ptr<CUtxoCond> ptr): sp_utxo_cond(ptr) {}

    bool IsEmpty() const { return sp_utxo_cond == nullptr; }
    void SetEmpty() { sp_utxo_cond = nullptr; }

    Object ToJson() const {
        return sp_utxo_cond->ToJson();
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        if (IsEmpty())
            return 1;

        return (*sp_utxo_cond).GetSerializeSize(nType, nVersion) + 1;
    }

    template <typename Stream>
    void Serialize(Stream &os, int nType, int nVersion) const {

        uint8_t utxoCondType = UtxoCondType::NULL_UTXOCOND_TYPE;

        if(!IsEmpty())
            utxoCondType = sp_utxo_cond->cond_type;

        uint8_t pt = (uint8_t&)utxoCondType;

        ::Serialize(os, pt, nType, nVersion);

        if(IsEmpty())
            return;

        switch (sp_utxo_cond->cond_type) {
            case UtxoCondType::IP2SA:
                ::Serialize(os, *((CSingleAddressCondIn *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            case UtxoCondType::IP2MA:
                ::Serialize(os, *((CMultiSignAddressCondIn *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            case UtxoCondType::IP2PH:
                ::Serialize(os, *((CPasswordHashLockCondIn *) (sp_utxo_cond.get())), nType, nVersion);
                break;

            case UtxoCondType::OP2SA:
                ::Serialize(os, *((CSingleAddressCondOut *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            case UtxoCondType::OP2MA:
                ::Serialize(os, *((CMultiSignAddressCondOut *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            case UtxoCondType::OP2PH:
                ::Serialize(os, *((CPasswordHashLockCondOut *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            case UtxoCondType::OCLAIM_LOCK:
                ::Serialize(os, *((CClaimLockCondOut *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            case UtxoCondType::ORECLAIM_LOCK:
                ::Serialize(os, *((CReClaimLockCondOut *) (sp_utxo_cond.get())), nType, nVersion);
                break;
            default:
                throw ios_base::failure(strprintf("Serialize: utxoCondType(%d) error.", sp_utxo_cond->cond_type));
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
                sp_utxo_cond = std::make_shared<CSingleAddressCondIn>();
                ::Unserialize(is, *((CSingleAddressCondIn *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::IP2MA: {
                sp_utxo_cond = std::make_shared<CMultiSignAddressCondIn>();
                ::Unserialize(is, *((CMultiSignAddressCondIn *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::IP2PH: {
                sp_utxo_cond = std::make_shared<CPasswordHashLockCondIn>();
                ::Unserialize(is, *((CPasswordHashLockCondIn *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OP2SA: {
                sp_utxo_cond = std::make_shared<CSingleAddressCondOut>();
                ::Unserialize(is, *((CSingleAddressCondOut *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OP2MA: {
                sp_utxo_cond = std::make_shared<CMultiSignAddressCondOut>();
                ::Unserialize(is, *((CMultiSignAddressCondOut *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OP2PH: {
                sp_utxo_cond = std::make_shared<CPasswordHashLockCondOut>();
                ::Unserialize(is, *((CPasswordHashLockCondOut *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::OCLAIM_LOCK: {
                sp_utxo_cond = std::make_shared<CClaimLockCondOut>();
                ::Unserialize(is, *((CClaimLockCondOut *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            case UtxoCondType::ORECLAIM_LOCK: {
                sp_utxo_cond = std::make_shared<CReClaimLockCondOut>();
                ::Unserialize(is, *((CReClaimLockCondOut *)(sp_utxo_cond.get())), nType, nVersion);
                break;
            }

            default:
                throw ios_base::failure(strprintf("Unserialize: nTxType(%d) error.", utxoCondType));
        }

        sp_utxo_cond->cond_type = condType;
    }

    std::string ToString() const {
        return sp_utxo_cond->ToString();
    }

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

    Object ToJson() const {
        Object o;
        o.push_back(Pair("prev_utxo_txid", prev_utxo_txid.ToString()));
        o.push_back(Pair("prev_utxo_vout_index", prev_utxo_vout_index));
        Array arr;
        for( auto cond: conds)
            arr.push_back(cond.ToJson());
        o.push_back(Pair("conds", arr));
        return o;

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

    Object ToJson() const {
        Object o;

        o.push_back(Pair("coin_amount", coin_amount));
        Array arr;
        for( auto cond: conds)
            arr.push_back(cond.ToJson());
        o.push_back(Pair("conds", arr));
        return o;

    }
};

#endif  // ENTITIES_UTXO_H