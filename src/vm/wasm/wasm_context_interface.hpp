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
        wasm_context_interface() {}
        ~wasm_context_interface() {}

    public:
        virtual void     execute_inline( inline_transaction trx ) {}
        virtual bool     has_recipient( uint64_t account ) const { return true; }
        virtual void     require_recipient( uint64_t recipient ) {}
        virtual uint64_t receiver() { return 0; }
        virtual uint64_t contract() { return 0; }
        virtual uint64_t action() { return 0; }
        virtual const char* get_action_data() { return nullptr; }
        virtual uint32_t    get_action_data_size() { return 0; }
        virtual bool set_data( uint64_t contract, string k, string v ) { return 0; }
        virtual bool get_data( uint64_t contract, string k, string &v ) { return 0; }
        virtual bool erase_data( uint64_t contract, string k ) { return 0; }
        virtual bool contracts_console() { return true; }
        virtual void console_append( string val ) {}
        virtual bool is_account( uint64_t account ) { return true; }
        virtual void require_auth( uint64_t account ) {}
        virtual void require_auth2( uint64_t account, uint64_t permission ) {}
        virtual bool has_authorization( uint64_t account ) const { return true; }
        virtual uint64_t block_time() { return 0; }
        virtual vm::wasm_allocator*       get_wasm_allocator(){ return nullptr; }
        virtual std::chrono::milliseconds get_max_transaction_duration(){ return std::chrono::milliseconds(max_wasm_execute_time_observe); }
        virtual void update_storage_usage(uint64_t account, int64_t size_in_bytes){}

        virtual void pause_billing_timer(){};
        virtual void resume_billing_timer(){};

    };

}