// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2016 The Coin developers
// Copyright (c) 2014-2019 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php


#ifndef COIN_MULSIGTX_H
#define COIN_MULSIGTX_H

#include "tx.h"

class CSignaturePair {
public:
    CRegID regId;  //!< regid only
    vector_unsigned_char signature;

    IMPLEMENT_SERIALIZE(
        READWRITE(regId);
        READWRITE(signature);)

public:
    CSignaturePair(const CSignaturePair &signaturePair) {
        regId     = signaturePair.regId;
        signature = signaturePair.signature;
    }

    CSignaturePair(const CRegID &regIdIn, const vector_unsigned_char &signatureIn) {
        regId     = regIdIn;
        signature = signatureIn;
    }

    CSignaturePair() {}

    string ToString() const;
    Object ToJson() const;
};

class CMulsigTx : public CBaseTx {
public:
    mutable CUserID desUserId;              //!< keyid or regid
    uint64_t bcoins;                        //!< transfer amount
    uint8_t required;                       //!< required keys
    vector_unsigned_char memo;              //!< memo
    vector<CSignaturePair> signaturePairs;  //!< signature pair

    CKeyID keyId;  //!< only in memory

public:
    CMulsigTx() : CBaseTx(COMMON_MTX) {}

    CMulsigTx(const CBaseTx *pBaseTx) : CBaseTx(COMMON_MTX) {
        assert(COMMON_MTX == pBaseTx->nTxType);
        *this = *(CMulsigTx *)pBaseTx;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feesIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn, const vector_unsigned_char &memoIn)
        : CBaseTx(COMMON_MTX, CNullID(), validHeightIn, feesIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoins         = valueIn;
        required       = requiredIn;
        memo           = memoIn;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feesIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn)
        : CBaseTx(COMMON_MTX, CNullID(), validHeightIn, feesIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoins         = valueIn;
        required       = requiredIn;
    }

    ~CMulsigTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(signaturePairs);
        READWRITE(desUserId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(required));
        READWRITE(memo);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight);
            // Do NOT add item.signature.
            for (const auto &item : signaturePairs) {
                ss << item.regId;
            }
            ss << desUserId << VARINT(llFees) << VARINT(bcoins) << VARINT(required) << memo;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoins; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CMulsigTx>(this); }
    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif //COIN_MULSIGTX_H