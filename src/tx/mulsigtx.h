// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MULSIGTX_H
#define COIN_MULSIGTX_H

#include "tx.h"

class CSignaturePair {
public:
    CRegID regid;                 //!< regid only
    UnsignedCharArray signature;  //!< signature

    IMPLEMENT_SERIALIZE(
        READWRITE(regid);
        READWRITE(signature);)

public:
    CSignaturePair(const CSignaturePair &signaturePair) {
        regid     = signaturePair.regid;
        signature = signaturePair.signature;
    }

    CSignaturePair(const CRegID &regIdIn, const UnsignedCharArray &signatureIn) {
        regid     = regIdIn;
        signature = signatureIn;
    }

    CSignaturePair() {}

    string ToString() const;
    Object ToJson() const;
};

class CMulsigTx : public CBaseTx {
public:
    mutable CUserID to_uid;              //!< keyid or regid
    uint64_t bcoins;                        //!< transfer amount
    uint8_t required;                       //!< number of required keys
    string memo;                            //!< memo
    vector<CSignaturePair> signaturePairs;  //!< signature pair

    CKeyID keyId;  //!< only in memory

public:
    CMulsigTx() : CBaseTx(BCOIN_TRANSFER_MTX) {}

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &toUidIn, uint64_t feesIn,
              const uint64_t valueIn, const int32_t validHeightIn, const uint8_t requiredIn, const string &memoIn)
        : CBaseTx(BCOIN_TRANSFER_MTX, CNullID(), validHeightIn, feesIn) {
        signaturePairs = signaturePairsIn;
        to_uid         = toUidIn;
        bcoins         = valueIn;
        required       = requiredIn;
        memo           = memoIn;
    }

    ~CMulsigTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(signaturePairs);
        READWRITE(to_uid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(required));
        READWRITE(memo);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height);
            // Do NOT add item.signature.
            for (const auto &item : signaturePairs) {
                ss << item.regid;
            }
            ss << to_uid << fee_symbol << VARINT(llFees) << VARINT(bcoins) << VARINT(required) << memo;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CMulsigTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

    // If the sender has no regid before, geneate a regid for the sender.
    bool GenerateRegID(CTxExecuteContext &context, CAccount &account);
};

#endif //COIN_MULSIGTX_H