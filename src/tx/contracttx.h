// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CONTRACT_H
#define TX_CONTRACT_H

#include "tx.h"

class CContractDeployTx : public CBaseTx {
public:
    string contractScript;  // contract script content

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
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << contractScript
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CContractDeployTx>(this); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, 0}}; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

class CContractInvokeTx : public CBaseTx {
public:
    mutable CUserID appUid;  // app regid or address
    uint64_t bcoins;         // transfer amount
    UnsignedCharArray arguments; // arguments to invoke a contract method

public:
    CContractInvokeTx() : CBaseTx(CONTRACT_INVOKE_TX) {}

    CContractInvokeTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_INVOKE_TX) {
        assert(CONTRACT_INVOKE_TX == pBaseTx->nTxType);
        *this = *(CContractInvokeTx *)pBaseTx;
    }

    CContractInvokeTx(const CUserID &txUidIn, CUserID appUidIn, uint64_t feesIn,
                uint64_t bcoinsIn, int validHeightIn, UnsignedCharArray &argumentsIn):
                CBaseTx(CONTRACT_INVOKE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty()); //FIXME: shouldnot be using assert here, throw an error instead.

        if (appUidIn.type() == typeid(CRegID))
            assert(!appUidIn.get<CRegID>().IsEmpty());

        appUid = appUidIn;
        bcoins = bcoinsIn;
        arguments = argumentsIn;
    }

    CContractInvokeTx(const CUserID &txUidIn, CUserID appUidIn, uint64_t feesIn, uint64_t bcoinsIn, int validHeightIn):
                CBaseTx(CONTRACT_INVOKE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());
        else if (txUidIn.type() == typeid(CPubKey))
            assert(txUidIn.get<CPubKey>().IsFullyValid());

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
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << appUid
               << VARINT(llFees) << VARINT(bcoins) << arguments;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, bcoins}}; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CContractInvokeTx>(this); }
    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif //TX_CONTRACT_H