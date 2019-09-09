#include "wasm/wasmcontext.hpp"
#include "wasm/wasmnativecontract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/types/name.hpp"
#include "wasm/wasmconfig.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

    // CRegID Name2RegID( uint64_t account ) {
    //     uint32_t height = uint32_t(account);
    //     uint16_t index = uint16_t(account >> 32);
    //     return CRegID(height, index);
    // }

    // uint64_t RegID2Name( CRegID regID ) {

    //     uint64_t account = uint64_t(regID.GetIndex());
    //     account = (account << 32) + uint64_t(regID.GetHeight());

    //     return account;
    // }

    static inline void print_debug( uint64_t receiver, const inline_transaction_trace &trace ) {
        if (!trace.console.empty()) {

            ostringstream prefix;

            auto contract_s = name(trace.trx.contract).to_string();
            auto action_s = name(trace.trx.action).to_string();
            auto receiver_s = name(receiver).to_string();

            prefix << "[(" << contract_s << "," << action_s << ")->" << receiver_s << "]";
            //prefix << "\n [(" << trace.trx.contract << "," << trace.trx.action << ")->" << receiver << "]";

            std::cout << prefix.str() << ": CONSOLE OUTPUT BEGIN =====================\n"
                      << trace.console << "\n"
                      << prefix.str() << ": CONSOLE OUTPUT END   =====================\n";
        }
    }


    void CWasmContext::reset_console() {
        _pending_console_output = std::ostringstream();
        _pending_console_output.setf(std::ios::scientific, std::ios::floatfield);
    }


    void CWasmContext::ExecuteInline( CInlineTransaction t ) {
        //wasm_assert check the code and authorization
        inline_transactions.push_back(t);
        //queue.pushBack(t);
    }

    std::vector <uint8_t> CWasmContext::GetCode( uint64_t account ) {

        CUniversalContract contract;
        cache.contractCache.GetContract(Name2RegID(account), contract);

        //CRegID contractRegID = Name2RegID(account);

        // std::cout << "GetCode ------------------------------------------"
        //           << " contractRegId:" << contractRegID.ToString()
        //           << " abi:"<< contract.abi
        //           // << " data:"<< params[3].get_str()
        //           << " \n";

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

        wasmInterface.Initialize(wasm::vmType::eosvm);
        RegisterNativeHandler(name("wasmio").value, name("setcode").value, WasmNativeSetcode);

    }

    void CWasmContext::Execute( inline_transaction_trace &trace ) {


        //std::cout << "Execute ----------------------------"<< " \n";

        Initialize();

        notified.push_back(receiver);
        ExecuteOne(trace);

        for (uint32_t i = 1; i < notified.size(); ++i) {
            receiver = notified[i];

            trace.inline_traces.emplace_back();
            ExecuteOne(trace.inline_traces.back());
        }

        WASM_ASSERT(recurse_depth < wasm::max_inline_transaction_depth,
                    TRANSACTION_EXCEPTION, "transaction_exception",
                    "max inline transaction depth per transaction reached");

        for (auto &inline_trx : inline_transactions) {
            //std::cout << "Execute ----------------------------"<< inline_transactions.size() << " \n";
            trace.inline_traces.emplace_back();
            control_trx.DispatchInlineTransaction(trace.inline_traces.back(), inline_trx, inline_trx.contract, cache,
                                                  state, recurse_depth + 1);
        }

    }

    string VectorToHexString(std::vector<uint8_t> str, string separator = " ")
    {

        const std::string hex = "0123456789abcdef";
        std::stringstream ss;
     
        for (std::string::size_type i = 0; i < str.size(); ++i)
            ss << hex[(unsigned uint8_t)str[i] >> 4] << hex[(unsigned uint8_t)str[i] & 0xf] << separator;

        return ss.str();

    }

    void CWasmContext::ExecuteOne( inline_transaction_trace &trace ) {

        //std::cout << "ExecuteOne ----------------------------"<< " \n";

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
        } catch (CException &e) {
            throw e;
        }

        trace.trx_id = control_trx.GetHash();
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

    void CWasmContext::RequireRecipient( uint64_t recipient ) {

        if (!HasRecipient(recipient)) {
            //std::cout << "RequireRecipient" << recipient << std::endl;
            notified.push_back(recipient);
        }

    }

    void CWasmContext::RegisterNativeHandler( uint64_t receiver, uint64_t action, nativeHandler v ) {
        native_handlers[std::pair(receiver, action)] = v;
    }

    nativeHandler *CWasmContext::FindNativeHandle( uint64_t receiver, uint64_t action ) {

        auto handler = native_handlers.find(std::pair(receiver, action));
        if (handler != native_handlers.end()) {
            return &handler->second;
        }

        return nullptr;

    }


}