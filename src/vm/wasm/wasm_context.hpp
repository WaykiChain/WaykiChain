#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#include "tx/universaltx.h"
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

    class wasm_context : public wasm_context_interface {

    public:
        wasm_context(CUniversalTx &ctrl, inline_transaction &t, CCacheWrapper &cw,
                     vector <CReceipt> &receipts_in, vector <transaction_log>& logs, bool mining, uint32_t depth = 0)
                : trx_cord(ctrl.txCord), trx(t), control_trx(ctrl), database(cw), receipts(receipts_in), trx_logs(logs), recurse_depth(depth) {
            reset_console();
        };

        ~wasm_context() {
            wasm_alloc.free();
        };

    public:
        void initialize();
        void execute(inline_transaction_trace &trace);
        void execute_one(inline_transaction_trace &trace);
        bool has_permission_from_inline_transaction(const permission &p);
        bool get_code(const uint64_t& contract, std::vector <uint8_t> &code, uint256 &hash);

        uint64_t get_runcost();
        void check_authorization(const inline_transaction& t);

        //fixme:V4
        void call_one(inline_transaction_trace &trace);
        int64_t call_one_with_return(inline_transaction_trace &trace);

// Console methods:
    public:
        void                      reset_console();
        std::ostringstream&       get_console_stream()       { return _pending_console_output; }
        const std::ostringstream& get_console_stream() const { return _pending_console_output; }

//virtual
    public:

        void        execute_inline   (const inline_transaction& t);
        void        notify_recipient (const uint64_t& recipient  );
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
        TxID        get_txid()  { return control_trx.GetHash(); }
        uint64_t    get_maintainer(const uint64_t& contract);
        void        exit    ()  { wasmif.exit(); }
        bool        get_system_asset_price(uint64_t base, uint64_t quote, std::vector<char>& price);

        bool set_data( const uint64_t& contract, const string& k, const string& v ) {
            CUniversalContractStore contractStore;
            CHAIN_ASSERT( database.contractCache.GetContract(CRegID(contract), contractStore),
                          contract_exception,
                          "contract '%s' does not exist",
                          wasm::regid(contract).to_string())

            return database.contractCache.SetContractData(CRegID(contract), k, v);
        }

        bool get_data( const uint64_t& contract, const string& k, string &v ) {
            CUniversalContractStore contractStore;
            CHAIN_ASSERT( database.contractCache.GetContract(CRegID(contract), contractStore),
                          contract_exception,
                          "contract '%s' does not exist",
                          wasm::regid(contract).to_string())

            return database.contractCache.GetContractData(CRegID(contract), k, v);
        }

        bool erase_data( const uint64_t& contract, const string& k ) {
            CUniversalContractStore contractStore;
            CHAIN_ASSERT( database.contractCache.GetContract(CRegID(contract), contractStore),
                          contract_exception,
                          "contract '%s' does not exist",
                          wasm::regid(contract).to_string())

            return database.contractCache.EraseContractData(CRegID(contract), k);
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

        void emit_result(const string_view &name, const string_view &type, const string_view &value) override {
            CHAIN_ASSERT( false, contract_exception, "%s() only used for rpc", __func__)
        }

        int64_t call_with_return(inline_transaction& t);

        uint64_t call(inline_transaction &inline_trx);//return with size
        void set_return(void *data, uint32_t data_len);
        std::vector<uint8_t> get_return();

        void append_log(uint64_t payer, uint64_t receiver, const string& topic, const string& data);

    public:
        CTxCord&                    trx_cord;
        inline_transaction&         trx;
        CUniversalTx&               control_trx;
        CCacheWrapper&              database;
        vector<CReceipt>&           receipts;
        vector<transaction_log>&    trx_logs;

        uint32_t                    recurse_depth;
        vector<uint64_t>            notified;
        vector<inline_transaction>  inline_transactions;

        wasm::wasm_interface        wasmif;
        vm::wasm_allocator          wasm_alloc;
        uint64_t                    _receiver;

        vector<vector<uint8_t>>     return_values;
        vector<uint8_t>             the_last_return_buffer;


        //inline_transaction_trace    *trace_ptr = nullptr;

    private:
        std::ostringstream         _pending_console_output;
    };
}
