// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CONTRACT_H
#define TX_CONTRACT_H

#include "tx.h"
#include "entities/contract.h"


/**#################### LuaVM Contract Deploy & Invoke Class Definitions ##############################**/
class CLuaContractDeployTx : public CBaseTx {
public:
    CLuaContract contract;  // contract script content

public:

    CLuaContractDeployTx(): CBaseTx(LCONTRACT_DEPLOY_TX) {}
    ~CLuaContractDeployTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(contract);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << contract
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CLuaContractDeployTx>(*this); }
    virtual uint64_t GetFuel(uint32_t nFuelRate);
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

class CLuaContractInvokeTx : public CBaseTx {
public:
    mutable CUserID app_uid;  // app regid or address
    uint64_t coin_amount;     // coin amount (coin symbol: WICC)
    string arguments;         // arguments to invoke a contract method

public:
    CLuaContractInvokeTx() : CBaseTx(LCONTRACT_INVOKE_TX) {}
    ~CLuaContractInvokeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(app_uid);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(coin_amount));
        READWRITE(arguments);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << app_uid
               << VARINT(llFees) << VARINT(coin_amount) << arguments;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CLuaContractInvokeTx>(*this); }
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

/**#################### Universal Contract Deploy & Invoke Class Definitions ##############################**/
class CUniversalContractDeployTx : public CBaseTx {
public:
    TokenSymbol         coin_symbol;
    uint64_t            coin_amount;
    CUniversalContract  contract;  // contract script content

public:
    CUniversalContractDeployTx(): CBaseTx(UCONTRACT_DEPLOY_TX) {}
    ~CUniversalContractDeployTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE(contract);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << VARINT(llFees) << fee_symbol
               << coin_symbol << VARINT(coin_amount) << contract;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUniversalContractDeployTx>(*this); }
    virtual uint64_t GetFuel(uint32_t nFuelRate);
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

class CUniversalContractInvokeTx : public CBaseTx {
public:
    mutable CUserID app_uid;  // app regid or address
    string arguments;         // arguments to invoke a contract method
    TokenSymbol coin_symbol;
    uint64_t coin_amount;  // transfer amount to contract account

public:
    CUniversalContractInvokeTx() : CBaseTx(UCONTRACT_INVOKE_TX) {}
    ~CUniversalContractInvokeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(app_uid);
        READWRITE(arguments);
        READWRITE(VARINT(llFees));
        READWRITE(fee_symbol);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << app_uid << arguments
               << VARINT(llFees) << fee_symbol << coin_symbol << VARINT(coin_amount);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUniversalContractInvokeTx>(*this); }
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};
#endif //TX_CONTRACT_H