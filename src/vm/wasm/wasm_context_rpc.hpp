#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasm_trace.hpp"
#include "eosio/vm/allocator.hpp"
#include "persistence/cachewrapper.h"
#include "wasm/exception/exceptions.hpp"
#include "wasm/wasm_control_rpc.hpp"

using namespace std;
using namespace wasm;
namespace wasm {

    // bool get_native_contract_abi(uint64_t contract, std::vector<char>& abi);
    // bool is_native_contract(uint64_t contract);
    //class wasm_control_rpc;
    class wasm_context_rpc : public wasm_context_interface {

    public:
        wasm_context_rpc(wasm_control_rpc &c, inline_transaction &t,  CCacheWrapper &cw, rpc_result_record &ret_value_in, uint32_t depth = 0)
                : control(c), trx(t), database(cw), ret_value(ret_value_in), recurse_depth(depth) {
            reset_console();
        };

        ~wasm_context_rpc() {
            wasm_alloc.free();
        };

    public:
        void initialize();
        void execute(inline_transaction_trace &trace);
        void execute_one(inline_transaction_trace &trace);
        bool has_permission_from_inline_transaction(const permission &p);
        bool get_code(const uint64_t& contract, std::vector <uint8_t> &code);
        uint64_t get_runcost();

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
        uint64_t    pending_block_time() {
            return control.current_block_time();
        }
        TxID        get_txid()  {
            return control.get_txid();
         }
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
            return true;
        }

        void console_append(const string& val) {
            _pending_console_output << val;
        }

        vm::wasm_allocator* get_wasm_allocator() { return &wasm_alloc; }
        bool                is_memory_in_wasm_allocator ( const uint64_t& p ) {
            return wasm_alloc.is_in_range(reinterpret_cast<const char*>(p));
        }
        std::chrono::milliseconds get_max_transaction_duration() {
            return std::chrono::milliseconds(wasm::max_wasm_execute_time_infinite);
        }
        void                      update_storage_usage( const uint64_t& account, const int64_t& size_in_bytes);
        void                      pause_billing_timer ()  { };
        void                      resume_billing_timer()  { };

        void set_rpc_result(const string_view &name, const string_view &value) override;

    public:
        wasm_control_rpc&           control;
        inline_transaction&         trx;
        CCacheWrapper&              database;
        rpc_result_record&          ret_value; // returned values
        uint32_t                    recurse_depth;
        vector<uint64_t>            notified;
        vector<inline_transaction>  inline_transactions;

        wasm::wasm_interface        wasmif;
        vm::wasm_allocator          wasm_alloc;
        uint64_t                    _receiver;

    private:
        std::ostringstream         _pending_console_output;
    };
}
