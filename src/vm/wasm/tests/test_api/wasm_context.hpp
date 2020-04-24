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

using namespace std;
using namespace wasm;
namespace wasm {

struct wasm_exit {
  int32_t code = 0;
};

template<typename T>
static inline string ToHex( const T &t, string separator = "" ) {
    const std::string hex = "0123456789abcdef";
    std::ostringstream o;

    for (std::string::size_type i = 0; i < t.size(); ++i)
        o << hex[(unsigned char) t[i] >> 4] << hex[(unsigned char) t[i] & 0xf] << separator;

    return o.str();

}


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

        // if(value.size() < 4096)
        //   WASM_TRACE("key:%s value:%s", ToHex(k), ToHex(value))

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
           //std::cout << "key:" << ToHex(iter->first) << " value:" << ToHex(iter->second) <<std::endl ;
           if(iter->second.size() < 4096)
            WASM_TRACE("key:%s value:%s", ToHex(iter->first), ToHex(iter->second))
    }

public:

	map<string, string> database;
};


class CUniversalContractTx{
public:
    CUniversalContractTx(){}

    public:
    bool ExecuteTx(wasm::transaction_trace &trx_trace, wasm::inline_transaction& trx);
    void execute_inline_transaction( wasm::inline_transaction_trace& trace,
                                     wasm::inline_transaction& trx,
                                     uint64_t receiver,
                                     CCacheWrapper &cache,
                                     CValidationState &state,
                                     uint32_t recurse_depth);

    CCacheWrapper cache;
    CValidationState state;
};



class wasm_context : public wasm_context_interface {

    public:
        wasm_context(CUniversalContractTx &ctrl, inline_transaction &t, CCacheWrapper &cw,
                     CValidationState &s, bool mining = false,
                     uint32_t depth = 0)
            : trx_cord(ctrl.txCord), trx(t), control_trx(ctrl), cache(cw), state(s), recurse_depth(depth) {
            reset_console();
        };

        ~wasm_context() {
            //wasm_alloc.free();
        };

    public:
        std::vector <uint8_t> get_code( const uint64_t& account );
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
        void execute_inline( const inline_transaction& t );
        bool has_recipient    ( const uint64_t& account   ) const;
        void notify_recipient( const uint64_t& recipient );
        uint64_t receiver() { return _receiver;    }
        uint64_t contract() { return trx.contract; }
        uint64_t action()   { return trx.action;   }
        const char *get_action_data()   { return trx.data.data(); }
        uint32_t get_action_data_size() { return trx.data.size(); }

        bool is_account   (const uint64_t& account)  const { return true; }
        void require_auth (const uint64_t& account ) const {}
        void require_auth2(const uint64_t& account, const uint64_t& permission ) const {}
        bool has_authorization( const uint64_t& account ) const {return true;}
        uint64_t pending_block_time() { return 0;      }
        string get_txid() { return ""; }
        void     exit() { wasmif.exit(); }
        bool get_system_asset_price(uint64_t base, uint64_t quote, std::vector<char>& price) { return false;};


        bool set_data  ( const uint64_t& contract, const string& k, const string& v )  { return cache.SetContractData(contract, k, v); }
        bool get_data  ( const uint64_t& contract, const string& k, string &v ) { return cache.GetContractData(contract, k, v); }
        bool erase_data( const uint64_t& contract, const string& k ) { return cache.EraseContractData(contract, k); }

        std::vector<uint64_t>    get_active_producers() { return std::vector<uint64_t>(); }
        vm::wasm_allocator*      get_wasm_allocator()   { return &wasm_alloc; }

        bool                     is_memory_in_wasm_allocator ( const uint64_t& p ) {
            return wasm_alloc.is_in_range(reinterpret_cast<const char*>(p));
        }
        std::chrono::milliseconds get_max_transaction_duration(){ return std::chrono::milliseconds(wasm::max_wasm_execute_time_infinite); }

        void update_storage_usage(const uint64_t& account, const int64_t& size_in_bytes){};
        bool contracts_console() { return true; } //should be set by console
        void console_append( const string& val ) {
            _pending_console_output << val;
        }

        void pause_billing_timer() { };
        void resume_billing_timer(){ };

    public:

        CTxCord                     trx_cord;
        inline_transaction&         trx;
        CUniversalContractTx&       control_trx;
        CCacheWrapper&              cache;
        CValidationState&           state;

        uint32_t                    recurse_depth;
        vector <uint64_t>           notified;
        vector <inline_transaction> inline_transactions;

        wasm::wasm_interface        wasmif;
        vm::wasm_allocator          wasm_alloc;
        uint64_t                    _receiver;
        //std::chrono::milliseconds ;

    private:
        std::ostringstream          _pending_console_output;
    };
}
