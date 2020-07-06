#include "wasm_context.hpp"
//#include "wasm/wasm_native_contract.hpp"
#include "wasm/exception/exceptions.hpp"
#include "wasm/types/name.hpp"
#include "wasm/wasm_constants.hpp"

using namespace std;
using namespace wasm;
using std::chrono::microseconds;
using std::chrono::system_clock;

const signed char p_util_hexdigit[256] = {
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  0,   1,   2,   3,  4,  5,  6,  7,  8,  9,   -1,  -1,
    -1,  -1,  -1,  -1, -1, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, 0xa, 0xb, 0xc,
    0xd, 0xe, 0xf, -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1,
};

namespace wasm {
    using nativeHandler = std::function<void(wasm_context & )>;
    map <pair<uint64_t, uint64_t>, nativeHandler>& get_wasm_native_handlers(){
        static map <pair<uint64_t, uint64_t>, nativeHandler> wasm_native_handlers;
        return wasm_native_handlers;
    }

    inline void register_native_handler(uint64_t receiver, uint64_t action, nativeHandler v) {
        get_wasm_native_handlers()[std::pair(receiver, action)] = v;
    }

    inline nativeHandler *find_native_handle(uint64_t receiver, uint64_t action) {
        auto handler = get_wasm_native_handlers().find(std::pair(receiver, action));
        if (handler != get_wasm_native_handlers().end()) {
            return &handler->second;
        }

        return nullptr;
    }

    bool CUniversalTx::ExecuteTx(wasm::transaction_trace &trx_trace, wasm::inline_transaction& trx){
        trx_trace.traces.emplace_back();
        execute_inline_transaction(trx_trace.traces.back(), trx, trx.contract, cache, state, 0);
        return true;

    };

    void CUniversalTx::execute_inline_transaction( wasm::inline_transaction_trace& trace,
                                    wasm::inline_transaction& trx,
                                     uint64_t receiver,
                                     CCacheWrapper &cache,
                                     CValidationState &state,
                                     uint32_t recurse_depth){

            //WASM_TRACE("%ld", receiver)
            wasm_context wasmContext(*this, trx, cache, state, false, recurse_depth);
            wasmContext._receiver = receiver;
            wasmContext.execute(trace);
    };

    static inline void print_debug( uint64_t receiver, const inline_transaction_trace &trace ) {
        if (!trace.console.empty()) {

            ostringstream prefix;

            auto contract_s = name(trace.trx.contract).to_string();
            auto action_s = name(trace.trx.action).to_string();
            auto receiver_s = name(receiver).to_string();

            prefix << "[(" << contract_s << "," << action_s << ")->" << receiver_s << "]";
            std::cout << prefix.str() << ": CONSOLE OUTPUT BEGIN =====================\n"
                      << trace.console << "\n"
                      << prefix.str() << ": CONSOLE OUTPUT END   =====================\n";
        }
    }


    void wasm_context::reset_console() {
        _pending_console_output = std::ostringstream();
        _pending_console_output.setf(std::ios::scientific, std::ios::floatfield);
    }

    void wasm_context::execute_inline( const inline_transaction& t ) {
        inline_transactions.push_back(t);
    }

    bool wasm_context::get_code(const uint64_t& contract, std::vector <uint8_t> &code) {
       return cache.GetCode(contract, code);
    }

    uint64_t wasm_context::get_fuel() {
        return trx.GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_per_byte;
    }

    void wasm_context::initialize() {

        static bool wasm_interface_inited = false;
        if (!wasm_interface_inited) {
            wasm_interface_inited = true;
            wasmif.initialize(wasm::vm_type::eos_vm_jit);
        }
        //RegisterNativeHandler(wasmio, N(setcode), WasmNativeSetcode);
        //RegisterNativeHandler(wasmio_bank, N(transfer), WasmNativeTransfer);
    }

    void wasm_context::execute( inline_transaction_trace &trace ) {

        initialize();

        notified.push_back(_receiver);
        execute_one(trace);

        for (uint32_t i = 1; i < notified.size(); ++i) {
            _receiver = notified[i];

            trace.inline_traces.emplace_back();
            execute_one(trace.inline_traces.back());
        }

        CHAIN_ASSERT( recurse_depth < wasm::max_inline_transaction_depth,
                      transaction_exception,
                      "max inline transaction depth per transaction reached");

        for (auto &inline_trx : inline_transactions) {
            trace.inline_traces.emplace_back();
            control_trx.execute_inline_transaction( trace.inline_traces.back(), inline_trx, inline_trx.contract, cache,
                                                    state, recurse_depth + 1);
        }

    }

    void wasm_context::execute_one( inline_transaction_trace &trace ) {

        auto start = system_clock::now();

        trace.trx = trx;
        trace.receiver = _receiver;

        try {
            vector <uint8_t> code;
            if (get_code(_receiver, code) && code.size() > 0)
                wasmif.execute(code, this);
        }
        CHAIN_RETHROW_EXCEPTIONS( wasm_exception, "pending console output: %s", _pending_console_output.str() )

        trace.console = _pending_console_output.str();

        reset_console();

        if (contracts_console()) {
            print_debug(_receiver, trace);
        }

    }

    bool wasm_context::has_recipient( const uint64_t& account ) const {
        for (auto a : notified)
            if (a == account)
                return true;
        return false;
    }

    void wasm_context::notify_recipient( const uint64_t& recipient ) {
        if (!has_recipient(recipient)) {
            notified.push_back(recipient);
        }

    }


}