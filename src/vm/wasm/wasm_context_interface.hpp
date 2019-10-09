#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "types/inline_transaction.hpp"


using namespace std;
namespace wasm {

    class CWasmContextInterface {

    public:
        CWasmContextInterface() {}
        ~CWasmContextInterface() {}

    public:
        virtual void ExecuteInline( inline_transaction trx ) {}
        virtual bool HasRecipient( uint64_t account ) const { return true; }
        virtual void require_recipient( uint64_t recipient ) {}
        virtual uint64_t Receiver() { return 0; }
        virtual uint64_t Contract() { return 0; }
        virtual uint64_t Action() { return 0; }
        virtual const char *GetActionData() { return nullptr; }
        virtual uint32_t GetActionDataSize() { return 0; }
        virtual bool SetData( uint64_t contract, string k, string v ) { return 0; }
        virtual bool GetData( uint64_t contract, string k, string &v ) { return 0; }
        virtual bool EraseData( uint64_t contract, string k ) { return 0; }
        virtual bool contracts_console() { return true; }
        virtual void console_append( string val ) {}

        virtual bool is_account( uint64_t account ) { return true; }
        virtual void require_auth( uint64_t account ) {}
        virtual void require_auth2( uint64_t account, uint64_t permission ) {}
        virtual bool has_authorization( uint64_t account ) const { return true; }


        virtual uint64_t block_time() { return 0; }

    };

}