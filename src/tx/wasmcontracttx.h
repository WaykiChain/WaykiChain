#ifndef TX_WASM_CONTRACT_TX_H
#define TX_WASM_CONTRACT_TX_H

#include "tx.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_trace.hpp"
#include "chrono"

using std::chrono::microseconds;
using std::chrono::system_clock;

class CWasmContractTx : public CBaseTx {
public:
    vector<wasm::inline_transaction> inline_transactions;
    vector<wasm::signature_pair>     signatures;
    //uint64_t payer;

public:
    bool mining = false;
    bool validating_tx_in_mem_pool = false;
    system_clock::time_point pseudo_start;
    std::chrono::microseconds billed_time = chrono::microseconds(0);

    void pause_billing_timer();
    void resume_billing_timer();

public:
    CWasmContractTx(const CBaseTx *pBaseTx): CBaseTx(WASM_CONTRACT_TX) {
        assert(WASM_CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CWasmContractTx *)pBaseTx;
    }
    CWasmContractTx(): CBaseTx(WASM_CONTRACT_TX) {}
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

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                << inline_transactions
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CWasmContractTx>(*this); }
    virtual uint64_t GetFuel(int32_t height, uint32_t fuelRate);
    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
    virtual bool IsMultiSignSupport() const {return true;}

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