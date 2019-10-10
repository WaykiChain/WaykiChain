#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "tx/wasmcontracttx.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasm_trace.hpp"
#include "persistence/cachewrapper.h"

using namespace std;
using namespace wasm;
namespace wasm {

    static inline CRegID Name2RegID( uint64_t account ) {
        uint32_t height = uint32_t(account);
        uint16_t index = uint16_t(account >> 32);
        return CRegID(height, index);
    }

    static inline uint64_t RegID2Name( CRegID regID ) {
        uint64_t account = uint64_t(regID.GetIndex());
        account = (account << 32) + uint64_t(regID.GetHeight());
        return account;
    }

    class CWasmContext;

    class CWasmContext : public CWasmContextInterface {

    public:
        CWasmContext( CWasmContractTx &ctrl, inline_transaction &t, CCacheWrapper &cw, CValidationState &s,
                      uint32_t depth = 0 )
                : trx(t), control_trx(ctrl), cache(cw), state(s), recurse_depth(depth) {
            reset_console();
        };

        ~CWasmContext() {};

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
            return cache.contractCache.SetContractData(Name2RegID(contract), k, v);
        }
        bool get_data( uint64_t contract, string k, string &v ) {
            return cache.contractCache.GetContractData(Name2RegID(contract), k, v);
        }
        bool erase_data( uint64_t contract, string k ) {
            return cache.contractCache.EraseContractData(Name2RegID(contract), k);
        }
        bool contracts_console() { return true; } //should be set by console
        void console_append( string val ) {
            _pending_console_output << val;
        }

        bool is_account( uint64_t account ) { return true; }
        void require_auth( uint64_t account ) {}
        void require_auth2( uint64_t account, uint64_t permission ) {}
        bool has_authorization( uint64_t account ) const { return true; }
        uint64_t block_time() { return 0; }

    public:
        uint64_t _receiver;

        inline_transaction &trx;
        CWasmContractTx &control_trx;
        CCacheWrapper &cache;
        CValidationState &state;

        uint32_t recurse_depth;
        vector <uint64_t> notified;
        vector <inline_transaction> inline_transactions;

        CWasmInterface wasmInterface;

    private:
        std::ostringstream _pending_console_output;
    };
}
