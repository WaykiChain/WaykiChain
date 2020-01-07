#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#include "wasm/wasm_config.hpp"
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
        virtual void     execute_inline( const inline_transaction& trx ) {}
        virtual bool     has_recipient    ( const uint64_t& account   ) const { return true; }
        virtual void     require_recipient( const uint64_t& recipient ) {}
        virtual uint64_t receiver() { return 0; }
        virtual uint64_t contract() { return 0; }
        virtual uint64_t action  () { return 0; }
        virtual const char* get_action_data     () { return nullptr; }
        virtual uint32_t    get_action_data_size() { return 0;       }
        virtual bool set_data  ( const uint64_t& contract, const string& k, const string& v ) { return 0; }
        virtual bool get_data  ( const uint64_t& contract, const string& k, string &v       ) { return 0; }
        virtual bool erase_data( const uint64_t& contract, const string& k                  ) { return 0; }

        virtual bool is_account       ( const uint64_t& account ) { return true; }
        virtual void require_auth     ( const uint64_t& account ) {}
        virtual void require_auth2    ( const uint64_t& account, const uint64_t& permission ) {}
        virtual bool has_authorization( const uint64_t& account ) const { return true; }
        virtual uint64_t block_time() { return 0; }
        virtual void     exit      () {}

        virtual std::vector<uint64_t> get_active_producers() { return std::vector<uint64_t>(); }
        virtual vm::wasm_allocator*   get_wasm_allocator()   { return nullptr;                 }
        virtual bool                  is_memory_in_wasm_allocator ( const uint64_t& p ) { 
            WASM_TRACE("%ld", p)
            return true;    
        }
        virtual std::chrono::milliseconds get_max_transaction_duration(){ return std::chrono::milliseconds(max_wasm_execute_time_infinite); }
        virtual void update_storage_usage(const uint64_t& account, const int64_t& size_in_bytes){}
        virtual bool contracts_console() { return true; }
        virtual void console_append   ( const string& val ) {}

        virtual void pause_billing_timer (){};
        virtual void resume_billing_timer(){};

    };

}