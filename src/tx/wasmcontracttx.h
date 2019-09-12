#ifndef TX_WASM_CONTRACT_TX_H
#define TX_WASM_CONTRACT_TX_H

#include "tx.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_trace.hpp"

class CWasmContractTx : public CBaseTx {
public:
    // uint64_t contract;
    // uint64_t action;
    // std::vector<uint64_t> permissions;
    // std::vector<char> data;
    vector<wasm::inline_transaction> inlinetransactions;

    // uint64_t amount;
     TokenSymbol symbol;

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
        // READWRITE(contract);
        // READWRITE(action);
        // READWRITE(data);
        READWRITE(inlinetransactions);
        // READWRITE(amount);
        // READWRITE(symbol);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
               //<< contract << action << data << amount << symbol
                << inlinetransactions
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CWasmContractTx>(this); }
    virtual uint64_t GetFuel(uint32_t nFuelRate);
    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);

public:
    void DispatchInlineTransaction( wasm::inline_transaction_trace& trace,
                                    wasm::inline_transaction& trx,
                                     uint64_t receiver,
                                     CCacheWrapper &cache,
                                     CValidationState &state,
                                     uint32_t recurse_depth);

};

#endif //TX_WASM_CONTRACT_TX_H