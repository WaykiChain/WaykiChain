#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#include "wasm/types/inline_transaction.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasm_trace.hpp"
#include "wasm/exception/exceptions.hpp"
#include "wasm/wasm_context_rpc.hpp"
#include "wasm/wasm_variant_trace.hpp"

using namespace wasm;

void wasm_control_rpc::pause_billing_timer() {

    if (billed_time > chrono::microseconds(0)) {
        return;// already paused
    }

    auto now    = system_clock::now();
    billed_time = std::chrono::duration_cast<std::chrono::microseconds>(now - pseudo_start);

}

void wasm_control_rpc::resume_billing_timer() {

    if (billed_time == chrono::microseconds(0)) {
        return;// already release pause
    }
    auto now     = system_clock::now();
    pseudo_start = now - billed_time;
    billed_time  = chrono::microseconds(0);
}

void wasm_control_rpc::call_inline_transaction(wasm::inline_transaction& trx){

    pseudo_start            = system_clock::now();
    trx_trace.traces.emplace_back();
    execute_inline_transaction(trx_trace.traces.back(), trx, trx.contract, 0);
    trx_trace.elapsed                 = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - pseudo_start);
}

void wasm_control_rpc::execute_inline_transaction(  wasm::inline_transaction_trace &trace,
                                  wasm::inline_transaction       &trx,
                                  uint64_t                        receiver,
                                  uint32_t                        recurse_depth){

    wasm_context_rpc wasm_execute_context(*this, trx, database, ret_value, recurse_depth);
    wasm_execute_context._receiver = receiver;
    wasm_execute_context.execute(trace);

}

TxID wasm_control_rpc::get_txid()  {
	check(false, "rpc call can not have txid");
    return TxID{};
}

uint64_t wasm_control_rpc::current_block_time(){
	return 0;
}
