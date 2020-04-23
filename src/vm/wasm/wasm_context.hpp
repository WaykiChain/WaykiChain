#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#include "tx/contracttx.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasm_trace.hpp"
#include "eosio/vm/allocator.hpp"
#include "persistence/cachewrapper.h"
#include "entities/receipt.h"
#include "wasm/exception/exceptions.hpp"

using namespace std;
using namespace wasm;
namespace wasm {

    // bool get_native_contract_abi(uint64_t contract, std::vector<char>& abi);
    // bool is_native_contract(uint64_t contract);

    class wasm_context : public wasm_context_interface {

    public:
        wasm_context(CUniversalContractTx &ctrl, inline_transaction &t, CCacheWrapper &cw,
                     vector <CReceipt> &receipts_in, bool mining, uint32_t depth = 0)
                : trx(t), control_trx(ctrl), database(cw), receipts(receipts_in), recurse_depth(depth) {
            reset_console();
        };

        ~wasm_context() {
            wasm_alloc.free();
        };

    public:
        void                  initialize();
        void                  execute(inline_transaction_trace &trace);
        void                  execute_one(inline_transaction_trace &trace);
        bool                  has_permission_from_inline_transaction(const permission &p);
        std::vector <uint8_t> get_code(const uint64_t& account);
// Console methods:
    public:
        void                      reset_console();
        std::ostringstream&       get_console_stream()       { return _pending_console_output; }
        const std::ostringstream& get_console_stream() const { return _pending_console_output; }

//virtual
    public:

        void        execute_inline   (const inline_transaction& t);
        void        notify_recipient(const uint64_t& recipient  );
        bool        has_recipient    (const uint64_t& account    ) const;

        uint64_t    receiver() { return _receiver;    }
        uint64_t    contract() { return trx.contract; }
        uint64_t    action()   { return trx.action;   }

        const char* get_action_data()      { return trx.data.data(); }
        uint32_t    get_action_data_size() { return trx.data.size(); }

        bool        is_account   (const uint64_t& account) const;
        void        require_auth (const uint64_t& account) const;
        void        require_auth2(const uint64_t& account, const uint64_t& permission) const {}
        bool        has_authorization(const uint64_t& account) const;
        uint64_t    pending_block_time() { return control_trx.pending_block_time; }
        string      get_txid()  { return control_trx.GetHash().ToString();}
        void        exit    ()  { wasmif.exit(); }
        bool        get_system_asset_price(uint64_t base, uint64_t quote, std::vector<char>& price);

        bool set_data( const uint64_t& contract, const string& k, const string& v ) {
            CAccount   contract_account;
            wasm::name contract_name = wasm::name(contract);
            CHAIN_ASSERT( database.accountCache.GetAccount(CRegID(contract), contract_account),
                          account_access_exception,
                          "contract '%s' does not exist",
                          contract_name.to_string().c_str())

            return database.contractCache.SetContractData(contract_account.regid, k, v);
        }

        bool get_data( const uint64_t& contract, const string& k, string &v ) {
            CAccount   contract_account;
            wasm::name contract_name = wasm::name(contract);
            CHAIN_ASSERT( database.accountCache.GetAccount(CRegID(contract), contract_account),
                          account_access_exception,
                          "contract '%s' does not exist",
                          contract_name.to_string().c_str())

            return database.contractCache.GetContractData(contract_account.regid, k, v);
        }

        bool erase_data( const uint64_t& contract, const string& k ) {
            CAccount   contract_account;
            wasm::name contract_name = wasm::name(contract);
            CHAIN_ASSERT( database.accountCache.GetAccount(CRegID(contract), contract_account),
                          account_access_exception,
                          "contract '%s' does not exist",
                          contract_name.to_string().c_str())

            return database.contractCache.EraseContractData(contract_account.regid, k);
        }

        std::vector<uint64_t> get_active_producers();

        bool contracts_console() {
            return SysCfg().GetBoolArg("-contracts_console", false) &&
                   control_trx.context_type == TxExecuteContextType::VALIDATE_MEMPOOL;
        }

        void console_append(const string& val) {
            _pending_console_output << val;
        }

        vm::wasm_allocator* get_wasm_allocator() { return &wasm_alloc; }
        bool                is_memory_in_wasm_allocator ( const uint64_t& p ) {
            return wasm_alloc.is_in_range(reinterpret_cast<const char*>(p));
        }
        std::chrono::milliseconds get_max_transaction_duration() { return control_trx.get_max_transaction_duration(); }
        void                      update_storage_usage( const uint64_t& account, const int64_t& size_in_bytes);
        void                      pause_billing_timer ()  { control_trx.pause_billing_timer();  };
        void                      resume_billing_timer()  { control_trx.resume_billing_timer(); };

    public:
        inline_transaction&        trx;
        CUniversalContractTx&           control_trx;
        CCacheWrapper&             database;
        vector<CReceipt>&          receipts;
        uint32_t                   recurse_depth;
        vector<uint64_t>           notified;
        vector<inline_transaction> inline_transactions;

        wasm::wasm_interface       wasmif;
        vm::wasm_allocator         wasm_alloc;
        uint64_t                   _receiver;

    private:
        std::ostringstream         _pending_console_output;
    };
}
