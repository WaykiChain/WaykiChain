#ifndef TX_WASM_CONTRACT_TX_H
#define TX_WASM_CONTRACT_TX_H

#include "tx.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_trace.hpp"
#include "wasm/wasm_constants.hpp"
#include "chrono"

using std::chrono::microseconds;
using std::chrono::system_clock;

class CWasmContractTx : public CBaseTx {
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
    CWasmContractTx(const CBaseTx *pBaseTx): CBaseTx(UNIVERSAL_CONTRACT_TX) {
        assert(UNIVERSAL_CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CWasmContractTx *)pBaseTx;
    }
    CWasmContractTx(): CBaseTx(UNIVERSAL_CONTRACT_TX) {}
    ~CWasmContractTx() {}

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

    virtual std::shared_ptr<CBaseTx>   GetNewInstance() const { return std::make_shared<CWasmContractTx>(*this); }
    virtual map<TokenSymbol, uint64_t> GetValues()      const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual uint64_t                   GetFuel(int32_t height, uint32_t fuelRate);
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

#endif //TX_WASM_CONTRACT_TX_H