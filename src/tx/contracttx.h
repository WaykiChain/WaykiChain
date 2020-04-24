// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CONTRACT_H
#define TX_CONTRACT_H


#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_trace.hpp"
#include "wasm/wasm_constants.hpp"
#include "chrono"
#include "tx.h"
#include "entities/contract.h"

using std::chrono::microseconds;
using std::chrono::system_clock;

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

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << contract
           << VARINT(llFees);
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CLuaContractDeployTx>(*this); }
    virtual uint64_t GetFuelFee(CCacheWrapper &cw, int32_t height, uint32_t fuelRate);
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CLuaContractInvokeTx : public CBaseTx {
public:
    mutable CUserID app_uid;  // app regid
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

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << app_uid
            << VARINT(llFees) << VARINT(coin_amount) << arguments;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CLuaContractInvokeTx>(*this); }
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

/**#################### **Deprecated** R2 Contract Deploy & Invoke Class Definitions ##############################**/
class CUniversalContractDeployR2Tx : public CBaseTx {
public:
    CUniversalContract contract;  // contract script content

public:
    CUniversalContractDeployR2Tx(): CBaseTx(UCONTRACT_DEPLOY_R2_TX) {}
    ~CUniversalContractDeployR2Tx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));
        READWRITE(contract);

        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
           << VARINT(llFees) << fee_symbol << contract;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUniversalContractDeployR2Tx>(*this); }
    virtual uint64_t GetFuelFee(CCacheWrapper &cw, int32_t height, uint32_t fuelRate);
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};
class CUniversalContractInvokeR2Tx : public CBaseTx {
public:
    mutable CUserID app_uid;  // app regid
    string arguments;         // arguments to invoke a contract method
    TokenSymbol coin_symbol;
    uint64_t coin_amount;  // transfer amount to contract account

public:
    CUniversalContractInvokeR2Tx() : CBaseTx(UCONTRACT_INVOKE_R2_TX) {}
    ~CUniversalContractInvokeR2Tx() {}

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

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << app_uid
           << arguments << VARINT(llFees) << fee_symbol << coin_symbol << VARINT(coin_amount);
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUniversalContractInvokeR2Tx>(*this); }
    virtual string ToString(CAccountDBCache &accountView);
    virtual Object ToJson(const CAccountDBCache &accountView) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

/**#################### Unified System/User Contract Class Definition ##############################**/
class CUniversalTx : public CBaseTx {
public:
    vector<wasm::inline_transaction> inline_transactions;
    vector<wasm::signature_pair>     signatures;

public:
    uint64_t                      run_cost;
    uint64_t                      pending_block_time;
    uint64_t                      recipients_size;
    system_clock::time_point      pseudo_start;
    std::chrono::microseconds     billed_time              = chrono::microseconds(0);
    std::chrono::milliseconds     max_transaction_duration = std::chrono::milliseconds(wasm::max_wasm_execute_time_infinite);
    TxExecuteContextType          context_type             = TxExecuteContextType::CONNECT_BLOCK;

    void                      pause_billing_timer();
    void                      resume_billing_timer();
    std::chrono::milliseconds get_max_transaction_duration() { return max_transaction_duration; }
    void                      set_signature(const uint64_t& account, const vector<uint8_t>& signature);
    void                      set_signature(const wasm::signature_pair& signature);

public:
    CUniversalTx(const CBaseTx *pBaseTx): CBaseTx(UNIVERSAL_CONTRACT_TX) {
        assert(UNIVERSAL_CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CUniversalTx *)pBaseTx;
    }
    CUniversalTx(): CBaseTx(UNIVERSAL_CONTRACT_TX) {}
    ~CUniversalTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(inline_transactions);
        READWRITE(signatures);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
           << inline_transactions << VARINT(llFees);

        WriteCompactSize(hw, signatures.size());
        for (const auto &s : signatures) {
            hw << VARINT(s.account);
        }
    }

    virtual std::shared_ptr<CBaseTx>   GetNewInstance() const { return std::make_shared<CUniversalTx>(*this); }
    virtual map<TokenSymbol, uint64_t> GetValues()      const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual uint64_t                   GetFuelFee(CCacheWrapper &cw, int32_t height, uint32_t fuelRate);
    virtual bool                       GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;


    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);


public:
    void validate_contracts(CTxExecuteContext &context);
    void validate_authorization(const std::vector<uint64_t> &authorization_accounts);
    void get_accounts_from_signatures(CCacheWrapper &database,
                                          std::vector<uint64_t> &authorization_accounts);
    void execute_inline_transaction( wasm::inline_transaction_trace &trace,
                                      wasm::inline_transaction &trx,
                                      uint64_t receiver,
                                      CCacheWrapper &database,
                                      vector<CReceipt> &receipts,
                                      //CValidationState &state,
                                      uint32_t recurse_depth);

};


#endif  // TX_CONTRACT_H