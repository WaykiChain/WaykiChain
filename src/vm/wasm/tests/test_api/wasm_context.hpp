#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <iostream>

#include "inline_transaction.hpp"
#include "name.hpp"
#include "asset.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasm_context_interface.hpp"
#include "wasm/wasm_trace.hpp"
#include "eosio/vm/allocator.hpp"
#include "entities/receipt.h"

using namespace std;
using namespace wasm;
namespace wasm {

struct wasm_exit {
  int32_t code = 0;
};


class CValidationState{
public:
    CValidationState(){}
};

class CCacheWrapper{
public:
    CCacheWrapper(){}
    bool GetContractData(const uint64_t &contract, const string &key, string &value){

        vector<char> buffer(sizeof(uint64_t) + key.size());
        wasm::datastream<char *> ds(buffer.data(),buffer.size());
        ds.write((char *)&contract, sizeof(uint64_t));
        ds.write(key.data(), key.size());
        string k(buffer.data(), buffer.size());

        auto iter = database.find(k);
        if( iter != database.end()){
        	value = iter->second;
        	return true;
        }

        return false;
    }

    bool SetContractData(const uint64_t &contract, const string &key, const string &value){

        vector<char> buffer(sizeof(uint64_t) + key.size());
        wasm::datastream<char *> ds(buffer.data(),buffer.size());
        ds.write((char *)&contract, sizeof(uint64_t));
        ds.write(key.data(), key.size());
        string k(buffer.data(), buffer.size());

        database[k] = value;

        return true;
    }

    bool EraseContractData(const uint64_t &contract, const string &key ){

        vector<char> buffer(sizeof(uint64_t) + key.size());
        wasm::datastream<char *> ds(buffer.data(),buffer.size());
        ds.write((char *)&contract, sizeof(uint64_t));
        ds.write(key.data(), key.size());
        string k(buffer.data(), buffer.size());

        database.erase(k);

        return true;
    }

    bool SetCode(const uint64_t account, std::vector <uint8_t> code) {

        string key("code");
        string value(code.begin(),code.end());
        SetContractData(account, key, value);
        return true;
    }

    std::vector <uint8_t> GetCode(const uint64_t account) {
        string key("code");
        string value;
        GetContractData(account, key, value);

        std::vector <uint8_t> code(value.begin(), value.end());
        return code;
    }

    void print() {
    	for(auto iter = database.begin(); iter != database.end(); iter++)
           std::cout << "key:" << iter->first << " value:" << iter->second<<std::endl ;
    }

public:

	map<string, string> database;
};


class CWasmContractTx{
public:
    CWasmContractTx(){}

    public:
    bool ExecuteTx(wasm::transaction_trace &trx_trace, wasm::inline_transaction& trx);
    void execute_inline_transaction( wasm::inline_transaction_trace& trace,
                                    wasm::inline_transaction& trx,
                                     uint64_t receiver,
                                     CCacheWrapper &cache,
                                     vector<CReceipt> &receipts,
                                     CValidationState &state,
                                     uint32_t recurse_depth);

    CCacheWrapper cache;
    CValidationState state;
};



class wasm_context : public wasm_context_interface {

    public:
        wasm_context(CWasmContractTx &ctrl, inline_transaction &t, CCacheWrapper &cw,
                     vector<CReceipt> &receipts, CValidationState &s, bool mining = false,
                     uint32_t depth = 0)
            : trx(t), control_trx(ctrl), cache(cw), state(s), recurse_depth(depth) {
            reset_console();
        };

        ~wasm_context() {};

    public:
        std::vector <uint8_t> get_code( uint64_t account );
        std::string get_abi( uint64_t account );
        void execute_one( inline_transaction_trace &trace );
        void initialize();
        void execute( inline_transaction_trace &trace );

// Console methods:
    public:
        void reset_console();
        std::ostringstream &get_console_stream() { return _pending_console_output; }
        const std::ostringstream &get_console_stream() const { return _pending_console_output; }

//virtual
    public:
        void execute_inline( inline_transaction t );
        bool has_recipient( uint64_t account ) const;
        void require_recipient( uint64_t recipient );
        uint64_t receiver() { return _receiver; }
        uint64_t contract() { return trx.contract; }
        uint64_t action() { return trx.action; }
        const char *get_action_data() { return trx.data.data(); }
        uint32_t get_action_data_size() { return trx.data.size(); }
        bool set_data( uint64_t contract, string k, string v ) {
            return cache.SetContractData(contract, k, v);
        }
        bool get_data( uint64_t contract, string k, string &v ) {
            return cache.GetContractData(contract, k, v);
        }
        bool erase_data( uint64_t contract, string k ) {
            return cache.EraseContractData(contract, k);
        }
        bool contracts_console() { return true; } //should be set by console
        void console_append( string val ) {
            _pending_console_output << val;
        }

        bool is_account(uint64_t account) { return true; }
        void require_auth( uint64_t account ) {}
        void require_auth2( uint64_t account, uint64_t permission ) {}
        bool has_authorization( uint64_t account ) const {return true;}
        uint64_t block_time() { return 0; }

        vm::wasm_allocator* get_wasm_allocator(){ return &wasm_alloc; }
        std::chrono::milliseconds get_transaction_duration(){ return std::chrono::milliseconds(wasm::max_wasm_execute_time_observe); }

        void update_storage_usage(uint64_t account, int64_t size_in_bytes){};

        void pause_billing_timer(){ };
        void resume_billing_timer(){ };

    public:
        uint64_t _receiver;

        inline_transaction &trx;
        CWasmContractTx &control_trx;
        CCacheWrapper &cache;
        CValidationState &state;
        vector<CReceipt> receipts;

        uint32_t recurse_depth;
        vector <uint64_t> notified;
        vector <inline_transaction> inline_transactions;

        wasm::wasm_interface wasmif;
        vm::wasm_allocator wasm_alloc;
        //std::chrono::milliseconds ;

    private:
        std::ostringstream _pending_console_output;
    };
}
