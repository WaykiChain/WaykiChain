#include "wasm/wasm_context.hpp"
#include "wasm/wasm_native_contract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/types/name.hpp"
#include "wasm/wasm_config.hpp"

using namespace std;
using namespace wasm;
using std::chrono::microseconds;
using std::chrono::system_clock;

namespace wasm {
    using nativeHandler = std::function<void( CWasmContext & )>;
    bool has_wasm_interface_initialized = false;
    map <pair<uint64_t, uint64_t>, nativeHandler> wasm_native_handlers;

    inline void RegisterNativeHandler( uint64_t receiver, uint64_t action, nativeHandler v ) {
        wasm_native_handlers[std::pair(receiver, action)] = v;
    }

    inline nativeHandler* FindNativeHandle( uint64_t receiver, uint64_t action ) {

        auto handler = wasm_native_handlers.find(std::pair(receiver, action));
        if (handler != wasm_native_handlers.end()) {
            return &handler->second;
        }

        return nullptr;

    }


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


    void CWasmContext::reset_console() {
        _pending_console_output = std::ostringstream();
        //_pending_console_output.setf(std::ios::scientific, std::ios::floatfield);
    }


    void CWasmContext::ExecuteInline( inline_transaction t ) {
        inline_transactions.push_back(t);
    }

    std::vector <uint8_t> CWasmContext::GetCode( uint64_t account ) {

        CUniversalContract contract;
        cache.contractCache.GetContract(Name2RegID(account), contract);

        vector <uint8_t> code;
        code.insert(code.begin(), contract.code.begin(), contract.code.end());

        return code;
    }

    std::string CWasmContext::GetAbi( uint64_t account ) {

        CUniversalContract contract;
        cache.contractCache.GetContract(Name2RegID(account), contract);

        return contract.abi;
    }

    void CWasmContext::Initialize() {

        if (!has_wasm_interface_initialized) {
            has_wasm_interface_initialized = true;
            wasmInterface.Initialize(wasm::vmType::eosvm);
            RegisterNativeHandler(wasmio, N(setcode), wasm_native_setcode);
            RegisterNativeHandler(wasmio_bank, N(transfer), wasm_native_transfer);
        }
    }

    void CWasmContext::Execute( inline_transaction_trace &trace ) {

        Initialize();

        notified.push_back(receiver);
        ExecuteOne(trace);

        for (uint32_t i = 1; i < notified.size(); ++i) {
            receiver = notified[i];

            trace.inline_traces.emplace_back();
            ExecuteOne(trace.inline_traces.back());
        }

        WASM_ASSERT(recurse_depth < wasm::max_inline_transaction_depth,
                    transaction_exception, "%s",
                    "max inline transaction depth per transaction reached");

        for (auto &inline_trx : inline_transactions) {
            trace.inline_traces.emplace_back();
            control_trx.DispatchInlineTransaction(trace.inline_traces.back(), inline_trx, inline_trx.contract, cache,
                                                  state, recurse_depth + 1);
        }

    }

    void CWasmContext::ExecuteOne( inline_transaction_trace &trace ) {

        auto start = system_clock::now();

        trace.trx = trx;
        trace.receiver = receiver;

        auto native = FindNativeHandle(receiver, trx.action);

        try {
            if (native) {
                (*native)(*this);
            } else {
                vector <uint8_t> code = GetCode(receiver);
                if (code.size() > 0) {
                    wasmInterface.Execute(code, this);
                }
            }
        } catch (wasm::exception &e) {
            std::ostringstream o;
            o << e.detail();

            if (_pending_console_output.str().size() > 0) o << " , " << tfm::format("pending console output: %s",
                                                                                    _pending_console_output.str().c_str());
            e.msg = o.str();
            throw;
        } catch (...) {
            if (_pending_console_output.str().size() > 0)
                throw wasm_exception(tfm::format("pending console output: %s", _pending_console_output.str().c_str()).c_str());
            else
                throw wasm_exception("wasm exception");
        }

        trace.trx_id = control_trx.GetHash();
        trace.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - start);
        trace.console = _pending_console_output.str();

        // trace.block_height =
        // trace.block_time =

        reset_console();

        if (contracts_console()) {
            print_debug(receiver, trace);
        }

    }

    bool CWasmContext::HasRecipient( uint64_t account ) const {
        for (auto a : notified)
            if (a == account)
                return true;
        return false;
    }

    void CWasmContext::require_recipient( uint64_t recipient ) {

        if (!HasRecipient(recipient)) {
            notified.push_back(recipient);
        }

    }


}