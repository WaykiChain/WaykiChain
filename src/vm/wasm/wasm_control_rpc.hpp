
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
#include "wasm/wasm_variant_trace.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

    struct rpc_result_record {
        string name;
        string type;
        vector<char> value;
    };

    struct rpc_trx_trace {
        uint256                   trx_id;
        std::chrono::microseconds elapsed;
        vector <inline_transaction_trace> traces;

        WASM_REFLECT( rpc_trx_trace, (trx_id)(elapsed)(traces) )
    };

    class wasm_control_rpc {

    public:
        wasm_control_rpc(CCacheWrapper &cw) : database(cw){};
        ~wasm_control_rpc(){};

    public:
        void pause_billing_timer();
        void resume_billing_timer();
        void call_inline_transaction(wasm::inline_transaction &trx);
        void execute_inline_transaction(wasm::inline_transaction_trace &trace,
                                        wasm::inline_transaction &trx, uint64_t receiver,
                                        uint32_t recurse_depth);
        TxID get_txid();
        uint64_t current_block_time();

    public:
        CCacheWrapper &database;
        system_clock::time_point pseudo_start;
        std::chrono::microseconds billed_time = chrono::microseconds(0);
        rpc_result_record ret_value;
        wasm::rpc_trx_trace trx_trace;
    };


    template<typename Resolver>
    static inline void to_variant(const wasm::rpc_trx_trace &t, json_spirit::Value &v, Resolver resolver) {

        json_spirit::Object obj;

        json_spirit::Value val;
        to_variant(t.trx_id.ToString(), val);
        json_spirit::Config::add(obj, "trx_id", val);

        to_variant(t.elapsed.count(), val);
        json_spirit::Config::add(obj, "elapsed", val);

        if (t.traces.size() > 0) {
            json_spirit::Array arr;
            for (const auto &trace :t.traces) {
                json_spirit::Value tmp;
                to_variant(trace, tmp, resolver);
                arr.push_back(tmp);
            }

            json_spirit::Config::add(obj, "traces", json_spirit::Value(arr));
        }

        v = obj;
    }

}

