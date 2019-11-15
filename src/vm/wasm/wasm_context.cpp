#include "wasm/wasm_context.hpp"
#include "wasm/wasm_native_contract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/types/name.hpp"
#include "wasm/wasm_config.hpp"
#include "wasm/wasm_log.hpp"
#include "entities/account.h"

using namespace std;
using namespace wasm;
// using std::chrono::microseconds;
// using std::chrono::system_clock;

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

    static inline void print_debug(uint64_t receiver, const inline_transaction_trace &trace) {
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
    }

    bool wasm_context::in_signatures(const permission& p){
        return std::find(trx.authorization.begin(), trx.authorization.end(), p) != trx.authorization.end();
    }

    void wasm_context::execute_inline(inline_transaction t) {

       for(const auto p: t.authorization){
         
          if(p.account  == _receiver){ continue; }
          if(t.contract == _receiver && in_signatures(p) ) { continue; }

           WASM_ASSERT(false , 
                       missing_auth_exception, 
                       "Do not get the authority by account %s ", 
                       wasm::name(p.account).to_string().c_str());     
       }

       inline_transactions.push_back(t);
    }

    std::vector <uint8_t> wasm_context::get_code(uint64_t account) {

        vector <uint8_t> code;
        CUniversalContract contract;
        CAccount contract_account ;
        if(database.accountCache.GetAccount(CNickID(wasm::name(account).to_string()),contract_account)
            && database.contractCache.GetContract(contract_account.regid, contract)) {
            code = vector <uint8_t>(contract.code.begin(), contract.code.end());
        }
        return code;
    }

    // std::string wasm_context::get_abi(uint64_t account) {

    //     CUniversalContract contract;
    //     CAccount contract_account ;
    //     database.accountCache.GetAccount(CNickID(wasm::name(account).to_string()),contract_account);
    //     database.contractCache.GetContract(contract_account.regid, contract);
    //     return contract.abi;
    // }

    void wasm_context::initialize() {

        static bool wasm_interface_inited = false;
        if (!wasm_interface_inited) {
            wasm_interface_inited = true;
            wasmif.initialize(wasm::vm_type::eos_vm_jit);
            register_native_handler(wasmio, N(setcode), wasm_native_setcode);
            register_native_handler(wasmio_bank, N(transfer), wasm_native_transfer);
        }
    }

    void wasm_context::execute(inline_transaction_trace &trace) {

        initialize();

        notified.push_back(_receiver);
        execute_one(trace);

        for (uint32_t i = 1; i < notified.size(); ++i) {
            _receiver = notified[i];

            trace.inline_traces.emplace_back();
            execute_one(trace.inline_traces.back());
        }

        WASM_ASSERT(recurse_depth < wasm::max_inline_transaction_depth,
                    transaction_exception, "%s",
                    "max inline transaction depth per transaction reached");

        for (auto &inline_trx : inline_transactions) {
            trace.inline_traces.emplace_back();
            control_trx.DispatchInlineTransaction(trace.inline_traces.back(), inline_trx, inline_trx.contract, database, recurse_depth + 1);
        }

    }

    void wasm_context::execute_one(inline_transaction_trace &trace) {

        //auto start = system_clock::now();
        trace.trx = trx;
        trace.receiver = _receiver;

        auto native = find_native_handle(_receiver, trx.action);

        try {
            if (native) {
                (*native)(*this);
            } else {
                vector <uint8_t> code = get_code(_receiver);
                if (code.size() > 0) {
                    wasmif.execute(code, this);
                }
            }
        } catch (wasm::exception &e) {
            std::ostringstream o;
            o << e.detail();
            if (_pending_console_output.str().size() > 0){
                o << " , " << tfm::format("pending console output: %s",
                                          _pending_console_output.str().c_str());
            }
            e.msg = o.str();
            throw;
        } catch (...) {
            if (_pending_console_output.str().size() > 0){
                throw wasm_exception(
                        tfm::format("pending console output: %s", _pending_console_output.str().c_str()).c_str());
            }
            else{
                throw;
            }
        }

        trace.trx_id = control_trx.GetHash();
        //trace.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - start);
        trace.console = _pending_console_output.str();

        // trace.block_height =
        // trace.block_time =
        reset_console();

        if (contracts_console()) {
            print_debug(_receiver, trace);
        }

    }

    bool wasm_context::has_recipient(uint64_t account) const {
        for (auto a : notified)
            if (a == account)
                return true;
        return false;
    }

    void wasm_context::require_recipient(uint64_t recipient) {

        if (!has_recipient(recipient)) {
            notified.push_back(recipient);
        }

    }

    void wasm_context::require_auth( uint64_t account ){
        for(auto p: trx.authorization){
            if(p.account == account){
                return;
            }
        }
        WASM_ASSERT(false, missing_auth_exception, "missing authority of %s", wasm::name(account).to_string().c_str());
    }

    bool wasm_context::is_account( uint64_t account ) { 

        auto account_name = wasm::name(account);
        return database.accountCache.HaveAccount(nick_name(account_name.to_string()));
    }

}