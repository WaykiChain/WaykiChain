
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
using namespace std;
using namespace wasm;

namespace wasm {

    struct rpc_result_record {
        string name;
        string type;
        vector<char> value;
    };

    class wasm_control_rpc {

        public:
            wasm_control_rpc(CCacheWrapper &cw): database(cw){};
            ~wasm_control_rpc() {};

        public:
            void pause_billing_timer();
            void resume_billing_timer();
            string call_inline_transaction( wasm::inline_transaction& trx);
            void execute_inline_transaction(  wasm::inline_transaction_trace &trace,
                                            wasm::inline_transaction &trx,
                                            uint64_t                       receiver,
                                            uint32_t                       recurse_depth);
            TxID get_txid()  ;
            uint64_t current_block_time();

        public:
            CCacheWrapper &database;
            system_clock::time_point    pseudo_start;
            std::chrono::microseconds   billed_time              = chrono::microseconds(0);
            rpc_result_record           ret_value;

    };
}

