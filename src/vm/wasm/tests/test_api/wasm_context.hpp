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

using namespace std;
using namespace wasm;
namespace wasm {



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

class CWasmContext;
using nativeHandler = std::function<void( CWasmContext & )>;


class CWasmContractTx{
public:
    CWasmContractTx(){}

    public:
    bool ExecuteTx(wasm::transaction_trace &trx_trace, wasm::inline_transaction& trx);
    void DispatchInlineTransaction( wasm::inline_transaction_trace& trace,
                                    wasm::inline_transaction& trx,
                                     uint64_t receiver,
                                     CCacheWrapper &cache,
                                     CValidationState &state,
                                     uint32_t recurse_depth);

    CCacheWrapper cache;
    CValidationState state;  
};



    class CWasmContext : public CWasmContextInterface {

    public:
        CWasmContext( CWasmContractTx &ctrl, inline_transaction &t, CCacheWrapper &cw, CValidationState &s,
                      uint32_t depth = 0 )
                : trx(t), control_trx(ctrl), cache(cw), state(s), recurse_depth(depth) {
            reset_console();
        };

        ~CWasmContext() {};

    public:
        std::vector <uint8_t> GetCode( uint64_t account );
        std::string GetAbi( uint64_t account );
        void RegisterNativeHandler( uint64_t receiver, uint64_t action, nativeHandler v );
        nativeHandler *FindNativeHandle( uint64_t receiver, uint64_t action );
        void ExecuteOne( inline_transaction_trace &trace );
        void Initialize();
        void Execute( inline_transaction_trace &trace );

// Console methods:
    public:
        void reset_console();
        std::ostringstream &get_console_stream() { return _pending_console_output; }
        const std::ostringstream &get_console_stream() const { return _pending_console_output; }

//virtual
    public:
        void ExecuteInline( inline_transaction t );
        bool HasRecipient( uint64_t account ) const;
        void RequireRecipient( uint64_t recipient );
        uint64_t Receiver() { return receiver; }
        uint64_t Contract() { return trx.contract; }
        uint64_t Action() { return trx.action; }
        const char *GetActionData() { return trx.data.data(); }
        uint32_t GetActionDataSize() { return trx.data.size(); }
        bool SetData( uint64_t contract, string k, string v ) {
            //return cache.contractCache.SetContractData(Name2RegID(contract), k, v);
            return cache.SetContractData(contract, k, v);
        }
        bool GetData( uint64_t contract, string k, string &v ) {
            //return cache.contractCache.GetContractData(Name2RegID(contract), k, v);
            return cache.GetContractData(contract, k, v);
        }
        bool EraseData( uint64_t contract, string k ) {
            //return cache.contractCache.EraseContractData(Name2RegID(contract), k);
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

    public:
        uint64_t receiver;

        inline_transaction &trx;
        CWasmContractTx &control_trx;
        CCacheWrapper &cache;
        CValidationState &state;

        uint32_t recurse_depth;
        vector <uint64_t> notified;
        vector <inline_transaction> inline_transactions;

        CWasmInterface wasmInterface;
        map <pair<uint64_t, uint64_t>, nativeHandler> native_handlers;

    private:
        std::ostringstream _pending_console_output;
    };
}
