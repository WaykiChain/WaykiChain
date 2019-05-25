// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONTRACT_H
#define CONTRACT_H

#include "tx.h"

class CContractDeployTx : public CBaseTx {
public:
    vector_unsigned_char contractScript;  // contract script content

public:
    CContractDeployTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_DEPLOY_TX) {
        assert(CONTRACT_DEPLOY_TX == pBaseTx->nTxType);
        *this = *(CContractDeployTx *)pBaseTx;
    }
    CContractDeployTx(): CBaseTx(CONTRACT_DEPLOY_TX) {}
    ~CContractDeployTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(contractScript);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << contractScript
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CContractDeployTx>(this); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual uint64_t GetValue() const { return 0; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

class CContractInvokeTx : public CBaseTx {
public:
    mutable CUserID appUid;  // app regid or address
    uint64_t bcoins;         // transfer amount
    vector_unsigned_char arguments; // arguments to invoke a contract method

public:
    CContractInvokeTx() : CBaseTx(CONTRACT_INVOKE_TX) {}

    CContractInvokeTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_INVOKE_TX) {
        assert(CONTRACT_INVOKE_TX == pBaseTx->nTxType);
        *this = *(CContractInvokeTx *)pBaseTx;
    }

    CContractInvokeTx(const CUserID &txUidIn, CUserID appUidIn, uint64_t feeIn,
                uint64_t bcoinsIn, int validHeightIn, vector_unsigned_char &argumentsIn):
                CBaseTx(CONTRACT_INVOKE_TX, txUidIn, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty()); //FIXME: shouldnot be using assert here, throw an error instead.

        if (appUidIn.type() == typeid(CRegID))
            assert(!appUidIn.get<CRegID>().IsEmpty());

        appUid = appUidIn;
        bcoins = bcoinsIn;
        arguments = argumentsIn;
    }

    CContractInvokeTx(const CUserID &txUidIn, CUserID appUidIn, uint64_t feeIn, uint64_t bcoinsIn, int validHeightIn):
                CBaseTx(CONTRACT_INVOKE_TX, txUidIn, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        if (appUidIn.type() == typeid(CRegID))
            assert(!appUidIn.get<CRegID>().IsEmpty());

        appUid = appUidIn;
        bcoins = bcoinsIn;
    }

    ~CContractInvokeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(appUid);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(arguments);
        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << appUid
               << VARINT(llFees) << VARINT(bcoins) << arguments;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoins; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CContractInvokeTx>(this); }
    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif