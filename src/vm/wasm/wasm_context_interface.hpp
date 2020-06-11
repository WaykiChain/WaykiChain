#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#include "wasm/wasm_constants.hpp"
#include "wasm/types/inline_transaction.hpp"
#include "eosio/vm/allocator.hpp"

using namespace eosio;
using namespace eosio::vm;
using namespace std;
namespace wasm {

    class wasm_context_interface {

    public:
        wasm_context_interface()  {}
        ~wasm_context_interface() {}

    public:
        virtual void     execute_inline( const inline_transaction& trx ) = 0;//{}
        virtual void     notify_recipient( const uint64_t& recipient )  = 0;//{}
        virtual bool     has_recipient    ( const uint64_t& account   )  const = 0;//{ return true; }
        virtual uint64_t receiver() = 0;//  { return 0; }
        virtual uint64_t contract() = 0;//  { return 0; }
        virtual uint64_t action  () = 0;//  { return 0; }
        virtual const char* get_action_data     () = 0;// { return nullptr; }
        virtual uint32_t    get_action_data_size() = 0;// { return 0;       }

        virtual bool is_account       ( const uint64_t& account ) const = 0;//{ return true; }
        virtual void require_auth     ( const uint64_t& account ) const = 0;//{}
        virtual bool has_authorization( const uint64_t& account ) const = 0;// { return true; }
        virtual void require_auth2    ( const uint64_t& account, const uint64_t& permission ) const = 0;// {}
        virtual uint64_t pending_block_time() = 0;//{ return 0; }
        virtual TxID        get_txid() = 0;
        virtual uint64_t    get_maintainer(const uint64_t& contract) = 0;//{ return 0; }
        virtual void exit      () = 0;//{}
        virtual bool get_system_asset_price(uint64_t base, uint64_t quote, std::vector<char>& price) = 0;

        virtual bool set_data  ( const uint64_t& contract, const string& k, const string& v ) = 0;//{ return 0; }
        virtual bool get_data  ( const uint64_t& contract, const string& k, string &v       ) = 0;//{ return 0; }
        virtual bool erase_data( const uint64_t& contract, const string& k                  ) = 0;//{ return 0; }

        virtual std::vector<uint64_t> get_active_producers() = 0;//{ return std::vector<uint64_t>(); }
        virtual vm::wasm_allocator*   get_wasm_allocator()   = 0;//{ return nullptr;                 }
        virtual bool                  is_memory_in_wasm_allocator ( const uint64_t& p ) = 0 ;
        // {
        //     WASM_TRACE("%ld", p)
        //     return true;
        // }
        virtual std::chrono::milliseconds get_max_transaction_duration() = 0;//{ return std::chrono::milliseconds(max_wasm_execute_time_infinite); }
        virtual void update_storage_usage(const uint64_t& account, const int64_t& size_in_bytes) = 0;//{}
        virtual bool contracts_console() = 0;//{ return true; }
        virtual void console_append   ( const string& val ) = 0;//{}

        virtual void pause_billing_timer () = 0;//{};
        virtual void resume_billing_timer() = 0;//{};

        virtual void set_rpc_result(const string_view &name, const string_view &type, const string_view &value) = 0; // only used for rpc
    };

}